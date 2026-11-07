#!/usr/bin/env python3
"""Check genuine production File append, stable surfaces and cumulative power.

All photon records must be exactly twice the corresponding one-shot records
as a multiset of physical bits, surface URLs and sides, not ordered binary IDs.
Qt QTextStream serializes normalization with limited decimal precision.
"""
from __future__ import annotations

import argparse
from collections import Counter
import math
from pathlib import Path
import sys

# Uses the existing production A0 canonical parser, never a second format.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from compare_photon_exports import read_export  # noqa: E402


def verify(single, appended):
    single_records, single_power, single_count, single_surfaces = read_export(single)
    appended_records, appended_power, appended_count, appended_surfaces = read_export(appended)
    if not single_surfaces or not appended_surfaces:
        raise ValueError("No selected surface was written to the photon map")
    if len(single_surfaces) != len(appended_surfaces):
        raise ValueError("Photon surface IDs duplicated or changed during append")
    if appended_records != Counter({key: count * 2 for key, count in single_records.items()}):
        raise ValueError("Appended records do not equal two exact physical copies of one run")
    if appended_count != 2 * single_count:
        raise ValueError(f"Append photon count mismatch: {appended_count}/{single_count}")
    if not math.isclose(appended_power * 2., single_power, rel_tol=2e-5, abs_tol=1e-12):
        raise ValueError(f"Cumulative photon power differs: first {single_power}, appended {appended_power}")
    print(f"M3 PHASE 3B2B2A: REAL FILE APPEND PASSED: "
          f"{single_count} + {single_count} = {appended_count} photons; "
          f"power {single_power} -> {appended_power}; "
          f"surface IDs {len(single_surfaces)} -> {len(appended_surfaces)}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("single")
    parser.add_argument("appended")
    args = parser.parse_args()
    try:
        verify(args.single, args.appended)
    except (OSError, ValueError) as exc:
        print(f"Phase 3B2B2A append mismatch: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
