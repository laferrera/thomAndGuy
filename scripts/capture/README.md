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
