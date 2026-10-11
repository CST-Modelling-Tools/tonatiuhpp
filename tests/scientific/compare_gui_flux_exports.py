#!/usr/bin/env python3
"""Compare two independent GUI Flux Analysis .txt exports at their saved precision.

The legacy .txt exporter writes a rectangular, tab-separated flux matrix in
W/m2 (including a trailing tab per row). This checks saved files, not raw
unrounded floating-point grid values or a separate physical flux reference.
"""

import argparse
import hashlib
import math
from pathlib import Path


class FluxExportError(ValueError):
    """An export is malformed or reproducibility is not demonstrated."""


def load_flux_grid(path, rows, cols):
    path = Path(path)
    contents = path.read_bytes()
    try:
        decoded = contents.decode("utf-8-sig")
    except UnicodeDecodeError as exc:
        raise FluxExportError(f"{path}: export is not UTF-8") from exc
    lines = decoded.splitlines()
    if len(lines) != rows:
        raise FluxExportError(f"{path}: expected {rows} rows, found {len(lines)}")
    grid = []
    for row_number, line in enumerate(lines, 1):
        fields = line.split("\t")
        if fields[-1] == "":
            fields.pop()  # The native exporter writes a trailing tab.
        if len(fields) != cols:
            raise FluxExportError(
                f"{path}: row {row_number} expected {cols} columns, found {len(fields)}"
            )
        row = []
        for column_number, cell in enumerate(fields, 1):
            if not cell.strip():
                raise FluxExportError(
                    f"{path}: empty cell at ({row_number}, {column_number})"
                )
            try:
                value = float(cell)
            except ValueError as exc:
                raise FluxExportError(
                    f"{path}: invalid numeric cell at ({row_number}, {column_number})"
                ) from exc
            if not math.isfinite(value) or value < 0:
                raise FluxExportError(
                    f"{path}: non-finite or negative flux at ({row_number}, {column_number})"
                )
            row.append(value)
        grid.append(tuple(row))
    if not any(value > 0 for row in grid for value in row):
        raise FluxExportError(f"{path}: zero-only flux map cannot demonstrate receiver hits")
    return tuple(grid), contents


def compare_exports(first, second, rows, cols):
    if rows < 2 or cols < 2:
        raise FluxExportError("Grid dimensions must be >= 2 on each axis")
    first = Path(first)
    second = Path(second)
    if first.resolve() == second.resolve():
        raise FluxExportError("Expected two distinct GUI export paths")
    grid_a, bytes_a = load_flux_grid(first, rows, cols)
    grid_b, bytes_b = load_flux_grid(second, rows, cols)
    if grid_a != grid_b:
        for row in range(rows):
            for col in range(cols):
                if grid_a[row][col] != grid_b[row][col]:
                    raise FluxExportError(
                        f"Saved flux differs at ({row + 1}, {col + 1}): "
                        f"{grid_a[row][col]!r} vs {grid_b[row][col]!r}"
                    )
    if bytes_a != bytes_b:
        raise FluxExportError(
            "Numerical grids match but export bytes differ; exact saved-file "
            "reproducibility was not demonstrated"
        )
    values = [value for row in grid_a for value in row]
    return {
        "rows": rows,
        "cols": cols,
        "sha256": hashlib.sha256(bytes_a).hexdigest(),
        "positive_cells": sum(value > 0 for value in values),
        "minimum_flux_w_m2": min(values),
        "maximum_flux_w_m2": max(values),
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("first", type=Path, help="First independently generated .txt export")
    parser.add_argument("second", type=Path, help="Second independently generated .txt export")
    parser.add_argument("--rows", type=int, required=True, help="Flux Analysis Cells U")
    parser.add_argument("--cols", type=int, required=True, help="Flux Analysis Cells V")
    args = parser.parse_args(argv)
    try:
        report = compare_exports(args.first, args.second, args.rows, args.cols)
    except (OSError, FluxExportError) as exc:
        parser.exit(1, f"FAIL: GUI Flux Analysis export comparison: {exc}\n")
    print(
        f"PASS: identical {report['rows']}x{report['cols']} exported flux cells; "
        f"positive cells {report['positive_cells']}; "
        f"saved-file SHA-256 {report['sha256']}; "
        f"max saved flux {report['maximum_flux_w_m2']} W/m2"
    )
    print(
        "NOTE: This proves equality at native .txt export precision only. "
        "Verify the effective GUI seed and independent execution logs; "
        "this is not headless or benchmark flux equivalence."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
