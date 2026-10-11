#!/usr/bin/env python3
"""Independently audit artifacts of an explicitly executed frozen 500M-ray run.

Usage: python tests/scientific/audit_500m_benchmark.py build/a0-full/m5_full_<id>

This tool never launches the ray tracer. Passing does not establish run provenance;
retain the executable/commit identity and original console log separately.
"""

import argparse
import csv
import hashlib
import json
import math
import re
import struct
import sys
from pathlib import Path

from verify_reference_v2 import verify as verify_frozen_reference


RAYS = 500_000_000
SEED = 123456789
CHUNK_SIZE = 10000
CHUNK_COUNT = 50000
GRID_WIDTH = 100
GRID_HEIGHT = 100
GRID_CELLS = GRID_WIDTH * GRID_HEIGHT
BOUNDS = {"x_min": -2, "x_max": 2, "y_min": -2, "y_max": 2}
GRID = {"width": GRID_WIDTH, "height": GRID_HEIGHT}
# The reference JSON specifies no overrides, so keep the production defaults.
TOTAL_POWER_TOLERANCE_PERCENT = 0.1
MAXIMUM_FLUX_TOLERANCE_PERCENT = 5.0


class AuditError(ValueError):
    """The run does not demonstrate acceptance against the frozen reference."""


def require(condition, message):
    if not condition:
        raise AuditError(message)


def read_object(path):
    data = json.loads(path.read_text(encoding="utf-8"))
    require(type(data) is dict, f"{path}: expected a JSON object")
    return data


def identity(data, name, expected):
    actual = data.get(name)
    require(type(actual) is type(expected) and actual == expected,
            f"{name}: expected {expected!r}, found {actual!r}")


def exact_integer(data, name, expected):
    value = data.get(name)
    require(type(value) in (int, float) and math.isfinite(value) and value == expected,
            f"{name}: expected integer {expected}, found {value!r}")


def number(data, name, minimum=0.0, positive=False):
    value = data.get(name)
    require(type(value) in (int, float) and math.isfinite(value),
            f"{name}: missing or non-finite numeric value")
    require(value > minimum if positive else value >= minimum,
            f"{name}: out-of-range numeric value {value!r}")
    return float(value)


def source_path(value, expected, field):
    require(type(value) is str and bool(value), f"{field}: missing path")
    require(Path(value).resolve(strict=True) == expected.resolve(strict=True),
            f"{field}: path does not identify the pinned input/output file")


def approximate(actual, expected, field, rel_tol=1e-11, abs_tol=1e-11):
    require(math.isclose(actual, expected, rel_tol=rel_tol, abs_tol=abs_tol),
            f"{field}: {actual!r} is inconsistent with grid/reference {expected!r}")


def flux_values_and_csv(binary_path, csv_path):
    binary = binary_path.read_bytes()
    require(len(binary) == GRID_CELLS * 8,
            f"Generated binary grid is {len(binary)} bytes, expected {GRID_CELLS * 8}")
    values = struct.unpack(f"<{GRID_CELLS}d", binary)
    require(all(math.isfinite(v) and v >= 0 for v in values),
            "Generated grid contains non-finite or negative flux")
    row_count = 0
    with csv_path.open(encoding="utf-8-sig", newline="") as handle:
        for row_count, row in enumerate(csv.reader(handle), 1):
            require(row_count <= GRID_HEIGHT and len(row) == GRID_WIDTH,
                    f"Generated CSV has wrong dimensions at row {row_count}")
            for col, text in enumerate(row):
                try:
                    encoded = struct.pack("<d", float(text))
                except (ValueError, OverflowError) as exc:
                    raise AuditError(f"Generated CSV invalid at ({row_count}, {col+1})") from exc
                offset = ((row_count - 1) * GRID_WIDTH + col) * 8
                require(encoded == binary[offset:offset + 8],
                        f"Generated CSV and binary disagree at ({row_count}, {col+1})")
    require(row_count == GRID_HEIGHT,
            f"Generated CSV has {row_count} rows, expected {GRID_HEIGHT}")
    return values, hashlib.sha256(binary).hexdigest()


def audit_run(run_dir, frozen_dir, reference):
    """Check generated data; caller must first verify frozen reference integrity.

    Kept separate from the frozen identity verification for synthetic unit tests.
    """
    run_dir = Path(run_dir).resolve(strict=True)
    frozen_dir = Path(frozen_dir).resolve(strict=True)
    require(re.fullmatch(r"m5_full_[0-9a-f]{10}", run_dir.name) is not None,
            "Expected an explicit M5 MODE=full output directory (m5_full_<10 hex>)")
    require(run_dir != frozen_dir and frozen_dir not in run_dir.parents,
            "Full benchmark outputs must not overwrite frozen examples")
    paths = {name: (run_dir / filename).resolve(strict=True)
             for name, filename in {
                 "config": "config_1.json",
                 "result": "result_1.json",
                 "binary": "grid_1.bin",
                 "csv": "grid_1.csv",
                 "stdout": "stdout_1.log",
             }.items()}
    require(len(set(paths.values())) == 5, "Generated run artifacts must be distinct files")
    require(all(path.parent == run_dir for path in paths.values()),
            "Generated output paths must remain in the independent run directory")
    config = read_object(paths["config"])
    result = read_object(paths["result"])
    stdout_lines = paths["stdout"].read_text(encoding="utf-8").splitlines()
    for expected_line in (
        "benchmark: benchmark_v1", "rays: 500000000", "seed: 123456789",
        "photon_export: false", "export_path: none",
        "Benchmark completed.", "rays_traced: 500000000",
        "chunk_count: 50000", "chunk_size: 10000",
        "benchmark_pass: true", "flux_grid_hash_matches: true",
    ):
        require(expected_line in stdout_lines,
                f"Native benchmark stdout log lacks required line {expected_line!r}")

    identity(config, "benchmark", "benchmark_v1")
    identity(config, "rays", RAYS)
    identity(config, "seed", SEED)
    identity(config, "target_side_id", 1)
    identity(config, "target_grid", GRID)
    identity(config, "target_bounds", BOUNDS)
    identity(config, "photon_export", False)
    require("worker_count" not in config and "chunk_size" not in config,
            "Nonstandard worker/chunk overrides are prohibited")
    for field, expected in (
        ("scene_file", frozen_dir / "benchmark_heliostat_field_v1.tnhpp"),
        ("reference_file", frozen_dir / "benchmark_reference_500M_v2.json"),
        ("output_file", paths["result"]),
        ("flux_grid_binary_output_file", paths["binary"]),
        ("flux_grid_output_file", paths["csv"]),
    ):
        source_path(config.get(field), expected, f"config.{field}")
    require("reference_flux_grid_file" not in config and
            "reference_flux_grid_binary_file" not in config,
            "Do not override the frozen reference grid from this run's config")

    for field, value in (
        ("schema_version", 1), ("rays", RAYS), ("seed", SEED),
        ("target_side_id", 1), ("chunk_count", CHUNK_COUNT),
        ("chunk_size", CHUNK_SIZE),
    ):
        exact_integer(result, field, value)
    for field, value in (("benchmark", "benchmark_v1"),
                         ("target_grid", GRID), ("target_bounds", BOUNDS)):
        identity(result, field, value)
    source_path(result.get("scene_file"), frozen_dir / "benchmark_heliostat_field_v1.tnhpp",
                "result.scene_file")
    for field, key in (("flux_grid_output_file", "csv"),
                       ("flux_grid_binary_output_file", "binary")):
        source_path(result.get(field), paths[key], f"result.{field}")
    source_path(result.get("reference_flux_grid_binary_file"),
                frozen_dir / "benchmark_reference_flux_grid_500M_v2.bin",
                "result.reference_flux_grid_binary_file")
    require(type(result.get("worker_count")) is int and result["worker_count"] > 0,
            "Invalid worker_count metadata")
    number(result, "elapsed_seconds", positive=True)
    number(result, "rays_per_second", positive=True)

    for flag in ("benchmark_pass", "total_power_pass", "maximum_flux_pass",
                 "flux_grid_hash_matches", "flux_grid_hash_pass"):
        require(result.get(flag) is True, f"{flag}: must be a real boolean true")

    values, actual_hash = flux_values_and_csv(paths["binary"], paths["csv"])
    expected_hash = reference["flux_grid_sha256"]
    require(actual_hash == expected_hash,
            f"Computed 500M flux grid SHA-256 differs from frozen reference: {actual_hash}")
    require(result.get("flux_grid_sha256") == actual_hash,
            "Result grid hash differs from actual generated binary bytes")

    total_flux = math.fsum(values)
    cell_area = ((BOUNDS["x_max"] - BOUNDS["x_min"]) / GRID_WIDTH) * (
        (BOUNDS["y_max"] - BOUNDS["y_min"]) / GRID_HEIGHT)
    derived = {
        "minimum_flux_mw_m2": min(values),
        "average_flux_mw_m2": total_flux / GRID_CELLS,
        "maximum_flux_mw_m2": max(values),
        "total_power_mw": total_flux * cell_area,
    }
    for field, measured in derived.items():
        actual = number(result, field)
        approximate(actual, measured, f"result.{field}")
        approximate(actual, float(reference[field]), f"frozen.{field}")

    total_error = abs(result["total_power_mw"] - reference["total_power_mw"]) / reference["total_power_mw"] * 100
    max_error = abs(result["maximum_flux_mw_m2"] - reference["maximum_flux_mw_m2"]) / reference["maximum_flux_mw_m2"] * 100
    require(total_error <= TOTAL_POWER_TOLERANCE_PERCENT,
            "Total power exceeds the existing 0.1% tolerance")
    require(max_error <= MAXIMUM_FLUX_TOLERANCE_PERCENT,
            "Maximum flux exceeds the existing 5% tolerance")
    approximate(number(result, "total_power_error_percent"), total_error,
                "total_power_error_percent")
    approximate(number(result, "maximum_flux_error_percent"), max_error,
                "maximum_flux_error_percent")
    for field in ("maximum_flux_grid_absolute_error_mw_m2",
                  "maximum_flux_grid_relative_error_percent", "rms_flux_grid_error_mw_m2"):
        approximate(number(result, field), 0.0, field)

    return {"ray_count": RAYS, "flux_grid_sha256": actual_hash,
            "total_power_mw": result["total_power_mw"],
            "maximum_flux_mw_m2": result["maximum_flux_mw_m2"]}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run_dir", type=Path,
                        help="m5_full_<id> directory produced by MODE=full")
    args = parser.parse_args(argv)
    frozen_dir = Path(__file__).resolve().parents[2] / "examples" / "benchmarks"
    try:
        # This identity check uses the committed size, scene MD5, exact grid
        # SHA-256 and reference metrics; it does not accept run-provided values.
        verify_frozen_reference(frozen_dir)
        reference = read_object(frozen_dir / "benchmark_reference_500M_v2.json")
        summary = audit_run(args.run_dir, frozen_dir, reference)
    except (OSError, AuditError, KeyError, ValueError, json.JSONDecodeError,
            struct.error) as exc:
        print(f"FAIL: A0 frozen 500M acceptance: {exc}", file=sys.stderr)
        return 1
    print("PASS: independently audited 500,000,000-ray run artifacts; "
          f"exact frozen grid SHA-256 {summary['flux_grid_sha256']}; "
          f"power {summary['total_power_mw']} MW; "
          f"peak flux {summary['maximum_flux_mw_m2']} MW/m2")
    print("NOTE: An artifact audit does not establish execution provenance. "
          "Preserve native console log, clean commit ID and executable identity.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
