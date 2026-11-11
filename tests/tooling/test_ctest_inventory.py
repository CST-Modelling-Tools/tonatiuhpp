"""Offline tests of the CTest inventory parser; no native Tonatiuh++ build needed."""
from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

MODULE = Path(__file__).resolve().parents[2] / "scripts" / "ctest_inventory.py"
spec = importlib.util.spec_from_file_location("ctest_inventory", MODULE)
assert spec is not None and spec.loader is not None
inventory = importlib.util.module_from_spec(spec)
spec.loader.exec_module(inventory)


class CTestInventoryTests(unittest.TestCase):
    def record(self, tests):
        return inventory.normalize({"kind": "ctestInfo", "version": {"major": 1, "minor": 0}, "tests": tests})

    def test_preserves_test_identity_and_labels(self):
        one = {"name": "scientific.HitTest.Bounded", "command": ["/tmp/test", "--gtest_filter=HitTest.Bounded"],
               "properties": [{"name": "LABELS", "value": ["equivalence", "scientific"]},
                              {"name": "TIMEOUT", "value": 120.0}]}
        two = {"name": "headless.help", "command": ["/tmp/tonatiuhpp-cli", "--help"],
               "properties": [{"name": "LABELS", "value": ["headless", "smoke"]}]}
        got = self.record([two, one])
        self.assertEqual([t["name"] for t in got["tests"]], ["headless.help", "scientific.HitTest.Bounded"])
        self.assertEqual(got["tests"][1]["labels"], ["equivalence", "scientific"])
        self.assertEqual(got["tests"][1]["timeout_seconds"], 120.0)
        self.assertEqual(got["tests"][1]["interface_inferred"], "cpp-google-test")
        self.assertEqual(got["tests"][0]["tier"], "regression (default; not runtime-measured)")

    def test_duplicate_names_are_rejected(self):
        with self.assertRaisesRegex(ValueError, "Duplicate CTest"):
            self.record([{"name": "headless.help"}, {"name": "headless.help"}])

    def test_invalid_schema_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "not CTest"):
            inventory.normalize({"kind": "different", "version": {"major": 1}})

    def test_baseline_accepts_both_formats(self):
        with tempfile.TemporaryDirectory() as scratch:
            path = Path(scratch) / "baseline.json"
            path.write_text(json.dumps({"kind": "ctestInfo", "tests": [{"name": "a"}, {"name": "b"}]}))
            self.assertEqual(inventory.baseline_names(path), {"a", "b"})
            path.write_text(json.dumps(self.record([{"name": "b"}])))
            self.assertEqual(inventory.baseline_names(path), {"b"})

    def test_unit_inference_uses_google_test_unit_label(self):
        test = {"name": "unit.Box2D.Contain", "command": ["/tmp/tonatiuhpp_math_box2d_tests",
                "--gtest_filter=Box2D.Contain"],
                "properties": [{"name": "LABELS", "value": ["unit", "math"]}]}
        got = self.record([test])["tests"][0]
        self.assertEqual(got["level_inferred"], "unit")
        self.assertEqual(got["domain_inferred"], "math")

    def test_markdown_output_quotes_pipes(self):
        got = self.record([{"name": "headless.case|special", "command": [], "properties": []}])
        self.assertIn(r"headless.case\|special", inventory.markdown(got))


if __name__ == "__main__":
    unittest.main()
