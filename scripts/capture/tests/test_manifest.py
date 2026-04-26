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
