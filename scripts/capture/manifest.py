"""Read/append rows of `manifest.csv`.

The CSV column order is locked by the spec (§6); the on-disk `range` column
maps to the `rng` field on `CaptureRow` (`range` is a Python builtin we
avoid as a dataclass attribute).
"""
from __future__ import annotations

import csv
from dataclasses import asdict, dataclass
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
