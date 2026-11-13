"""Bounded production CLI smoke for the external Python adapter."""
from pathlib import Path
import argparse
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "python"))
from tonatiuhpp_automation import TonatiuhClient

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cli", required=True, type=Path)
    parser.add_argument("--scene", required=True, type=Path)
    options = parser.parse_args()
    client = TonatiuhClient(options.cli, timeout=120)
    validated = client.validate_scene(options.scene)
    if "Scene validation succeeded" not in validated.stdout:
        raise AssertionError(f"CLI did not validate scene: {validated.stdout}")
    traced = client.trace_scene(options.scene, rays=2048, seed=123456789)
    if "Trace completed." not in traced.stdout or "rays_traced: 2048" not in traced.stdout:
        raise AssertionError(f"Unexpected native trace result: {traced.stdout}")
    print("Python automation native smoke passed (2048 rays, no export).")

if __name__ == "__main__":
    main()
