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
    from calibration import run_preflight

    captures_dir = Path(args.captures)
    captures_dir.mkdir(parents=True, exist_ok=True)
    cal = run_preflight(captures_dir=captures_dir, interface_kwargs={})
    if cal["noise_floor_dbfs"] > -60.0:
        print(f"WARNING: noise floor {cal['noise_floor_dbfs']} dBFS is high — check cabling.")
    return 0


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
