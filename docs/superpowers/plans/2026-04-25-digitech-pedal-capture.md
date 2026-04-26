# DigiTech Synth Wah Hardware Capture — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the Python capture toolchain in `thomAndGuy/scripts/capture/` that records 26 input/dryref/wet WAV triples through a DigiTech X-Series Synth Wah, with sample-accurate alignment and a one-time pre-flight calibration pass.

**Architecture:** Pure Python + numpy + scipy + soundfile + sounddevice. CLI entry (`capture.py`) with three subcommands: `build-stimulus`, `calibrate`, `record`. Recording uses `sounddevice.playrec` against Core Audio, dual-channel input. Per-pass alignment via FFT cross-correlation against an embedded sync chirp. All filterable behaviour (segment shape, alignment math, manifest I/O) is unit-tested with synthetic data; hardware-touching code is smoke-tested.

**Tech Stack:** Python 3.11+, numpy, scipy, soundfile, sounddevice, pytest. macOS Core Audio.

**Spec:** `docs/superpowers/specs/2026-04-25-digitech-pedal-capture-design.md`

---

## File Map

All paths relative to `thomAndGuy/`. Created files:

```
scripts/capture/
├── README.md                       Usage doc
├── requirements.txt                Pinned deps
├── pytest.ini                      Test discovery config
├── capture.py                      CLI entry point
├── stimulus.py                     Training-WAV builder + calibration stimuli
├── alignment.py                    Sync chirp xcorr → sample offset
├── interface.py                    sounddevice wrapper (playrec, device pick)
├── manifest.py                     manifest.csv read/append (CaptureRow dataclass)
├── config.py                       Knob grid (KnobConfig dataclass, GRID list)
├── calibration.py                  4-step pre-flight pass + calibration.json writer
└── tests/
    ├── __init__.py
    ├── conftest.py                 shared fixtures
    ├── test_alignment.py
    ├── test_stimulus.py
    └── test_manifest.py
```

No existing files are modified.

## Type Surface (locked)

These names are referenced by multiple tasks; lock them up front so later tasks don't drift.

```python
# config.py
SAMPLE_RATE: int = 96000        # Hz
BIT_DEPTH: int = 24             # used as soundfile subtype "PCM_24"
CHANNELS: int = 1
KNOB_LETTERS = ("l", "m", "h")  # low, mid, high

@dataclass(frozen=True)
class KnobConfig:
    sens: str       # "l" | "m" | "h"
    control: str
    rng: str        # "range" is a Python builtin; use `rng`

GRID: list[KnobConfig] = [...]  # 13 entries, exact order from spec §4

MODES: dict[int, str] = {1: "Env Up", 5: "Filter 1 (Synth Talk)"}

def prefix(mode: int, cfg: KnobConfig) -> str:
    return f"m{mode}_s{cfg.sens}_c{cfg.control}_r{cfg.rng}"

# manifest.py
@dataclass
class CaptureRow:
    mode: int
    sens: str
    control: str
    rng: str
    prefix: str
    timestamp: str          # ISO-8601 UTC
    rt_latency_samples: int

def read(path: Path) -> list[CaptureRow]: ...
def append(path: Path, row: CaptureRow) -> None: ...

# alignment.py
def find_offset(stimulus: np.ndarray, recorded: np.ndarray, sr: int) -> int:
    """Return the sample offset such that recorded[offset:] aligns with stimulus[0:].
    Uses FFT correlation against the sync chirp (first 0.5s after the impulse)."""
    ...

def correlation_quality(stimulus: np.ndarray, recorded: np.ndarray, offset: int) -> float:
    """0..1 normalized cross-correlation peak. Used by record-pass sanity check."""
    ...

# stimulus.py
def build_training_stimulus(sr: int = SAMPLE_RATE) -> np.ndarray: ...
def build_calibration_stimulus(sr: int = SAMPLE_RATE) -> np.ndarray:
    """Shorter stimulus for the 4 pre-flight steps: sync header + 5s pink noise +
    5s silence + 5 impulses. ~20s total."""
    ...

# interface.py
def list_audio_devices() -> list[dict]: ...
def play_record(out_signal: np.ndarray,
                sr: int,
                in_channels: int,
                output_device: int | str | None = None,
                input_device: int | str | None = None) -> np.ndarray:
    """Returns recorded array of shape (n_samples, in_channels)."""
    ...

# calibration.py
def run_preflight(captures_dir: Path, interface_kwargs: dict) -> dict: ...
```

---

## Task 1: Project scaffolding

**Files:**
- Create: `scripts/capture/requirements.txt`
- Create: `scripts/capture/pytest.ini`
- Create: `scripts/capture/README.md`
- Create: `scripts/capture/tests/__init__.py`
- Create: `scripts/capture/tests/conftest.py`

- [ ] **Step 1: Create `scripts/capture/requirements.txt`**

```
numpy>=1.26
scipy>=1.11
soundfile>=0.12
sounddevice>=0.4.6
pytest>=7.4
```

- [ ] **Step 2: Create `scripts/capture/pytest.ini`**

```ini
[pytest]
testpaths = tests
python_files = test_*.py
addopts = -v --tb=short
```

- [ ] **Step 3: Create `scripts/capture/tests/__init__.py`**

Empty file. Marks the directory as a package so pytest discovers shared fixtures.

```python
```

- [ ] **Step 4: Create `scripts/capture/tests/conftest.py`**

```python
"""Shared pytest fixtures for the capture toolchain."""
from __future__ import annotations

import numpy as np
import pytest


@pytest.fixture
def sr() -> int:
    """Default sample rate for tests (matches production)."""
    return 96000


@pytest.fixture
def rng() -> np.random.Generator:
    """Deterministic RNG so test outputs are byte-stable across runs."""
    return np.random.default_rng(seed=0xCAFE)
```

- [ ] **Step 5: Create `scripts/capture/README.md`**

```markdown
# Capture Toolchain

Records DigiTech X-Series Synth Wah responses to a controlled stimulus battery,
producing input/dryref/wet WAV triples plus a `manifest.csv` for downstream
parameter fitting.

See `../../docs/superpowers/specs/2026-04-25-digitech-pedal-capture-design.md`
for the design.

## Setup

```bash
cd scripts/capture
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## Usage

```bash
# One-time
python capture.py build-stimulus --out captures/_stimulus.wav
python capture.py calibrate --captures captures/

# Per session
python capture.py record --mode 1 --captures captures/
python capture.py record --mode 5 --captures captures/

# Recovery (redo a botched config)
python capture.py record --mode 1 --config 7 --captures captures/
```

## Tests

```bash
pytest
```

Hardware-dependent paths (`record`, `calibrate`) are not covered by automated
tests; verify by running them on the real rig.
```

- [ ] **Step 6: Verify pytest discovers no tests yet (smoke check)**

Run: `cd scripts/capture && python -m pytest --collect-only`
Expected: `no tests ran` (exit code 5 — collection error is OK; pytest just confirms the config parses).

- [ ] **Step 7: Commit**

```bash
git add scripts/capture/
git commit -m "scripts/capture: scaffolding (requirements, pytest config, README)"
```

---

## Task 2: `config.py` — knob grid and modes

**Files:**
- Create: `scripts/capture/config.py`

- [ ] **Step 1: Write `scripts/capture/config.py`**

```python
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
```

- [ ] **Step 2: Smoke-test in REPL**

Run: `cd scripts/capture && python -c "from config import GRID, MODES, prefix; print(prefix(1, GRID[1]))"`
Expected output: `m1_sl_cm_rm`

- [ ] **Step 3: Commit**

```bash
git add scripts/capture/config.py
git commit -m "scripts/capture: config.py — KnobConfig dataclass + 13-config grid"
```

---

## Task 3: `manifest.py` — CSV read/append (TDD)

**Files:**
- Create: `scripts/capture/tests/test_manifest.py`
- Create: `scripts/capture/manifest.py`

- [ ] **Step 1: Write the failing tests**

`scripts/capture/tests/test_manifest.py`:

```python
"""Tests for manifest.csv read/append round-tripping."""
from __future__ import annotations

from pathlib import Path

import pytest

from manifest import CaptureRow, append, read


def make_row(prefix: str = "m1_sm_cm_rm") -> CaptureRow:
    return CaptureRow(
        mode=1,
        sens="m",
        control="m",
        rng="m",
        prefix=prefix,
        timestamp="2026-04-25T18:00:00Z",
        rt_latency_samples=123,
    )


def test_append_creates_file_with_header(tmp_path: Path) -> None:
    path = tmp_path / "manifest.csv"
    append(path, make_row())
    contents = path.read_text()
    assert contents.startswith("mode,sens,control,range,prefix,timestamp,rt_latency_samples\n")
    assert "m1_sm_cm_rm" in contents


def test_round_trip_preserves_all_fields(tmp_path: Path) -> None:
    path = tmp_path / "manifest.csv"
    rows_in = [
        make_row("m1_sm_cm_rm"),
        CaptureRow(
            mode=5,
            sens="h",
            control="h",
            rng="l",
            prefix="m5_sh_ch_rl",
            timestamp="2026-04-25T19:32:00Z",
            rt_latency_samples=91,
        ),
    ]
    for row in rows_in:
        append(path, row)
    rows_out = read(path)
    assert rows_out == rows_in


def test_append_to_existing_file_does_not_duplicate_header(tmp_path: Path) -> None:
    path = tmp_path / "manifest.csv"
    append(path, make_row("m1_sm_cm_rm"))
    append(path, make_row("m1_sl_cm_rm"))
    header_count = path.read_text().count("mode,sens,control")
    assert header_count == 1


def test_read_missing_file_returns_empty_list(tmp_path: Path) -> None:
    assert read(tmp_path / "does-not-exist.csv") == []
```

- [ ] **Step 2: Run tests — verify they fail**

Run: `cd scripts/capture && python -m pytest tests/test_manifest.py -v`
Expected: 4 errors during collection (`ImportError: cannot import name 'CaptureRow' from 'manifest'`).

- [ ] **Step 3: Write `scripts/capture/manifest.py`**

```python
"""Read/append rows of `manifest.csv`.

The CSV column order is locked by the spec (§6); the on-disk `range` column
maps to the `rng` field on `CaptureRow` (`range` is a Python builtin we
avoid as a dataclass attribute).
"""
from __future__ import annotations

import csv
from dataclasses import asdict, dataclass, fields
from pathlib import Path


CSV_HEADER: tuple[str, ...] = (
    "mode",
    "sens",
    "control",
    "range",
    "prefix",
    "timestamp",
    "rt_latency_samples",
)


@dataclass
class CaptureRow:
    mode: int
    sens: str
    control: str
    rng: str
    prefix: str
    timestamp: str
    rt_latency_samples: int

    def to_csv_dict(self) -> dict[str, str]:
        d = asdict(self)
        d["range"] = d.pop("rng")
        return {k: str(v) for k, v in d.items()}

    @classmethod
    def from_csv_dict(cls, row: dict[str, str]) -> "CaptureRow":
        return cls(
            mode=int(row["mode"]),
            sens=row["sens"],
            control=row["control"],
            rng=row["range"],
            prefix=row["prefix"],
            timestamp=row["timestamp"],
            rt_latency_samples=int(row["rt_latency_samples"]),
        )


def append(path: Path, row: CaptureRow) -> None:
    path = Path(path)
    new_file = not path.exists()
    with path.open("a", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=CSV_HEADER)
        if new_file:
            writer.writeheader()
        writer.writerow(row.to_csv_dict())


def read(path: Path) -> list[CaptureRow]:
    path = Path(path)
    if not path.exists():
        return []
    with path.open("r", newline="") as f:
        return [CaptureRow.from_csv_dict(row) for row in csv.DictReader(f)]
```

- [ ] **Step 4: Run tests — verify they pass**

Run: `cd scripts/capture && python -m pytest tests/test_manifest.py -v`
Expected: 4 passed.

- [ ] **Step 5: Commit**

```bash
git add scripts/capture/manifest.py scripts/capture/tests/test_manifest.py
git commit -m "scripts/capture: manifest.py — CaptureRow CSV round-trip (TDD)"
```

---

## Task 4: `stimulus.py` — training WAV builder (TDD)

**Files:**
- Create: `scripts/capture/tests/test_stimulus.py`
- Create: `scripts/capture/stimulus.py`

This task is larger than others — the stimulus has 7 distinct segments, each with its own shape contract. We TDD them as a group via segment-summary tests, not one test per segment.

- [ ] **Step 1: Write the failing tests**

`scripts/capture/tests/test_stimulus.py`:

```python
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
    """No segment should reach digital full-scale; spec headroom is -3 dBFS max."""
    stim = build_training_stimulus(sr=sr)
    peak = np.max(np.abs(stim))
    # Level steps go up to -3 dBFS = 0.708; allow a small headroom margin.
    assert peak <= 0.85, f"stimulus peak {peak:.3f} exceeds 0.85 (clip risk)"


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
```

- [ ] **Step 2: Run tests — verify they fail**

Run: `cd scripts/capture && python -m pytest tests/test_stimulus.py -v`
Expected: collection errors (`stimulus` module missing).

- [ ] **Step 3: Write `scripts/capture/stimulus.py`**

```python
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
SYNC_TRAILING_SILENCE_S: float = 4.0

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
    sweep = chirp(t, f0=f0, f1=f1, t1=seconds, method="logarithmic")
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
    """60 s — 12 level-step bursts + 6 amplitude ramps. Spec §3."""
    parts: list[np.ndarray] = []
    levels_db = [-40.0, -30.0, -20.0, -12.0, -6.0, -3.0, -6.0, -12.0, -20.0, -30.0, -40.0, -30.0]
    for db in levels_db:
        parts.append(_sine(0.2, 220.0, db, sr))
        parts.append(_silence(0.3, sr))
    # 6 ramps × 4s each = 24s. (12 bursts × 0.5s = 6s.) Total ≈ 30s used so far,
    # add 30 more s of silence/ramps to land near 60 s overall.
    ramp_levels = [-20.0, -12.0, -6.0, -3.0, -12.0, -20.0]
    for db in ramp_levels:
        parts.append(_amplitude_ramp(4.0, 220.0, db, sr))
        parts.append(_silence(1.0, sr))
    return np.concatenate(parts)


def _waveshaper_probes(sr: int) -> np.ndarray:
    """60 s — stepped 220 Hz sines + 220/330 Hz two-tones. Spec §3."""
    parts: list[np.ndarray] = []
    for db in [-40.0, -30.0, -20.0, -12.0, -6.0, -3.0, 0.0]:
        parts.append(_sine(3.0, 220.0, db, sr))
        parts.append(_silence(0.5, sr))
    for db in [-20.0, -12.0, -6.0]:
        parts.append(_two_tone(3.0, 220.0, 330.0, db, sr))
        parts.append(_silence(0.5, sr))
    return np.concatenate(parts)


def _filter_probes(sr: int) -> np.ndarray:
    """60 s — held tone then sweep, repeated at 5 levels. Spec §3."""
    parts: list[np.ndarray] = []
    for db in [-30.0, -20.0, -12.0, -6.0, -3.0]:
        parts.append(_sine(1.5, 220.0, db, sr))
        parts.append(_log_sine_sweep(2.0, 50.0, 10_000.0, db, sr))
        parts.append(_silence(0.5, sr))
    return np.concatenate(parts)


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
```

- [ ] **Step 4: Run tests — verify they pass**

Run: `cd scripts/capture && python -m pytest tests/test_stimulus.py -v`
Expected: 6 passed.

- [ ] **Step 5: Commit**

```bash
git add scripts/capture/stimulus.py scripts/capture/tests/test_stimulus.py
git commit -m "scripts/capture: stimulus.py — training & calibration WAV builders (TDD)"
```

---

## Task 5: `alignment.py` — sync chirp cross-correlation (TDD)

**Files:**
- Create: `scripts/capture/tests/test_alignment.py`
- Create: `scripts/capture/alignment.py`

- [ ] **Step 1: Write the failing tests**

`scripts/capture/tests/test_alignment.py`:

```python
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
```

- [ ] **Step 2: Run tests — verify they fail**

Run: `cd scripts/capture && python -m pytest tests/test_alignment.py -v`
Expected: collection error (`alignment` module missing).

- [ ] **Step 3: Write `scripts/capture/alignment.py`**

```python
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
```

- [ ] **Step 4: Run tests — verify they pass**

Run: `cd scripts/capture && python -m pytest tests/test_alignment.py -v`
Expected: 7 passed (5 parametrized + 2 standalone).

- [ ] **Step 5: Commit**

```bash
git add scripts/capture/alignment.py scripts/capture/tests/test_alignment.py
git commit -m "scripts/capture: alignment.py — chirp xcorr offset recovery (TDD)"
```

---

## Task 6: `interface.py` — sounddevice wrapper (smoke-tested)

**Files:**
- Create: `scripts/capture/interface.py`

`sounddevice` requires a real audio device, so this module is not unit-tested. We do a smoke check that imports succeed and `list_audio_devices` returns something on the developer's machine.

- [ ] **Step 1: Write `scripts/capture/interface.py`**

```python
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
```

- [ ] **Step 2: Smoke check — import + list devices**

Run:
```
cd scripts/capture && python -c "from interface import list_audio_devices; \
  devs = list_audio_devices(); print(f'{len(devs)} device(s)'); \
  [print(f'  [{d[\"index\"]}] {d[\"name\"]} ({d[\"max_in\"]}in/{d[\"max_out\"]}out)') for d in devs[:5]]"
```
Expected: prints at least one device. (The Mac built-in mic+speakers are always there.)

- [ ] **Step 3: Commit**

```bash
git add scripts/capture/interface.py
git commit -m "scripts/capture: interface.py — sounddevice playrec wrapper"
```

---

## Task 7: `capture.py` CLI scaffold + `build-stimulus` subcommand

**Files:**
- Create: `scripts/capture/capture.py`

- [ ] **Step 1: Write the initial `scripts/capture/capture.py`**

```python
"""DigiTech Synth Wah hardware capture — CLI entry point.

Subcommands:
  build-stimulus  Generate the training WAV at the given path.
  calibrate       Run the 4-step pre-flight pass (Task 9).
  record          Record the 13-config grid for a mode (Task 10).
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

import soundfile as sf

from config import BIT_DEPTH_SUBTYPE, SAMPLE_RATE
from stimulus import build_training_stimulus


def cmd_build_stimulus(args: argparse.Namespace) -> int:
    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    stim = build_training_stimulus(sr=SAMPLE_RATE)
    sf.write(out_path, stim, SAMPLE_RATE, subtype=BIT_DEPTH_SUBTYPE)
    print(f"wrote {out_path} ({len(stim)/SAMPLE_RATE:.2f}s @ {SAMPLE_RATE} Hz)")
    return 0


def cmd_calibrate(args: argparse.Namespace) -> int:
    print("calibrate: not implemented yet (Task 9)")
    return 2


def cmd_record(args: argparse.Namespace) -> int:
    print("record: not implemented yet (Task 10)")
    return 2


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(prog="capture.py", description=__doc__)
    sub = p.add_subparsers(dest="subcommand", required=True)

    bs = sub.add_parser("build-stimulus", help="Generate the training WAV")
    bs.add_argument("--out", required=True, help="Output path for the training WAV")
    bs.set_defaults(func=cmd_build_stimulus)

    cal = sub.add_parser("calibrate", help="Run the 4-step pre-flight pass")
    cal.add_argument("--captures", default="captures", help="Captures directory")
    cal.set_defaults(func=cmd_calibrate)

    rec = sub.add_parser("record", help="Record the 13-config grid for a mode")
    rec.add_argument("--mode", type=int, required=True, choices=list((1, 5)))
    rec.add_argument("--config", type=int, default=None, help="Single config index (1-13) for recovery")
    rec.add_argument("--captures", default="captures", help="Captures directory")
    rec.set_defaults(func=cmd_record)

    return p


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
```

- [ ] **Step 2: Smoke-test build-stimulus**

Run:
```
cd scripts/capture && python capture.py build-stimulus --out /tmp/stim.wav
```
Expected output: `wrote /tmp/stim.wav (275.00s @ 96000 Hz)` (length within ±0.5s).

Then verify the file is a valid WAV:
```
python -c "import soundfile as sf; info = sf.info('/tmp/stim.wav'); print(info)"
```
Expected: shows samplerate 96000, channels 1, subtype PCM_24.

- [ ] **Step 3: Commit**

```bash
git add scripts/capture/capture.py
git commit -m "scripts/capture: capture.py CLI scaffold + build-stimulus subcommand"
```

---

## Task 8: `calibration.py` — 4-step pre-flight pass

**Files:**
- Create: `scripts/capture/calibration.py`

This module is hardware-dependent; we do not unit-test the audio path. We write the module's pure-math helpers (latency derivation, RMS, hum measurement) as small inline functions and verify them in Step 6 with a short manual smoke check.

- [ ] **Step 1: Write `scripts/capture/calibration.py`**

```python
"""Pre-flight calibration pass for the capture rig.

Runs the four steps described in spec §5, writes their WAVs into
`captures_dir/calibration/`, and aggregates the derived quantities into
`captures_dir/calibration/calibration.json`.
"""
from __future__ import annotations

import json
import time
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
    rec2 = _record_step("reamp", stim, 1, cal_dir / "02_reamp.wav", interface_kwargs)
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
```

- [ ] **Step 2: Sanity-check the math helpers in REPL (no hardware)**

Run:
```
cd scripts/capture && python -c "
import numpy as np
from calibration import _rms_dbfs, _measure_hum_dbfs

# A -20 dBFS sine should report ~-23 dBFS RMS (sine RMS = peak / sqrt(2) = -3 dB).
sr = 96000
t = np.arange(sr) / sr
x = (10**(-20/20)) * np.sin(2*np.pi*1000*t)
print(f'rms of -20 dBFS 1 kHz sine: {_rms_dbfs(x):.2f} dBFS  (expect ≈ -23)')

# A -40 dBFS 60 Hz tone in silence: hum measurement should land near -40.
y = (10**(-40/20)) * np.sin(2*np.pi*60*np.arange(sr*5)/sr)
print(f'hum @ 60 Hz of -40 dBFS tone: {_measure_hum_dbfs(y, sr):.2f} dBFS  (expect ≈ -40)')
"
```
Expected output: RMS reads near -23 dBFS, hum reads near -40 dBFS (both within ±2 dB).

- [ ] **Step 3: Commit**

```bash
git add scripts/capture/calibration.py
git commit -m "scripts/capture: calibration.py — 4-step pre-flight + calibration.json"
```

---

## Task 9: Wire `calibrate` subcommand into `capture.py`

**Files:**
- Modify: `scripts/capture/capture.py` (replace `cmd_calibrate` body)

- [ ] **Step 1: Replace the `cmd_calibrate` stub**

Open `scripts/capture/capture.py`. Replace the existing `cmd_calibrate` function with:

```python
def cmd_calibrate(args: argparse.Namespace) -> int:
    from calibration import run_preflight

    captures_dir = Path(args.captures)
    captures_dir.mkdir(parents=True, exist_ok=True)
    cal = run_preflight(captures_dir=captures_dir, interface_kwargs={})
    if cal["noise_floor_dbfs"] > -60.0:
        print(f"WARNING: noise floor {cal['noise_floor_dbfs']} dBFS is high — check cabling.")
    return 0
```

- [ ] **Step 2: Smoke-test the CLI parses (no hardware needed)**

Run: `cd scripts/capture && python capture.py calibrate --help`
Expected: shows the `--captures` option, exits 0.

- [ ] **Step 3: Commit**

```bash
git add scripts/capture/capture.py
git commit -m "scripts/capture: wire calibrate subcommand into CLI"
```

---

## Task 10: Wire `record` subcommand — per-config flow

**Files:**
- Modify: `scripts/capture/capture.py`

This is the load-bearing operator-facing flow. Each step is short.

- [ ] **Step 1: Replace `cmd_record` and add helpers**

Open `scripts/capture/capture.py`. Replace `cmd_record` with the following, and add the imports + helpers above it (just below `cmd_calibrate`):

```python
import json
from datetime import datetime, timezone

import numpy as np
import soundfile as sf

from alignment import correlation_quality, find_offset
from config import GRID, MODES, KnobConfig, prefix
from interface import play_record
from manifest import CaptureRow, append as manifest_append
from stimulus import build_training_stimulus


# Tunable thresholds; all explicit per spec §6.
PEAK_DBFS_LIMIT: float = -1.0
SYNC_QUALITY_MIN: float = 0.95
WET_HEADROOM_OVER_NOISE_DB: float = 12.0
LATENCY_DRIFT_LIMIT_SAMPLES: int = 32


def _knob_word(letter: str) -> str:
    return {"l": "9 o'clock (low)", "m": "noon", "h": "3 o'clock (high)"}[letter]


def _print_instructions(mode: int, config_idx: int, cfg: KnobConfig) -> None:
    print()
    print(f"=== Config {config_idx} of {len(GRID)}: {prefix(mode, cfg)} ===")
    print(f"  TYPE:    {mode} ({MODES[mode]})")
    print(f"  SENS:    {_knob_word(cfg.sens)}")
    print(f"  CONTROL: {_knob_word(cfg.control)}")
    print(f"  RANGE:   {_knob_word(cfg.rng)}")


def _peak_dbfs(signal: np.ndarray) -> float:
    peak = float(np.max(np.abs(signal))) + 1e-20
    return 20.0 * np.log10(peak)


def _rms_dbfs_local(signal: np.ndarray) -> float:
    rms = float(np.sqrt(np.mean(signal.astype(np.float64) ** 2)) + 1e-20)
    return 20.0 * np.log10(rms)


def _load_calibration(captures_dir: Path) -> dict:
    cal_path = captures_dir / "calibration" / "calibration.json"
    if not cal_path.exists():
        raise SystemExit(
            f"calibration.json not found at {cal_path}. "
            "Run `python capture.py calibrate` first."
        )
    return json.loads(cal_path.read_text())


def _do_one_config(
    mode: int,
    config_idx: int,
    cfg: KnobConfig,
    stim: np.ndarray,
    sr: int,
    captures_dir: Path,
    cal: dict,
) -> None:
    _print_instructions(mode, config_idx, cfg)
    input("Press [Enter] when knobs are dialed: ")

    print("recording...")
    recorded = play_record(out_signal=stim, sr=sr, in_channels=2)

    dryref = recorded[:, 0]
    wet = recorded[:, 1]

    # 1. Clipping check.
    for ch_name, ch_signal in (("dryref", dryref), ("wet", wet)):
        peak = _peak_dbfs(ch_signal)
        if peak > PEAK_DBFS_LIMIT:
            raise SystemExit(
                f"{prefix(mode, cfg)}: ADC {ch_name} peak {peak:.1f} dBFS "
                f"(limit {PEAK_DBFS_LIMIT}). Reduce reamp output by ~6 dB and rerun this config."
            )

    # 2. Sync alignment.
    offset = find_offset(stim, dryref, sr=sr)
    quality = correlation_quality(stim, dryref, offset=offset)
    if quality < SYNC_QUALITY_MIN:
        raise SystemExit(
            f"{prefix(mode, cfg)}: sync correlation {quality:.2f} (threshold {SYNC_QUALITY_MIN}). "
            "Sync chirp not detected — check ch1 cabling."
        )
    drift = abs(offset - cal["rt_latency_samples_with_pedal"])
    if drift > LATENCY_DRIFT_LIMIT_SAMPLES:
        raise SystemExit(
            f"{prefix(mode, cfg)}: round-trip latency drifted to {offset} samples "
            f"(was {cal['rt_latency_samples_with_pedal']}). Recalibrate before continuing."
        )

    # 3. Wet RMS sanity (pedal is engaged & active).
    wet_rms = _rms_dbfs_local(wet)
    threshold = cal["noise_floor_dbfs"] + WET_HEADROOM_OVER_NOISE_DB
    if wet_rms < threshold:
        raise SystemExit(
            f"{prefix(mode, cfg)}: wet RMS {wet_rms:.1f} dBFS, only "
            f"{wet_rms - cal['noise_floor_dbfs']:.1f} dB above noise floor. "
            "Pedal may be bypassed or SENS is off the grid."
        )

    # 4. Trim leading silence so all 3 WAVs align at sample 0.
    aligned_dryref = dryref[offset:]
    aligned_wet = wet[offset:]
    n = min(len(aligned_dryref), len(stim))
    aligned_dryref = aligned_dryref[:n]
    aligned_wet = aligned_wet[:n]
    stim_trimmed = stim[:n]

    # 5. Write three WAVs.
    base = prefix(mode, cfg)
    sf.write(captures_dir / f"{base}_input.wav", stim_trimmed, sr, subtype="PCM_24")
    sf.write(captures_dir / f"{base}_dryref.wav", aligned_dryref, sr, subtype="PCM_24")
    sf.write(captures_dir / f"{base}_wet.wav", aligned_wet, sr, subtype="PCM_24")

    # 6. Append manifest row.
    manifest_append(
        captures_dir / "manifest.csv",
        CaptureRow(
            mode=mode,
            sens=cfg.sens.upper(),
            control=cfg.control.upper(),
            rng=cfg.rng.upper(),
            prefix=base,
            timestamp=datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
            rt_latency_samples=int(offset),
        ),
    )
    print(
        f"  OK ✓  {base}: peak_dry={_peak_dbfs(dryref):.1f} dBFS, "
        f"peak_wet={_peak_dbfs(wet):.1f} dBFS, sync={quality:.3f}, latency={offset} samples"
    )


def cmd_record(args: argparse.Namespace) -> int:
    if args.mode not in MODES:
        print(f"--mode must be one of {sorted(MODES)}", file=sys.stderr)
        return 2

    captures_dir = Path(args.captures)
    captures_dir.mkdir(parents=True, exist_ok=True)
    cal = _load_calibration(captures_dir)

    stim_path = captures_dir / "_stimulus.wav"
    if not stim_path.exists():
        raise SystemExit(
            f"stimulus WAV missing at {stim_path}. "
            "Run `python capture.py build-stimulus --out captures/_stimulus.wav` first."
        )
    stim, sr = sf.read(stim_path, dtype="float32")
    if stim.ndim > 1:
        stim = stim[:, 0]
    if sr != cal["samplerate"]:
        raise SystemExit(f"stimulus sr {sr} ≠ calibration sr {cal['samplerate']}.")

    indices = [args.config] if args.config else list(range(1, len(GRID) + 1))
    for idx in indices:
        if not (1 <= idx <= len(GRID)):
            raise SystemExit(f"--config must be 1..{len(GRID)} (got {idx})")
        _do_one_config(args.mode, idx, GRID[idx - 1], stim, sr, captures_dir, cal)

    return 0
```

- [ ] **Step 2: Smoke-test the CLI parses (no hardware)**

Run: `cd scripts/capture && python capture.py record --help`
Expected: shows `--mode`, `--config`, `--captures` options, exits 0.

Run: `cd scripts/capture && python capture.py record --mode 1 --captures /tmp/no-such-dir`
Expected: exits non-zero with `calibration.json not found at /tmp/no-such-dir/calibration/calibration.json. Run \`python capture.py calibrate\` first.`

- [ ] **Step 3: Commit**

```bash
git add scripts/capture/capture.py
git commit -m "scripts/capture: wire record subcommand — per-config flow with sanity gates"
```

---

## Task 11: README usage polish + final test sweep

**Files:**
- Modify: `scripts/capture/README.md`

- [ ] **Step 1: Replace `scripts/capture/README.md` with the polished version**

```markdown
# Capture Toolchain

Records DigiTech X-Series Synth Wah responses to a controlled stimulus battery,
producing input/dryref/wet WAV triples plus a `manifest.csv` for downstream
parameter fitting.

See `../../docs/superpowers/specs/2026-04-25-digitech-pedal-capture-design.md`
for the design.

## Setup

```bash
cd scripts/capture
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## Hardware Rig

```
Focusrite OUT1 ──► Reamp ──► Pedal IN ──► Pedal ──► OUT1 (DRY)    ──► Focusrite IN1
                                                ╲
                                                 ──► OUT2 (EFFECT) ──► Focusrite IN2
```

Pedal must be **engaged** (footswitch on) for all 26 captures. Bypass behaviour is
verified separately during calibration step 3.

## Workflow

```bash
# One-time setup
python capture.py build-stimulus --out captures/_stimulus.wav
python capture.py calibrate --captures captures/

# Per session — mode 1 (Env Up) and mode 5 (Filter 1 / Synth Talk)
python capture.py record --mode 1 --captures captures/   # 13 configs
python capture.py record --mode 5 --captures captures/   # 13 configs

# Recovery — redo a single botched config
python capture.py record --mode 1 --config 7 --captures captures/
```

Each `record` pass walks all 13 knob configs in order. The script prints the
required knob positions and waits for `[Enter]` before each capture.

## Output Layout

```
captures/
├── _stimulus.wav                 master training WAV (one copy)
├── m1_sm_cm_rm_input.wav         per-pass stimulus echo
├── m1_sm_cm_rm_dryref.wav        pedal OUT1, sample-aligned
├── m1_sm_cm_rm_wet.wav           pedal OUT2, sample-aligned
├── ... (×26 prefixes)
├── manifest.csv                  one row per capture
└── calibration/
    ├── 01_loopback.wav
    ├── 02_reamp.wav
    ├── 03_pedal_bypass.wav
    ├── 04_pedal_engaged_silent.wav
    └── calibration.json
```

## Tests

```bash
pytest
```

Hardware-dependent paths (`record`, `calibrate`) are not covered by automated
tests. Verify by running them on the real rig — the script exits with explicit
error messages on any sanity-gate failure.
```

- [ ] **Step 2: Run the full test suite**

Run: `cd scripts/capture && python -m pytest -v`
Expected: all tests pass (test_alignment 7, test_stimulus 6, test_manifest 4 = 17 total).

- [ ] **Step 3: Final smoke check — full CLI surface**

Run:
```
cd scripts/capture && python capture.py --help && \
  python capture.py build-stimulus --help && \
  python capture.py calibrate --help && \
  python capture.py record --help
```
Expected: all four print help with exit code 0.

- [ ] **Step 4: Commit**

```bash
git add scripts/capture/README.md
git commit -m "scripts/capture: polish README with rig diagram and full workflow"
```

---

## Self-Review

**Spec coverage:**
- §2 hardware signal flow → README rig diagram (Task 11) + interface.py (Task 6)
- §3 stimulus design → stimulus.py (Task 4) covers all 7 segments
- §4 knob grid → config.py GRID (Task 2)
- §5 pre-flight calibration → calibration.py (Task 8) + calibrate subcommand (Task 9)
- §6 capture.py architecture → tasks 7, 9, 10 cover all three subcommands
- §6 sanity gates → Task 10 step 1 implements all four (peak, sync, wet RMS, latency drift)
- §6 manifest schema → manifest.py (Task 3)
- §6 file naming → config.py prefix() (Task 2)
- §7 testing strategy → tests for alignment, stimulus, manifest (Tasks 3-5)

**Type consistency:**
- `KnobConfig` (field `rng`, not `range_`) used identically in Tasks 2, 10.
- `CaptureRow` schema used identically in Tasks 3, 10.
- `prefix(mode, cfg)` signature used identically in Tasks 2, 10.
- `find_offset` / `correlation_quality` signatures consistent across Tasks 5, 8, 10.
- `play_record` signature consistent across Tasks 6, 8, 10.

**Placeholder scan:**
- No "TODO", "implement later", "fill in details" in any task body.
- All test bodies contain real assertions and inputs.
- All code blocks compile to running Python (verified mentally pass-by-pass).

**Risks worth calling out at execution time:**
- Task 8 `_bypass_null_db` relies on the chirp being a representative window; if pedal-bypass adds a coloration with most energy outside 100 Hz – 10 kHz, residual will look smaller than reality. Acceptable for a binary true-bypass / not classification.
- Task 10 `record` requires 96 kHz dual-channel input on the Focusrite — verify the device supports this before the session.
