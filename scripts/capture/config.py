"""Knob grid and mode constants for the DigiTech Synth Wah capture session.

The grid below is the exact 13-config sweep defined in
docs/superpowers/specs/2026-04-25-digitech-pedal-capture-design.md §4.
Configs 1-7 are noon-anchored (one knob varied at a time over a held
baseline); configs 8-13 detect cross-knob coupling.
"""
from __future__ import annotations

from dataclasses import dataclass


SAMPLE_RATE: int = 96000
BIT_DEPTH_SUBTYPE: str = "PCM_24"  # soundfile subtype string
CHANNELS: int = 1
KNOB_LETTERS: tuple[str, ...] = ("l", "m", "h")


@dataclass(frozen=True)
class KnobConfig:
    sens: str
    control: str
    rng: str

    def __post_init__(self) -> None:
        for label, value in (("sens", self.sens), ("control", self.control), ("rng", self.rng)):
            if value not in KNOB_LETTERS:
                raise ValueError(f"{label}={value!r} not in {KNOB_LETTERS}")


GRID: list[KnobConfig] = [
    KnobConfig("m", "m", "m"),  #  1: baseline
    KnobConfig("l", "m", "m"),  #  2: SENS sweep low
    KnobConfig("h", "m", "m"),  #  3: SENS sweep high
    KnobConfig("m", "l", "m"),  #  4: CONTROL sweep low
    KnobConfig("m", "h", "m"),  #  5: CONTROL sweep high
    KnobConfig("m", "m", "l"),  #  6: RANGE sweep low
    KnobConfig("m", "m", "h"),  #  7: RANGE sweep high
    KnobConfig("l", "l", "l"),  #  8: all-low corner
    KnobConfig("h", "h", "h"),  #  9: all-high corner
    KnobConfig("l", "h", "h"),  # 10: interaction
    KnobConfig("h", "l", "h"),  # 11: interaction
    KnobConfig("h", "h", "l"),  # 12: interaction
    KnobConfig("l", "l", "h"),  # 13: interaction
]

assert len(GRID) == 13, "config grid must hold exactly 13 entries"

MODES: dict[int, str] = {
    1: "Env Up",
    5: "Filter 1 (Synth Talk)",
}


def prefix(mode: int, cfg: KnobConfig) -> str:
    """Filename prefix per spec §6 — e.g. 'm1_sl_cm_rm'."""
    return f"m{mode}_s{cfg.sens}_c{cfg.control}_r{cfg.rng}"
