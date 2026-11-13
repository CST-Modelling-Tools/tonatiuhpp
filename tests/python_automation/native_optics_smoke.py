"""Bounded native scientific validation and same-machine seeded repeatability."""
from __future__ import annotations

import argparse
from pathlib import Path
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "python"))
from tonatiuhpp_automation import OpticalRequest, StudyCase, TonatiuhClient, run_study


FIXTURE = r"""
var root = new NodeObject;
root.setName('M3_Flux_Parity');
var receiver = root.createNode('Receiver');
var shape = receiver.createShape();
shape.setName('Shape');
shape.insertSurface('Planar');
var profile = shape.insertProfile('Box');
profile.setParameter('uSize', '8');
profile.setParameter('vSize', '8');
shape.insertMaterial('Absorber');
tn.Clear();
tn.InsertScene(root);
tn.SetValue('//Node/M3_Flux_Parity/Receiver', 'translation', '0 0 1');
tn.ChangeSunPosition(180, 90);
if (!tn.saveScene(SCENE_PATH)) throw new Error('fixture save failed');
"""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cli", type=Path, required=True)
    args = parser.parse_args()
    client = TonatiuhClient(args.cli, timeout=120)
    with tempfile.TemporaryDirectory(prefix="tonatiuh_python_optics_") as temp:
        root = Path(temp)
        scene = root / "optical_fixture.tnhpp"
        script = root / "create_fixture.tnhpps"
        # JSON quotes/escaping work on Windows and POSIX.
        import json
        script.write_text(FIXTURE.replace("SCENE_PATH", json.dumps(str(scene))), encoding="utf-8")
        client.run_script(script)
        client.validate_scene(scene)
        optical = OpticalRequest("//Node/M3_Flux_Parity/Receiver/Shape",
                                 interception_rays=512, flux_rays=512,
                                 height_bins=4, width_bins=4)
        cases = (
            StudyCase("repeat_a", 180, 90, 512, 123456789, optical=optical),
            StudyCase("repeat_b", 180, 90, 512, 123456789, optical=optical),
        )
        result_a, result_b = run_study(client, scene, cases, root / "study")
        assert result_a.optical is not None and result_b.optical is not None
        assert result_a.optical.intercepted_power_watts == result_b.optical.intercepted_power_watts
        assert result_a.optical.flux_sha256 == result_b.optical.flux_sha256
        assert result_a.optical.flux_grid.read_bytes() == result_b.optical.flux_grid.read_bytes()
        for result in (result_a, result_b):
            assert result.execution_stats["rays_traced"] == 512
            assert result.optical.flux_grid.is_file()
            assert result.optical.flux_max_w_m2 >= result.optical.flux_min_w_m2
    print("Native optical studies passed: intercepted power, flux grid and repeatability (6 x 512 rays).")


if __name__ == "__main__":
    main()
