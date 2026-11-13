"""Hermetic scientific-result contract tests; no scientific kernel is mocked."""
from __future__ import annotations

from dataclasses import replace
import json
from pathlib import Path
import re
import sys
import tempfile
import unittest
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "python"))
from tonatiuhpp_automation import OpticalRequest, StudyCase, TonatiuhClient, run_study
from tonatiuhpp_automation.optics import _flux_numbers, _measurement_script


class FakeScientificCli:
    """Emulates files produced by the native subprocess, not optical math."""
    def __init__(self):
        self.calls = []
        self.bad_power = False
        self.bad_flux = False
        self.fail_optics = False

    def run_script(self, path, *, timeout=None):
        source = Path(path).read_text(encoding="utf-8")
        self.calls.append("optics" if "tn.FindInterception(" in source else "prepare")
        if "tn.FindInterception(" in source:
            if self.fail_optics:
                raise RuntimeError("simulated native optical execution failure")
            values = re.findall(r'tn\.RunFluxAnalysis\([^\n]*?, (\d+), (\d+), ("[^\n]*?"), false, (\d+)\)', source)
            assert len(values) == 1, source
            height, width, grid_json, seed = values[0]
            grid = Path(json.loads(grid_json))
            grid.write_text(("NaN\t2\n" if self.bad_flux else "1\t2\n3\t4\n"), encoding="utf-8")
            match = re.search(r'tn\.writeJson\(("(?:\\.|[^"])*"),', source)
            path = Path(json.loads(match.group(1)))
            # Assert metadata contract is validated independently by Python.
            path.write_text(json.dumps({
                "schema": "tonatiuhpp.python.optical-measurements.v1",
                "intercepted_power_watts": "bad" if self.bad_power else 32.5,
                "surface_url": "//Node/Receiver/Shape",
                "interception_rays": 12,
                "interception_master_seed": 0,
                "flux_rays": 16,
                "flux_master_seed": int(seed),
                "height_bins": int(height), "width_bins": int(width),
                "surface_side": "front",
            }), encoding="utf-8")
        else:
            scene_match = re.search(r'tn\.saveScene\(("(?:\\.|[^"])*")\)', source)
            Path(json.loads(scene_match.group(1))).write_text("scene", encoding="utf-8")
            data_line = next(line for line in source.splitlines() if "tn.writeJson(" in line)
            match = re.search(r'tn\.writeJson\(("(?:\\.|[^"])*"), (\{.*\})\)', data_line)
            Path(json.loads(match.group(1))).write_text(json.dumps(json.loads(match.group(2))), encoding="utf-8")
        return SimpleNamespace(returncode=0, stdout="", stderr="")

    def trace_scene(self, scene, *, rays, seed, update_trackers, photon_export_dir, timeout):
        self.calls.append("trace")
        return SimpleNamespace(stdout=(
            "Trace completed.\n"
            f"rays_traced: {rays}\n"
            "elapsed_seconds: 0.125000\n"
            "rays_per_second: 1000.000000\n"
            "worker_count: 1\nchunk_count: 1\nchunk_size: 10000\n"
        ), stderr="")


class OpticalTests(unittest.TestCase):
    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        self.root = Path(tmp.name)
        self.scene = self.root / "input.tnhpp"
        self.scene.write_text("source", encoding="utf-8")
        self.client = FakeScientificCli()
        self.request = OpticalRequest("//Node/Receiver/Shape", 12, 16, 2, 2)

    def case(self):
        return StudyCase("sun1", 180, 60, 128, 2468, optical=self.request)

    def test_scientific_results_are_separate_from_execution_counters(self):
        result, = run_study(self.client, self.scene, (self.case(),), self.root / "results")
        self.assertEqual(self.client.calls, ["prepare", "trace", "optics"])
        self.assertEqual(result.execution_stats["rays_traced"], 128)
        self.assertEqual(result.optical.intercepted_power_watts, 32.5)
        self.assertEqual(result.optical.flux_mean_w_m2, 2.5)
        self.assertEqual(result.optical.flux_min_w_m2, 1)
        self.assertEqual(result.optical.flux_max_w_m2, 4)
        self.assertEqual(len(result.optical.flux_sha256), 64)
        self.assertEqual(json.loads(result.optical.summary_json.read_text())["flux_master_seed"], 2468)
        self.assertNotIn("intercepted_power", json.loads(result.result_json.read_text()))
        self.assertEqual(self.scene.read_text(), "source")

    def test_invalid_optical_configuration_rejected_before_output(self):
        bad = [replace(self.request, surface_url=""),
               replace(self.request, flux_rays=200001),
               replace(self.request, interception_rays=True),
               replace(self.request, height_bins=1),
               replace(self.request, width_bins=1000000, height_bins=2),
               replace(self.request, surface_side="both")]
        for i, request in enumerate(bad):
            with self.subTest(request=request), self.assertRaises(ValueError):
                run_study(self.client, self.scene,
                          (replace(self.case(), optical=request),), self.root / f"bad{i}")
            self.assertFalse((self.root / f"bad{i}").exists())
        self.assertEqual(self.client.calls, [])

    def test_invalid_native_scientific_data_fails_closed(self):
        for i, flag in enumerate(("bad_power", "bad_flux", "fail_optics")):
            self.client = FakeScientificCli()
            setattr(self.client, flag, True)
            with self.subTest(flag=flag), self.assertRaises((ValueError, RuntimeError)):
                run_study(self.client, self.scene, (self.case(),), self.root / f"failure{i}")
            self.assertTrue((self.root / f"failure{i}" / "sun1" / "prepared_scene.tnhpp").exists())
            self.assertTrue((self.root / f"failure{i}" / "sun1" / "trace_result.json").exists())

    def test_script_escaping_and_seeds(self):
        request = replace(self.request, surface_url='//Node/"quoted"\\mirror')
        script = _measurement_script(self.scene, request, 42, self.root / "grid.tsv", self.root / "data.json")
        self.assertIn('\\"quoted\\"', script)
        self.assertIn('\\\\mirror', script)
        self.assertIn('false, 42)', script)
        self.assertIn('interception_master_seed: 0', script)

    def test_flux_grid_rejects_wrong_shape(self):
        grid = self.root / "grid.tsv"
        grid.write_text("1\t2\n3\n", encoding="utf-8")
        with self.assertRaises(ValueError):
            _flux_numbers(grid, self.request)


if __name__ == "__main__":
    unittest.main()
