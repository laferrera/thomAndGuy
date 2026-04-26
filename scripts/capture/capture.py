"""DigiTech Synth Wah hardware capture — CLI entry point.

Subcommands:
  build-stimulus  Generate the training WAV at the given path.
  calibrate       Run the 4-step pre-flight pass.
  record          Record the 13-config grid for a mode.
"""
from __future__ import annotations

import argparse
import json
import sys
from datetime import datetime, timezone
from pathlib import Path

import numpy as np
import soundfile as sf

from alignment import correlation_quality, find_offset
from config import BIT_DEPTH_SUBTYPE, GRID, MODES, KnobConfig, SAMPLE_RATE, prefix
from interface import play_record
from manifest import CaptureRow, append as manifest_append
from stimulus import build_training_stimulus


# Tunable thresholds; all explicit per spec §6.
PEAK_DBFS_LIMIT: float = -1.0
SYNC_QUALITY_MIN: float = 0.95
WET_HEADROOM_OVER_NOISE_DB: float = 12.0
LATENCY_DRIFT_LIMIT_SAMPLES: int = 32


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
