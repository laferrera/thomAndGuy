"""Sample-accurate alignment of recorded WAVs against the played stimulus.

Strategy: cross-correlate the first ~0.5 s of the recorded signal against
the chirp segment that's at known position 1..1+chirp_len in the stimulus.
The chirp's autocorrelation has a sharp central peak, so the argmax of the
correlation gives the integer-sample offset.
"""
from __future__ import annotations

import numpy as np
from scipy.signal import correlate

from stimulus import (
    SYNC_CHIRP_DURATION_S,
    SYNC_TRAILING_SILENCE_S,
)


def _chirp_template(stimulus: np.ndarray, sr: int) -> np.ndarray:
    """The chirp lives immediately after the 1-sample impulse at index 0."""
    chirp_start = 1
    chirp_end = chirp_start + int(SYNC_CHIRP_DURATION_S * sr)
    return stimulus[chirp_start:chirp_end].astype(np.float32)


def find_offset(stimulus: np.ndarray, recorded: np.ndarray, sr: int) -> int:
    """Return integer sample offset such that recorded[offset+1:] matches the chirp.

    Search window: only the first SYNC_CHIRP_DURATION_S + SYNC_TRAILING_SILENCE_S
    seconds of `recorded` — keeps the FFT small and avoids spurious matches deeper
    in the file (e.g., the impulse train at the end).
    """
    template = _chirp_template(stimulus, sr)
    max_search_samples = int((SYNC_CHIRP_DURATION_S + SYNC_TRAILING_SILENCE_S) * sr) + len(template)
    head = recorded[:max_search_samples].astype(np.float32)
    corr = correlate(head, template, mode="valid", method="fft")
    # `recorded[offset+1:offset+1+len(template)]` ≈ template ⇒ template aligns at
    # corr argmax. Subtract 1 because the impulse is at index 0 (the chirp starts
    # at index 1).
    chirp_start_in_recorded = int(np.argmax(np.abs(corr)))
    return max(0, chirp_start_in_recorded - 1)


def correlation_quality(stimulus: np.ndarray, recorded: np.ndarray, offset: int) -> float:
    """Normalized cross-correlation in [0, 1] between the chirp template and
    recorded[offset+1 : offset+1+len(template)]. Used as a sanity gate."""
    sr_guess = 96000  # quality is scale-free wrt sample rate; cheap default
    template = _chirp_template(stimulus, sr_guess)
    start = offset + 1
    end = start + len(template)
    if end > len(recorded):
        return 0.0
    window = recorded[start:end].astype(np.float32)
    num = float(np.dot(template, window))
    den = float(np.linalg.norm(template) * np.linalg.norm(window) + 1e-12)
    return abs(num / den)
