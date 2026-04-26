"""Training-WAV and calibration-stimulus generators.

Produces deterministic float32 mono numpy arrays. Levels are linear
amplitudes such that 1.0 == 0 dBFS. The training stimulus is the per-pass
playback file; the calibration stimulus is used during the 4-step
pre-flight pass and only needs to carry the sync header and a short
broadband segment.
"""
from __future__ import annotations

import numpy as np
from scipy.signal import chirp


# ─────────────────── Shared constants ──────────────────────────────
SAMPLE_RATE: int = 96000

SYNC_IMPULSE_DBFS: float = -6.0
SYNC_CHIRP_DURATION_S: float = 0.5
SYNC_CHIRP_F_START_HZ: float = 100.0
SYNC_CHIRP_F_END_HZ: float = 10_000.0
SYNC_CHIRP_DBFS: float = -12.0
SYNC_TRAILING_SILENCE_S: float = 4.5

NOISE_FLOOR_SILENCE_S: float = 30.0

# Total budgeted training length (informational; tests allow ±0.5s slack).
TRAINING_TOTAL_DURATION_S: float = 275.0  # 4:35 per spec §3


# ─────────────────── Helpers ───────────────────────────────────────
def _dbfs_to_amp(dbfs: float) -> float:
    return float(10.0 ** (dbfs / 20.0))


def _silence(seconds: float, sr: int) -> np.ndarray:
    return np.zeros(int(round(seconds * sr)), dtype=np.float32)


def _sine(seconds: float, freq_hz: float, dbfs: float, sr: int) -> np.ndarray:
    n = int(round(seconds * sr))
    t = np.arange(n, dtype=np.float32) / sr
    return _dbfs_to_amp(dbfs) * np.sin(2 * np.pi * freq_hz * t).astype(np.float32)


def _log_sine_sweep(seconds: float, f0: float, f1: float, dbfs: float, sr: int) -> np.ndarray:
    n = int(round(seconds * sr))
    t = np.arange(n, dtype=np.float32) / sr
    # phi=-90 deg makes the sweep start at amplitude 0 (sin-phase) so the
    # sample immediately after the impulse is silent, satisfying the
    # alignment test's "isolated impulse" expectation.
    sweep = chirp(t, f0=f0, f1=f1, t1=seconds, method="logarithmic", phi=-90)
    return (_dbfs_to_amp(dbfs) * sweep).astype(np.float32)


def _two_tone(seconds: float, f_a: float, f_b: float, dbfs: float, sr: int) -> np.ndarray:
    a = _sine(seconds, f_a, dbfs - 6.0, sr)  # -6 dB each so sum peaks at dbfs
    b = _sine(seconds, f_b, dbfs - 6.0, sr)
    return (a + b).astype(np.float32)


def _amplitude_ramp(seconds: float, freq_hz: float, peak_dbfs: float, sr: int) -> np.ndarray:
    """Triangular envelope: 0 → peak → 0 over the segment, modulating a sine."""
    n = int(round(seconds * sr))
    half = n // 2
    env = np.concatenate([np.linspace(0, 1, half, dtype=np.float32),
                          np.linspace(1, 0, n - half, dtype=np.float32)])
    tone = _sine(seconds, freq_hz, peak_dbfs, sr)
    return (env * tone).astype(np.float32)


def _pink_noise(seconds: float, dbfs: float, sr: int, seed: int) -> np.ndarray:
    """Paul Kellet's 3-stage pink-noise filter (approx. -3 dB/oct)."""
    rng = np.random.default_rng(seed)
    n = int(round(seconds * sr))
    white = rng.standard_normal(n).astype(np.float32)
    b0, b1, b2 = 0.0, 0.0, 0.0
    out = np.empty(n, dtype=np.float32)
    for i in range(n):
        b0 = 0.99765 * b0 + white[i] * 0.0990460
        b1 = 0.96300 * b1 + white[i] * 0.2965164
        b2 = 0.57000 * b2 + white[i] * 1.0526913
        out[i] = b0 + b1 + b2 + white[i] * 0.1848
    out /= np.max(np.abs(out)) + 1e-12
    return (_dbfs_to_amp(dbfs) * out).astype(np.float32)


def _white_noise(seconds: float, dbfs: float, sr: int, seed: int) -> np.ndarray:
    rng = np.random.default_rng(seed)
    n = int(round(seconds * sr))
    out = rng.standard_normal(n).astype(np.float32)
    out /= np.max(np.abs(out)) + 1e-12
    return (_dbfs_to_amp(dbfs) * out).astype(np.float32)


def _pad_to(signal: np.ndarray, target_seconds: float, sr: int) -> np.ndarray:
    """Append zero-padding so the result is exactly target_seconds long."""
    target = int(round(target_seconds * sr))
    if len(signal) >= target:
        return signal[:target]
    return np.concatenate([signal, np.zeros(target - len(signal), dtype=np.float32)])


# ─────────────────── Segment builders ──────────────────────────────
def _sync_header(sr: int) -> np.ndarray:
    impulse = np.zeros(1, dtype=np.float32)
    impulse[0] = _dbfs_to_amp(SYNC_IMPULSE_DBFS)
    chirp_seg = _log_sine_sweep(
        SYNC_CHIRP_DURATION_S,
        SYNC_CHIRP_F_START_HZ,
        SYNC_CHIRP_F_END_HZ,
        SYNC_CHIRP_DBFS,
        sr,
    )
    silence = _silence(SYNC_TRAILING_SILENCE_S, sr)
    return np.concatenate([impulse, chirp_seg, silence])


def _envelope_probes(sr: int) -> np.ndarray:
    """60 s — 12 level-step bursts + 6 amplitude ramps + trailing silence. Spec §3."""
    parts: list[np.ndarray] = []
    levels_db = [-40.0, -30.0, -20.0, -12.0, -6.0, -3.0, -6.0, -12.0, -20.0, -30.0, -40.0, -30.0]
    for db in levels_db:
        parts.append(_sine(0.2, 220.0, db, sr))
        parts.append(_silence(0.3, sr))
    ramp_levels = [-20.0, -12.0, -6.0, -3.0, -12.0, -20.0]
    for db in ramp_levels:
        parts.append(_amplitude_ramp(4.0, 220.0, db, sr))
        parts.append(_silence(1.0, sr))
    return _pad_to(np.concatenate(parts), 60.0, sr)


def _waveshaper_probes(sr: int) -> np.ndarray:
    """60 s — stepped 220 Hz sines + 220/330 Hz two-tones + trailing silence. Spec §3."""
    parts: list[np.ndarray] = []
    for db in [-40.0, -30.0, -20.0, -12.0, -6.0, -3.0, 0.0]:
        parts.append(_sine(3.0, 220.0, db, sr))
        parts.append(_silence(0.5, sr))
    for db in [-20.0, -12.0, -6.0]:
        parts.append(_two_tone(3.0, 220.0, 330.0, db, sr))
        parts.append(_silence(0.5, sr))
    return _pad_to(np.concatenate(parts), 60.0, sr)


def _filter_probes(sr: int) -> np.ndarray:
    """60 s — held tone then sweep, repeated at 5 levels + trailing silence. Spec §3."""
    parts: list[np.ndarray] = []
    for db in [-30.0, -20.0, -12.0, -6.0, -3.0]:
        parts.append(_sine(1.5, 220.0, db, sr))
        parts.append(_log_sine_sweep(2.0, 50.0, 10_000.0, db, sr))
        parts.append(_silence(0.5, sr))
    return _pad_to(np.concatenate(parts), 60.0, sr)


def _noise_probes(sr: int) -> np.ndarray:
    """30 s — 15s pink + 15s white at -20 dBFS. Spec §3."""
    return np.concatenate([
        _pink_noise(15.0, -20.0, sr, seed=0xBADBEEF),
        _white_noise(15.0, -20.0, sr, seed=0xFEEDFACE),
    ])


def _impulse_train(sr: int) -> np.ndarray:
    """30 s — 30 single-sample clicks at -6 dBFS, 1 s apart."""
    out = np.zeros(int(round(30.0 * sr)), dtype=np.float32)
    amp = _dbfs_to_amp(-6.0)
    for i in range(30):
        out[i * sr] = amp
    return out


# ─────────────────── Public API ────────────────────────────────────
def build_training_stimulus(sr: int = SAMPLE_RATE) -> np.ndarray:
    return np.concatenate([
        _sync_header(sr),
        _silence(NOISE_FLOOR_SILENCE_S, sr),
        _envelope_probes(sr),
        _waveshaper_probes(sr),
        _filter_probes(sr),
        _noise_probes(sr),
        _impulse_train(sr),
    ]).astype(np.float32)


def build_calibration_stimulus(sr: int = SAMPLE_RATE) -> np.ndarray:
    """~20 s: sync header + 5s pink noise + 5s silence + 5 impulses."""
    return np.concatenate([
        _sync_header(sr),
        _pink_noise(5.0, -20.0, sr, seed=0xCAFEBABE),
        _silence(5.0, sr),
        _impulse_train(sr)[: 5 * sr],  # first 5 of the 30 impulses
    ]).astype(np.float32)
