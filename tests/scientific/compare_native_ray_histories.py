#!/usr/bin/env python3
"""Compare native ordered per-ray propagation history signatures."""
import argparse
import json
import re
import sys
from pathlib import Path

FIELDS = (
    "schema", "scene_sha256", "rays", "seed", "grid_width", "grid_height",
    "aperture_area_bits", "irradiance_bits", "power_per_ray_bits",
    "ray_count", "interaction_count", "continued_count",
    "surface_terminations", "air_terminations", "escaped_rays",
    "max_depth", "history_sha256",
)


def load(path):
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    if data.get("schema") != "tonatiuhpp.a0.native-ray-histories.v1":
        raise ValueError(f"{path}: unsupported ray-history schema")
    if data.get("mode") not in ("native-gui", "headless-cli"):
        raise ValueError(f"{path}: invalid mode")
    for field in ("scene_sha256", "history_sha256"):
        if not re.fullmatch(r"[0-9a-f]{64}", str(data.get(field, ""))):
            raise ValueError(f"{path}: invalid {field}")
    for field in ("aperture_area_bits", "irradiance_bits", "power_per_ray_bits"):
        if not re.fullmatch(r"[0-9a-f]{16}", str(data.get(field, ""))):
            raise ValueError(f"{path}: invalid {field}")
    for field in (
        "rays", "seed", "ray_count", "interaction_count",
        "continued_count", "surface_terminations", "air_terminations",
        "escaped_rays", "max_depth",
    ):
        value = data.get(field)
        if not isinstance(value, str) or not re.fullmatch(r"(0|[1-9][0-9]*)", value):
            raise ValueError(f"{path}: invalid {field}")
    for field in ("grid_width", "grid_height"):
        value = data.get(field)
        if type(value) is not int or value <= 0:
            raise ValueError(f"{path}: invalid {field}")
    count = int(data["ray_count"])
    if not count or count != int(data["rays"]):
        raise ValueError(f"{path}: incomplete ray coverage")
    if (sum(int(data[x]) for x in (
        "surface_terminations", "air_terminations", "escaped_rays")) != count):
        raise ValueError(f"{path}: termination counts do not cover all rays")
    if int(data["continued_count"]) > int(data["interaction_count"]):
        raise ValueError(f"{path}: more continuations than interactions")
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("first", type=Path)
    parser.add_argument("second", type=Path)
    args = parser.parse_args()
    try:
        first, second = load(args.first), load(args.second)
        mismatches = [field for field in FIELDS if first[field] != second[field]]
        for field in mismatches:
            print(f"MISMATCH {field}: {first[field]!r} != {second[field]!r}", file=sys.stderr)
        if mismatches:
            return 1
        print(f"A0 ordered ray histories match ({first['ray_count']} rays, "
              f"{first['interaction_count']} interactions; "
              f"surface/air/escape: {first['surface_terminations']}/"
              f"{first['air_terminations']}/{first['escaped_rays']}).")
        return 0
    except (OSError, ValueError, KeyError, json.JSONDecodeError) as exc:
        print(f"Invalid ray-history signature: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
