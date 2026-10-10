#!/usr/bin/env python3
"""Exercise the native A0 signature file comparator without a graphical display."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

def main():
    tool = Path(__file__).with_name("compare_native_trace_signatures.py")
    sample = {
        "schema": "tonatiuhpp.a0.native-hits.v1",
        "mode": "native-gui",
        "scene_sha256": "a" * 64,
        "rays": "20001",
        "seed": "123456789",
        "grid_width": 200,
        "grid_height": 200,
        "aperture_area_bits": "3ff0000000000000",
        "irradiance_bits": "408f400000000000",
        "power_per_ray_bits": "3f10000000000000",
        "hit_count": "400",
        "front_hit_count": "200",
        "hit_sha256": "b" * 64
    }
    with tempfile.TemporaryDirectory() as directory:
        a, b = Path(directory) / "a.json", Path(directory) / "b.json"
        a.write_text(json.dumps(sample), encoding="utf-8")
        other = dict(sample, mode="headless-cli")
        b.write_text(json.dumps(other), encoding="utf-8")
        def result():
            return subprocess.run([sys.executable, str(tool), str(a), str(b)],
                                  capture_output=True, text=True).returncode
        assert result() == 0
        other["hit_sha256"] = "c" * 64
        b.write_text(json.dumps(other), encoding="utf-8")
        assert result() == 1
        other["hit_sha256"] = "b" * 64
        other["seed"] = "123456788"
        b.write_text(json.dumps(other), encoding="utf-8")
        assert result() == 1
        b.write_text("{invalid", encoding="utf-8")
        assert result() == 2

if __name__ == "__main__":
    main()
