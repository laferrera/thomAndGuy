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
