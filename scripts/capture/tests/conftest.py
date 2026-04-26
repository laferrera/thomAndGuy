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
