"""Tests for the training and calibration stimulus generators."""
from __future__ import annotations

import numpy as np
import pytest

from stimulus import (
    build_calibration_stimulus,
    build_training_stimulus,
    SYNC_CHIRP_DURATION_S,
    SYNC_CHIRP_F_START_HZ,
    SYNC_CHIRP_F_END_HZ,
    TRAINING_TOTAL_DURATION_S,
)


def test_training_total_length_matches_spec(sr: int) -> None:
    """Spec §3 claims ~4:35 = 275s. Allow ±0.5s slack for segment rounding."""
    stim = build_training_stimulus(sr=sr)
    expected_samples = int(TRAINING_TOTAL_DURATION_S * sr)
    assert abs(len(stim) - expected_samples) <= int(0.5 * sr), (
        f"got {len(stim)/sr:.2f}s, expected ~{TRAINING_TOTAL_DURATION_S}s"
    )


def test_training_starts_with_unit_impulse(sr: int) -> None:
    """First sample is the alignment impulse (sample 0, -6 dBFS)."""
    stim = build_training_stimulus(sr=sr)
    impulse_amp = 10 ** (-6.0 / 20.0)
    assert abs(stim[0]) == pytest.approx(impulse_amp, rel=1e-3)
    assert abs(stim[1]) < 1e-6  # only the very first sample is the impulse


def test_training_chirp_band_matches_spec(sr: int) -> None:
    """The 0.5s chirp after the impulse spans 100 Hz → 10 kHz."""
    stim = build_training_stimulus(sr=sr)
    # Chirp lives at samples [1, 1 + chirp_len].
    chirp_len = int(SYNC_CHIRP_DURATION_S * sr)
    chirp = stim[1 : 1 + chirp_len]
    spectrum = np.abs(np.fft.rfft(chirp))
    freqs = np.fft.rfftfreq(chirp_len, d=1.0 / sr)
    # Energy should concentrate inside [SYNC_CHIRP_F_START_HZ, SYNC_CHIRP_F_END_HZ].
    in_band = (freqs >= SYNC_CHIRP_F_START_HZ) & (freqs <= SYNC_CHIRP_F_END_HZ)
    in_band_energy = (spectrum[in_band] ** 2).sum()
    out_band_energy = (spectrum[~in_band] ** 2).sum()
    assert in_band_energy > 50 * out_band_energy, "chirp leaks excessive out-of-band energy"


def test_training_levels_never_clip(sr: int) -> None:
    """Stimulus must not exceed digital full-scale. Spec §3 allows up to 0 dBFS
    in the waveshaper segment, so the threshold is essentially 1.0."""
    stim = build_training_stimulus(sr=sr)
    peak = np.max(np.abs(stim))
    assert peak <= 1.001, f"stimulus peak {peak:.3f} exceeds digital full-scale"


def test_calibration_stimulus_has_sync_header_and_short_total(sr: int) -> None:
    cal = build_calibration_stimulus(sr=sr)
    impulse_amp = 10 ** (-6.0 / 20.0)
    assert abs(cal[0]) == pytest.approx(impulse_amp, rel=1e-3)
    # Cal stimulus must be much shorter than the training one.
    assert len(cal) <= int(30 * sr), "calibration stimulus exceeds 30s"
    assert len(cal) >= int(15 * sr), "calibration stimulus too short for noise-floor measurement"


def test_training_is_mono_1d_float32(sr: int) -> None:
    stim = build_training_stimulus(sr=sr)
    assert stim.ndim == 1
    assert stim.dtype == np.float32
