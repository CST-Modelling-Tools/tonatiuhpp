#!/usr/bin/env python3
"""Check benchmark-v2 reference artifacts without running the 500M-ray trace."""

import csv
import hashlib
import json
import math
import struct
import sys
from pathlib import Path


IDENTITY = {
    "benchmark": "benchmark_v2",
    "schema_version": 1,
    "rays": 500000000,
    "seed": 123456789,
    "chunk_count": 50000,
    "chunk_size": 10000,
    "scene_file": "benchmark_heliostat_field_v1.tnhpp",
    "target_side_id": 1,
    "flux_grid_sha256": "36c7dc17cee8e7fdef820f08d4482e7c3dcdee3b5eed4748ea3949a6cf4af472",
    "total_power_mw": 42.217052166502086,
    "minimum_flux_mw_m2": 0.0807424687410667,
    "average_flux_mw_m2": 2.638565760406375,
    "maximum_flux_mw_m2": 19.52519778288914,
}


def verify(directory):
    reference = json.loads((directory / "benchmark_reference_500M_v2.json").read_text(encoding="utf-8"))
    if not isinstance(reference, dict):
        raise ValueError("Reference JSON is not an object")
    for field, value in IDENTITY.items():
        if type(reference.get(field)) is not type(value) or reference[field] != value:
            raise ValueError(f"Reference identity changed: {field}: {reference.get(field)!r}")
    grid = reference.get("target_grid")
    if grid != {"width": 100, "height": 100}:
        raise ValueError(f"Unexpected reference grid: {grid!r}")
    if reference.get("target_bounds") != {"x_min": -2, "x_max": 2, "y_min": -2, "y_max": 2}:
        raise ValueError("Unexpected reference bounds")
    expected_files = {
        "flux_grid_binary_file": "benchmark_reference_flux_grid_500M_v2.bin",
        "flux_grid_file": "benchmark_reference_flux_grid_500M_v2.csv",
    }
    for field, name in expected_files.items():
        if reference.get(field) != name:
            raise ValueError(f"Unexpected {field}: {reference.get(field)!r}")
    binary_data = (directory / expected_files["flux_grid_binary_file"]).read_bytes()
    count = grid["width"] * grid["height"]
    if len(binary_data) != count * 8:
        raise ValueError(f"Binary grid byte count: {len(binary_data)}, expected {count * 8}")
    actual_hash = hashlib.sha256(binary_data).hexdigest()
    if actual_hash != IDENTITY["flux_grid_sha256"]:
        raise ValueError(f"Reference binary SHA-256 mismatch: {actual_hash}")
    flux_values = struct.unpack(f"<{count}d", binary_data)
    if not all(math.isfinite(x) and x >= 0 for x in flux_values):
        raise ValueError("Nonfinite or negative binary flux grid value")

    row_count = 0
    with (directory / expected_files["flux_grid_file"]).open(encoding="utf-8-sig", newline="") as source:
        for row_count, row in enumerate(csv.reader(source), 1):
            if row_count > grid["height"] or len(row) != grid["width"]:
                raise ValueError(f"CSV grid dimensions differ on row {row_count}")
            for col, cell in enumerate(row):
                try:
                    csv_bytes = struct.pack("<d", float(cell))
                except ValueError as err:
                    raise ValueError(f"Invalid CSV float ({row_count}, {col + 1})") from err
                index = (row_count - 1) * grid["width"] + col
                if csv_bytes != binary_data[index * 8:(index + 1) * 8]:
                    raise ValueError(f"CSV and binary differ at ({row_count}, {col + 1})")
    if row_count != grid["height"]:
        raise ValueError(f"CSV grid height differs: {row_count}")

    calculated = {
        "minimum_flux_mw_m2": min(flux_values),
        "average_flux_mw_m2": math.fsum(flux_values) / count,
        "maximum_flux_mw_m2": max(flux_values),
    }
    for field, value in calculated.items():
        if not math.isclose(value, IDENTITY[field], rel_tol=1e-12, abs_tol=1e-12):
            raise ValueError(f"Reference summary differs: {field}: {value}")
    # The frozen scene is the exact input used for the published v2 reference.
    # MD5 here matches the Zenodo deposit; SHA-256 of the grid stays separate.
    scene_path = directory / IDENTITY["scene_file"]
    scene_data = scene_path.read_bytes()
    if len(scene_data) != 9413273:
        raise ValueError(f"Unexpected frozen scene size: {len(scene_data)}")
    scene_md5 = hashlib.md5(scene_data).hexdigest()
    if scene_md5 != "e42985c98b965f084a958226851ed626":
        raise ValueError(f"Frozen scene differs from Zenodo MD5: {scene_md5}")
    print(f"PASS: authentic benchmark scene; bytes={len(scene_data)}, Zenodo MD5={scene_md5}")
    print(f"PASS: benchmark-v2 reference is internally consistent; {count} cells; SHA-256 {actual_hash}")


if __name__ == "__main__":
    root = Path(__file__).resolve().parents[2] / "examples" / "benchmarks"
    try:
        verify(root)
    except (ValueError, OSError, json.JSONDecodeError, struct.error) as err:
        sys.exit(f"FAIL: benchmark-v2 reference integrity: {err}")
