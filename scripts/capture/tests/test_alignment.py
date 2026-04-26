"""Tests for sync-chirp alignment recovery."""
from __future__ import annotations

import numpy as np
import pytest

from alignment import correlation_quality, find_offset
from stimulus import build_training_stimulus


@pytest.mark.parametrize("offset", [0, 64, 256, 1024, 4096])
def test_find_offset_recovers_known_offset_clean(offset: int, sr: int) -> None:
    stim = build_training_stimulus(sr=sr)
    # Simulate "recorded" by prepending `offset` samples of silence.
    recorded = np.concatenate([np.zeros(offset, dtype=np.float32), stim]).astype(np.float32)
    recovered = find_offset(stim, recorded, sr=sr)
    assert abs(recovered - offset) <= 1, f"recovered {recovered}, expected {offset}"


def test_find_offset_robust_under_noise(sr: int, rng: np.random.Generator) -> None:
    """SNR ≈ 20 dB additive white noise should not break alignment."""
    stim = build_training_stimulus(sr=sr)
    offset = 512
    recorded = np.concatenate([np.zeros(offset, dtype=np.float32), stim]).astype(np.float32)
    noise_amp = float(np.sqrt(np.mean(stim ** 2))) * 0.1  # 20 dB below RMS signal
    recorded = recorded + noise_amp * rng.standard_normal(len(recorded)).astype(np.float32)
    recovered = find_offset(stim, recorded, sr=sr)
    assert abs(recovered - offset) <= 1


def test_correlation_quality_high_for_correct_offset(sr: int) -> None:
    stim = build_training_stimulus(sr=sr)
    offset = 200
    recorded = np.concatenate([np.zeros(offset, dtype=np.float32), stim]).astype(np.float32)
    q = correlation_quality(stim, recorded, offset=offset)
    assert q > 0.95, f"quality {q:.3f} should be >0.95 for clean alignment"


def test_correlation_quality_low_for_random_signal(sr: int, rng: np.random.Generator) -> None:
    stim = build_training_stimulus(sr=sr)
    junk = rng.standard_normal(len(stim) + 1024).astype(np.float32) * 0.1
    q = correlation_quality(stim, junk, offset=0)
    assert q < 0.5, f"quality {q:.3f} should be <0.5 for unrelated signal"
