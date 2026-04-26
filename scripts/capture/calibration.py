"""Pre-flight calibration pass for the capture rig.

Runs the four steps described in spec §5, writes their WAVs into
`captures_dir/calibration/`, and aggregates the derived quantities into
`captures_dir/calibration/calibration.json`.
"""
from __future__ import annotations

import json
from datetime import datetime, timezone
from pathlib import Path

import numpy as np
import soundfile as sf

from alignment import find_offset
from config import BIT_DEPTH_SUBTYPE, SAMPLE_RATE
from interface import play_record
from stimulus import build_calibration_stimulus


# ─────────────────── Pure-math helpers ─────────────────────────────
def _rms_dbfs(signal: np.ndarray) -> float:
    rms = float(np.sqrt(np.mean(signal.astype(np.float64) ** 2)) + 1e-20)
    return 20.0 * np.log10(rms)


def _measure_hum_dbfs(signal: np.ndarray, sr: int, hum_hz: float = 60.0) -> float:
    """Bin-level magnitude at `hum_hz` after FFT of a silent recording."""
    spec = np.abs(np.fft.rfft(signal.astype(np.float64)))
    freqs = np.fft.rfftfreq(len(signal), d=1.0 / sr)
    bin_idx = int(np.argmin(np.abs(freqs - hum_hz)))
    mag = spec[bin_idx] / (len(signal) / 2.0)
    return 20.0 * np.log10(mag + 1e-20)


def _gain_db(stimulus: np.ndarray, recorded: np.ndarray, offset: int) -> float:
    """Mean log-amplitude ratio across the chirp segment."""
    template_start = 1 + offset
    template_len = int(0.5 * SAMPLE_RATE)
    rec_window = recorded[template_start : template_start + template_len]
    stim_window = stimulus[1 : 1 + template_len]
    rec_rms = float(np.sqrt(np.mean(rec_window.astype(np.float64) ** 2)) + 1e-20)
    stim_rms = float(np.sqrt(np.mean(stim_window.astype(np.float64) ** 2)) + 1e-20)
    return 20.0 * np.log10(rec_rms / stim_rms)


def _bypass_null_db(
    stimulus: np.ndarray,
    recorded: np.ndarray,
    offset: int,
    dac_to_adc_db: float,
) -> float:
    """RMS of (stimulus - gain-corrected recorded) inside the chirp window, in dBFS."""
    template_len = int(0.5 * SAMPLE_RATE)
    stim_window = stimulus[1 : 1 + template_len].astype(np.float64)
    rec_window = recorded[offset + 1 : offset + 1 + template_len].astype(np.float64)
    if len(rec_window) < len(stim_window):
        return 0.0
    gain = 10.0 ** (-dac_to_adc_db / 20.0)
    residual = stim_window - gain * rec_window
    return _rms_dbfs(residual)


# ─────────────────── Step orchestration ────────────────────────────
def _prompt(message: str) -> None:
    print()
    print(f"=== {message} ===")
    input("Press [Enter] when ready: ")


def _record_step(
    label: str,
    stimulus: np.ndarray,
    in_channels: int,
    out_path: Path,
    interface_kwargs: dict,
) -> np.ndarray:
    print(f"recording {label}...")
    recorded = play_record(
        out_signal=stimulus,
        sr=SAMPLE_RATE,
        in_channels=in_channels,
        **interface_kwargs,
    )
    sf.write(out_path, recorded, SAMPLE_RATE, subtype=BIT_DEPTH_SUBTYPE)
    print(f"  wrote {out_path}")
    return recorded


def run_preflight(captures_dir: Path, interface_kwargs: dict | None = None) -> dict:
    interface_kwargs = interface_kwargs or {}
    cal_dir = captures_dir / "calibration"
    cal_dir.mkdir(parents=True, exist_ok=True)
    stim = build_calibration_stimulus()

    # Step 1: loopback (no pedal, no reamp).
    _prompt("Step 1/4: connect interface OUT1 directly to interface IN1+IN2 (no pedal, no reamp)")
    rec1 = _record_step("loopback", stim, 2, cal_dir / "01_loopback.wav", interface_kwargs)
    rt_loopback = find_offset(stim, rec1[:, 0], sr=SAMPLE_RATE)
    dac_to_adc_db = _gain_db(stim, rec1[:, 0], rt_loopback)

    # Step 2: reamp (no pedal).
    _prompt("Step 2/4: insert reamp inline (OUT1 → reamp → IN1, no pedal yet)")
    _record_step("reamp", stim, 1, cal_dir / "02_reamp.wav", interface_kwargs)
    # No further metric needed beyond the WAV; downstream fitter can null this if it cares.

    # Step 3: pedal in bypass.
    _prompt("Step 3/4: connect pedal (footswitch BYPASSED). Reamp → pedal IN; pedal OUT1 → IN1; pedal OUT2 → IN2")
    rec3 = _record_step("pedal_bypass", stim, 2, cal_dir / "03_pedal_bypass.wav", interface_kwargs)
    rt_with_pedal = find_offset(stim, rec3[:, 0], sr=SAMPLE_RATE)
    rt_pedal_out2 = find_offset(stim, rec3[:, 1], sr=SAMPLE_RATE)
    internal_pedal_latency = rt_pedal_out2 - rt_with_pedal
    # True-bypass = OUT1 closely matches stimulus after gain & latency match.
    bypass_residual_db = _bypass_null_db(stim, rec3[:, 0], rt_with_pedal, dac_to_adc_db)
    pedal_is_true_bypass = bypass_residual_db < -40.0  # < -40 dB residual = true bypass

    # Step 4: pedal engaged but silent.
    _prompt("Step 4/4: ENGAGE the footswitch (effect on). Send NO INPUT — script plays silence")
    silent_stim = np.zeros_like(stim)
    rec4 = _record_step("pedal_engaged_silent", silent_stim, 2, cal_dir / "04_pedal_engaged_silent.wav", interface_kwargs)
    noise_floor = _rms_dbfs(rec4[:, 1])
    hum_60 = _measure_hum_dbfs(rec4[:, 1], SAMPLE_RATE, 60.0)

    cal = {
        "samplerate": SAMPLE_RATE,
        "rt_latency_samples_loopback": int(rt_loopback),
        "rt_latency_samples_with_pedal": int(rt_with_pedal),
        "internal_pedal_latency_samples": int(internal_pedal_latency),
        "dac_to_adc_gain_db": round(float(dac_to_adc_db), 3),
        "noise_floor_dbfs": round(float(noise_floor), 2),
        "hum_60hz_dbfs": round(float(hum_60), 2),
        "pedal_is_true_bypass": bool(pedal_is_true_bypass),
        "bypass_residual_dbfs": round(float(bypass_residual_db), 2),
        "captured_at": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
    }
    cal_json = cal_dir / "calibration.json"
    cal_json.write_text(json.dumps(cal, indent=2))
    print()
    print(f"=== Calibration complete → {cal_json} ===")
    print(json.dumps(cal, indent=2))
    return cal
