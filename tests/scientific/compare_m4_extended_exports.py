#!/usr/bin/env python3
"""Compare M4 File exports by physical photon multiset and power, across segments.

This intentionally ignores nonphysical global photon IDs and file ordering.
It does not change, rewrite or normalize any photon export on disk.
"""
import argparse
from collections import Counter
import math
from pathlib import Path
import re
import struct
import sys


RECORD = struct.Struct(">6d")  # QDataStream default big-endian double.


def load(directory, basename="PhotonMap", max_per_file=None):
    folder = Path(directory)
    metadata = (folder / f"{basename}_parameters.txt").read_text(encoding="utf-8")
    lines = metadata.splitlines()
    if not lines or lines[0] != "START PARAMETERS":
        raise ValueError(f"{folder}: missing parameters header")
    stop = lines.index("END PARAMETERS")
    if lines[1:stop] != ["id", "x", "y", "z", "side", "surface ID"]:
        raise ValueError(f"{folder}: unexpected File column layout")
    start_surfaces = lines.index("START SURFACES", stop)
    end_surfaces = lines.index("END SURFACES", start_surfaces)
    surfaces = {}
    for item in lines[start_surfaces + 1:end_surfaces]:
        number, sep, url = item.partition(" ")
        if not sep or not number.isdecimal() or not url:
            raise ValueError(f"{folder}: invalid surface metadata")
        key = int(number)
        if key in surfaces or key <= 0:
            raise ValueError(f"{folder}: duplicate/invalid surface identifier")
        surfaces[key] = url
    if not surfaces:
        raise ValueError(f"{folder}: no physical surface mapping")
    power = float("\n".join(lines[end_surfaces + 1:]).strip())
    if not math.isfinite(power) or power <= 0:
        raise ValueError(f"{folder}: invalid power per photon")

    single = folder / f"{basename}.dat"
    pattern = re.compile(re.escape(basename) + r"_([1-9][0-9]*)\.dat\Z")
    entries = list(folder.glob(f"{basename}*.dat"))
    if single.exists():
        if entries != [single]:
            raise ValueError(f"{folder}: mixed single/segmented data")
        ordered = [single]
    else:
        numbered = {}
        for path in entries:
            match = pattern.fullmatch(path.name)
            if not match:
                raise ValueError(f"{folder}: invalid numbered file {path.name}")
            n = int(match.group(1))
            if n in numbered:
                raise ValueError(f"{folder}: duplicate segment {n}")
            numbered[n] = path
        if not numbered or sorted(numbered) != list(range(1, len(numbered) + 1)):
            raise ValueError(f"{folder}: missing or noncontiguous segments")
        ordered = [numbered[k] for k in sorted(numbered)]

    records = Counter()
    next_id = 1
    counts = []
    for path in ordered:
        if path.is_symlink() or not path.is_file():
            raise ValueError(f"{folder}: unsafe data file")
        content = path.read_bytes()
        if len(content) % RECORD.size:
            raise ValueError(f"{folder}: truncated binary photon record")
        count = len(content) // RECORD.size
        counts.append(count)
        if max_per_file and count > max_per_file:
            raise ValueError(f"{folder}: exceeded segment size {max_per_file}")
        for offset in range(0, len(content), RECORD.size):
            identifier, x, y, z, side, surface_id = RECORD.unpack_from(content, offset)
            if not identifier.is_integer() or identifier != next_id:
                raise ValueError(f"{folder}: nonconsecutive global id {identifier}/{next_id}")
            if side not in (0.0, 1.0) or not surface_id.is_integer():
                raise ValueError(f"{folder}: malformed side/surface id")
            sid = int(surface_id)
            if sid < 0 or (sid != 0 and sid not in surfaces):
                raise ValueError(f"{folder}: unknown surface identifier {sid}")
            if not all(math.isfinite(a) for a in (x, y, z)):
                raise ValueError(f"{folder}: non-finite physical coordinate")
            records[(struct.pack(">3d", x, y, z), int(side), surfaces.get(sid, ""))] += 1
            next_id += 1
    if not records:
        raise ValueError(f"{folder}: no physical photons")
    return records, power, counts


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("first")
    ap.add_argument("second")
    ap.add_argument("--expect-duplicate", action="store_true")
    ap.add_argument("--expect-segments", action="store_true")
    ap.add_argument("--max-per-file", type=int)
    args = ap.parse_args()
    try:
        a, power_a, files_a = load(args.first, max_per_file=args.max_per_file)
        b, power_b, files_b = load(args.second, max_per_file=args.max_per_file)
        if args.expect_segments and (len(files_a) < 2 or len(files_b) < 2):
            raise ValueError("Expected multiple File segments in both exports")
        expected = Counter({k: v * (2 if args.expect_duplicate else 1) for k, v in a.items()})
        expected_power = power_a / 2 if args.expect_duplicate else power_a
        if expected != b or not math.isclose(expected_power, power_b, rel_tol=1e-5, abs_tol=1e-12):
            print(f"M4 physical photon mismatch: {sum(expected.values())} vs {sum(b.values())}, "
                  f"power {expected_power} vs {power_b}, "
                  f"missing {sum((expected-b).values())}, extra {sum((b-expected).values())}",
                  file=sys.stderr)
            return 1
        print(f"M4 photon equality accepted: {sum(b.values())} photons, "
              f"{len(files_a)}/{len(files_b)} output files, power={power_b}")
        return 0
    except (OSError, ValueError, OverflowError) as exc:
        print(f"M4 invalid File export: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
