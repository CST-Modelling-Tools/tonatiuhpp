"""Failure diagnostics and status semantics for local Python optical studies."""
from __future__ import annotations

from dataclasses import replace
import json
from pathlib import Path
import sys
import tempfile
import unittest
from types import SimpleNamespace

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "python"))
from tonatiuhpp_automation import CliError, CliResult, OpticalRequest, StudyCase, run_study
from tonatiuhpp_automation.study import StudyError, _summary
from test_optics import FakeScientificCli


class FailingScientificCli(FakeScientificCli):
    def __init__(self, fail_stage):
        super().__init__()
        self.fail_stage = fail_stage

    def run_script(self, path, *, timeout=None):
        stage = "optics" if "tn.FindInterception(" in Path(path).read_text(encoding="utf-8") else "prepare"
        if stage == self.fail_stage:
            raise CliError("simulated failure", result=CliResult(("fake",), 2, "partial out", "native error"))
        return super().run_script(path, timeout=timeout)

    def trace_scene(self, scene, *, rays, seed, update_trackers, photon_export_dir, timeout):
        if self.fail_stage == "trace":
            raise CliError("simulated trace failure", result=CliResult(("fake",), 3, "partial trace", "trace error"))
        return super().trace_scene(scene, rays=rays, seed=seed,
                                   update_trackers=update_trackers,
                                   photon_export_dir=photon_export_dir, timeout=timeout)


class QualityTests(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.scene = self.root / "original.tnhpp"
        self.scene.write_text("original", encoding="utf-8")
        self.optical = OpticalRequest("//Node/Receiver/Shape", 12, 16, 2, 2)
        self.case = StudyCase("testcase", 180, 60, 128, 2468, optical=self.optical)

    def test_success_status_and_scientific_files(self):
        (result,) = run_study(FakeScientificCli(), self.scene, (self.case,), self.root / "study")
        status = json.loads(result.status_json.read_text(encoding="utf-8"))
        self.assertEqual((status["status"], status["stage"]), ("completed", "complete"))
        self.assertIsNotNone(result.optical)
        self.assertTrue(result.optical.summary_json.exists())
        self.assertEqual(self.scene.read_text(encoding="utf-8"), "original")

    def test_each_native_failure_retains_logs_and_status(self):
        for stage in ("prepare", "trace", "optics"):
            out = self.root / stage
            with self.subTest(stage=stage), self.assertRaises(CliError):
                run_study(FailingScientificCli(stage), self.scene, (self.case,), out)
            case_dir = out / "testcase"
            status = json.loads((case_dir / "case_status.json").read_text(encoding="utf-8"))
            self.assertEqual(status["status"], "failed")
            self.assertEqual(status["stage"], stage)
            self.assertEqual((case_dir / f"{stage}_stderr.txt").read_text(encoding="utf-8"),
                             "native error" if stage != "trace" else "trace error")
            self.assertFalse((case_dir / "optical_summary.json").exists())
            self.assertEqual(self.scene.read_text(encoding="utf-8"), "original")

    def test_bad_native_data_is_never_marked_complete(self):
        client = FakeScientificCli()
        client.bad_flux = True
        with self.assertRaises(ValueError):
            run_study(client, self.scene, (self.case,), self.root / "bad")
        status = json.loads((self.root / "bad" / "testcase" / "case_status.json").read_text())
        self.assertEqual((status["status"], status["stage"]), ("failed", "optics"))
        self.assertFalse((self.root / "bad" / "testcase" / "optical_summary.json").exists())

    def test_rejects_nonfinite_native_timing_and_invalid_edit_container(self):
        text = ("Trace completed.\nrays_traced: 5\nelapsed_seconds: 1e309\n"
                "rays_per_second: 5\nworker_count: 1\nchunk_count: 1\nchunk_size: 10000\n")
        with self.assertRaises(StudyError):
            _summary(text, 5)
        with self.assertRaises(ValueError):
            run_study(FakeScientificCli(), self.scene,
                      (replace(self.case, field_edits=None),), self.root / "invalid")
        self.assertFalse((self.root / "invalid").exists())


if __name__ == "__main__":
    unittest.main()
