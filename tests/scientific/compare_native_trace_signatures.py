#!/usr/bin/env python3
"""Compare two real-process A0 hit signatures; exit nonzero on any scientific mismatch."""
import argparse
import json
import re
import sys
from pathlib import Path

FIELDS = (
    "schema", "scene_sha256", "rays", "seed", "grid_width", "grid_height",
    "aperture_area_bits", "irradiance_bits", "power_per_ray_bits",
    "hit_count", "front_hit_count", "hit_sha256",
)

def read(path):
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    if data.get("schema") != "tonatiuhpp.a0.native-hits.v1":
        raise ValueError(f"{path}: unsupported signature schema")
    if data.get("mode") not in ("native-gui", "headless-cli"):
        raise ValueError(f"{path}: invalid mode")
    for name in ("scene_sha256", "hit_sha256"):
        if not re.fullmatch(r"[0-9a-f]{64}", str(data.get(name, ""))):
            raise ValueError(f"{path}: invalid {name}")
    for name in ("aperture_area_bits", "irradiance_bits", "power_per_ray_bits"):
        if not re.fullmatch(r"[0-9a-f]{16}", str(data.get(name, ""))):
            raise ValueError(f"{path}: invalid {name}")
    for name in ("rays", "seed", "hit_count", "front_hit_count"):
        value = data.get(name)
        if not isinstance(value, str) or not re.fullmatch(r"(0|[1-9][0-9]*)", value):
            raise ValueError(f"{path}: invalid {name}")
    for name in ("grid_width", "grid_height"):
        if not isinstance(data.get(name), int) or isinstance(data[name], bool) or data[name] <= 0:
            raise ValueError(f"{path}: invalid {name}")
    if int(data["rays"]) <= 0 or int(data["hit_count"]) <= 0 or int(data["front_hit_count"]) > int(data["hit_count"]):
        raise ValueError(f"{path}: invalid hit/ray counts")
    return data

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("first", type=Path)
    parser.add_argument("second", type=Path)
    args = parser.parse_args()
    try:
        first, second = read(args.first), read(args.second)
        mismatches = [key for key in FIELDS if first.get(key) != second.get(key)]
        if mismatches:
            for key in mismatches:
                print(f"MISMATCH {key}: {first.get(key)!r} != {second.get(key)!r}", file=sys.stderr)
            return 1
        print(f"A0 scientific hit signatures match ({first['hit_count']} events, seed {first['seed']}).")
        return 0
    except (OSError, ValueError, json.JSONDecodeError) as ex:
        print(f"Invalid scientific signature: {ex}", file=sys.stderr)
        return 2

if __name__ == "__main__":
    sys.exit(main())
