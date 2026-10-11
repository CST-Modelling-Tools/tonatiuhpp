#!/usr/bin/env python3
"""Compare real PhotonsFile outputs as physical record multisets, not file order."""
import argparse
from collections import Counter
from pathlib import Path
import struct
import sys


def read_export(folder):
    folder = Path(folder)
    metadata = (folder / "PhotonMap_parameters.txt").read_text(encoding="utf-8")
    lines = metadata.splitlines()
    if not lines or lines[0] != "START PARAMETERS":
        raise ValueError(f"{folder}: parameters section is absent")
    end_parameters = lines.index("END PARAMETERS")
    columns = lines[1:end_parameters]
    expected = ["id", "x", "y", "z", "side", "surface ID"]
    if columns != expected:
        raise ValueError(f"{folder}: unexpected photon column layout: {columns}")
    start_surfaces = lines.index("START SURFACES", end_parameters)
    end_surfaces = lines.index("END SURFACES", start_surfaces)
    surfaces = {}
    for line in lines[start_surfaces + 1:end_surfaces]:
        index, sep, url = line.partition(" ")
        if not sep or not index.isdecimal() or not url:
            raise ValueError(f"{folder}: invalid surface mapping")
        key = int(index)
        if key in surfaces:
            raise ValueError(f"{folder}: duplicate surface identifier")
        surfaces[key] = url
    power = "\n".join(lines[end_surfaces + 1:]).strip()
    if not power:
        raise ValueError(f"{folder}: missing photon power metadata")
    try:
        power_float = float(power)
    except ValueError as exc:
        raise ValueError(f"{folder}: invalid photon power metadata") from exc
    if not (power_float > 0 and power_float < float("inf")):
        raise ValueError(f"{folder}: photon power must be finite and positive")

    files = sorted(folder.glob("PhotonMap*.dat"))
    if files != [folder / "PhotonMap.dat"]:
        raise ValueError(f"{folder}: expected one PhotonMap.dat")
    content = files[0].read_bytes()
    if len(content) % 48:
        raise ValueError(f"{folder}: truncated 6-double binary photon record")
    records = Counter()
    for offset in range(0, len(content), 48):
        # Qt QDataStream defaults to big-endian float64.
        values = struct.unpack_from(">6d", content, offset)
        identifier, x, y, z, side, surface_id = values
        if not identifier.is_integer() or identifier < 1:
            raise ValueError(f"{folder}: invalid photon identifier")
        if side not in (0.0, 1.0) or not surface_id.is_integer():
            raise ValueError(f"{folder}: invalid side/surface id")
        sid = int(surface_id)
        if sid < 0 or (sid != 0 and sid not in surfaces):
            raise ValueError(f"{folder}: unknown surface {sid}")
        import math
        if not all(map(math.isfinite, (x, y, z))):
            raise ValueError(f"{folder}: nonfinite photon position")
        # Preserve exact coordinate bits, but discard global serialized ID
        # and creation order. Surface numbers are mapped back to stable URLs.
        key = (struct.pack(">3d", x, y, z), int(side), surfaces.get(sid, ""))
        records[key] += 1
    if not records:
        raise ValueError(f"{folder}: no photons were exported")
    return records, power_float, sum(records.values()), surfaces


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("first")
    parser.add_argument("second")
    args = parser.parse_args()
    try:
        a, power_a, count_a, surfaces_a = read_export(args.first)
        b, power_b, count_b, surfaces_b = read_export(args.second)
        if a != b or power_a != power_b:
            print(f"Photon scientific mismatch: counts {count_a}/{count_b}, "
                  f"power {power_a}/{power_b}, "
                  f"record differences {sum((a-b).values())}/{sum((b-a).values())}",
                  file=sys.stderr)
            return 1
        if not surfaces_a or not surfaces_b:
            raise ValueError("surface mappings are empty; export is not meaningful")
        print(f"Photon export canonical records match ({count_a} photons; "
              f"power={power_a}; surfaces={len(surfaces_a)}/{len(surfaces_b)}).")
        return 0
    except (OSError, ValueError) as exc:
        print(f"Invalid photon export: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
