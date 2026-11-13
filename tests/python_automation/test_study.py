"""Fast contract tests for the external optical study orchestration.

These tests do not run a native ray tracer; native_study_smoke.py does.
"""
from __future__ import annotations

import json
from pathlib import Path
import re
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "python"))
from tonatiuhpp_automation import FieldEdit, StudyCase, StudyError, run_study
from tonatiuhpp_automation.study import _prepare_script, _summary


class FakeClient:
    def __init__(self):
        self.calls = []
        self.scripts = []

    def run_script(self, script, *, timeout=None):
        source = Path(script).read_text(encoding="utf-8")
        self.scripts.append(source)
        self.calls.append(("script", script))
        # Emulate the *file artifacts* of the already-tested native scene API.
        save_line = next(line for line in source.splitlines() if "tn.saveScene(" in line)
        save_path = json.loads(re.search(r'tn\.saveScene\(("(?:\\.|[^"])*")\)', save_line).group(1))
        Path(save_path).write_text("simulated scene", encoding="utf-8")
        data_line = next(line for line in source.splitlines() if "tn.writeJson(" in line)
        match = re.search(r'tn\.writeJson\(("(?:\\.|[^"])*"), (\{.*\})\)', data_line)
        metadata_path = json.loads(match.group(1))
        Path(metadata_path).write_text(json.dumps(json.loads(match.group(2))), encoding="utf-8")

    def trace_scene(self, scene, *, rays, seed, update_trackers, photon_export_dir, timeout):
        self.calls.append(("trace", scene, rays, seed, update_trackers, photon_export_dir))
        from types import SimpleNamespace
        return SimpleNamespace(stdout=(
            "Trace completed.\n"
            f"rays_traced: {rays}\n"
            "elapsed_seconds: 0.125000\n"
            "rays_per_second: 16384.000000\n"
            "worker_count: 2\nchunk_count: 1\nchunk_size: 10000\n"
        ), stderr="")


class StudyTests(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.source = self.root / "source scene.tnhpp"
        self.source.write_text("original", encoding="utf-8")
        self.client = FakeClient()

    def test_two_independent_conditions_and_structured_counters(self):
        cases = (
            StudyCase("morning", 90, 35, 2048, 7, field_edits=(
                FieldEdit("/Scene/Receiver", "translation", "1 2 3"),)),
            StudyCase("afternoon", 245, 60, 1024, 8, export_photons=True),
        )
        results = run_study(self.client, self.source, cases, self.root / "study")
        self.assertEqual(len(results), 2)
        self.assertEqual(results[0].execution_stats["rays_traced"], 2048)
        self.assertEqual(results[1].execution_stats["rays_traced"], 1024)
        self.assertTrue(results[1].photon_directory.is_dir())
        self.assertEqual([c[0] for c in self.client.calls], ["script", "trace", "script", "trace"])
        self.assertEqual(self.source.read_text(), "original")
        self.assertIn('tn.ChangeSunPosition(90, 35)', self.client.scripts[0])
        self.assertIn('tn.SetValue("/Scene/Receiver", "translation", "1 2 3")', self.client.scripts[0])
        self.assertNotIn('RunBenchmark', self.client.scripts[0])
        record = json.loads(results[0].result_json.read_text(encoding="utf-8"))
        self.assertEqual(record["schema"], "tonatiuhpp.python.trace-execution.v1")
        self.assertNotIn("intercepted_power", record)
        with self.assertRaises(FileExistsError):
            run_study(self.client, self.source, cases, self.root / "study")

    def test_case_validation_before_any_output(self):
        failures = (
            StudyCase("../escape", 20, 30, 100, 0),
            StudyCase("x", float("nan"), 30, 100, 0),
            StudyCase("x", 20, 100, 100, 0),
            StudyCase("x", 20, 30, 0, 0),
            StudyCase("x", 20, 30, 100, -1),
        )
        for i, case in enumerate(failures):
            with self.subTest(i=i), self.assertRaises(ValueError):
                run_study(self.client, self.source, (case,), self.root / f"out{i}")
            self.assertFalse((self.root / f"out{i}").exists())
        with self.assertRaises(ValueError):
            run_study(self.client, self.source,
                      (StudyCase("a", 0, 30, 10, 0), StudyCase("a", 0, 30, 10, 1)),
                      self.root / "duplicates")

    def test_script_literal_escaping(self):
        case = StudyCase("escaped", 10, 20, 4, 3,
                         field_edits=(FieldEdit('/World/"mirror"', 'name', 'line\\one\nother'),))
        script = _prepare_script(self.source, self.root / "prepared.tnhpp",
                                 self.root / "meta.json", case)
        self.assertIn('\\"mirror\\"', script)
        self.assertIn('line\\\\one\\nother', script)
        self.assertNotIn('line\\one\nother', script)

    def test_cli_summary_fails_closed(self):
        with self.assertRaises(StudyError):
            _summary("Trace completed.\nrays_traced: 30\n", 30)
        with self.assertRaises(StudyError):
            _summary("Trace completed.\nrays_traced: 20\n", 30)


if __name__ == "__main__":
    unittest.main()
