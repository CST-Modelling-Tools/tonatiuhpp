"""Bounded native study smoke; writes ONLY into a new temporary directory."""
from __future__ import annotations
import argparse
from pathlib import Path
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "python"))
from tonatiuhpp_automation import StudyCase, TonatiuhClient, run_study


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cli", required=True, type=Path)
    parser.add_argument("--scene", required=True, type=Path)
    options = parser.parse_args()
    client = TonatiuhClient(options.cli, timeout=120)
    with tempfile.TemporaryDirectory(prefix="tonatiuh_python_study_") as temporary:
        results = run_study(client, options.scene, (
            StudyCase("sun_east", 90.0, 45.0, 1024, 123456789),
            StudyCase("sun_west", 270.0, 45.0, 1024, 123456789),
        ), Path(temporary) / "study")
        assert len(results) == 2
        for result in results:
            assert result.prepared_scene.is_file()
            assert result.execution_stats["rays_traced"] == 1024
            client.validate_scene(result.prepared_scene)
    print("Python multi-condition native study smoke passed (2 x 1024 rays, no export).")


if __name__ == "__main__":
    main()
