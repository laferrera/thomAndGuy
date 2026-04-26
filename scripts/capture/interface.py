"""Thin wrapper over `sounddevice` for sample-accurate play+record."""
from __future__ import annotations

import numpy as np
import sounddevice as sd


def list_audio_devices() -> list[dict]:
    """Return the list of devices Core Audio can see, with index + name + I/O channels."""
    return [
        {
            "index": i,
            "name": d["name"],
            "max_in": d["max_input_channels"],
            "max_out": d["max_output_channels"],
        }
        for i, d in enumerate(sd.query_devices())
    ]


def play_record(
    out_signal: np.ndarray,
    sr: int,
    in_channels: int,
    output_device: int | str | None = None,
    input_device: int | str | None = None,
) -> np.ndarray:
    """Play `out_signal` (mono float32) and simultaneously record `in_channels`
    inputs. Blocks until playback ends. Returns array of shape (n_samples, in_channels).
    """
    if out_signal.ndim != 1:
        raise ValueError(f"out_signal must be 1-D mono; got shape {out_signal.shape}")
    if out_signal.dtype != np.float32:
        out_signal = out_signal.astype(np.float32)

    if output_device is not None or input_device is not None:
        sd.default.device = (input_device, output_device)
    sd.default.samplerate = sr

    recorded = sd.playrec(
        out_signal,
        samplerate=sr,
        channels=in_channels,
        dtype="float32",
        blocking=True,
    )
    return np.asarray(recorded)
