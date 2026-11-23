#!/usr/bin/env python3
"""Regression tests for the production lambda policy and audit behavior."""

import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
from audit_production_lambdas import ScannerTests, audit, find_lambda_candidates


class AuditIntegrationTests(unittest.TestCase):
    def test_only_production_sources_are_scanned(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / "source").mkdir()
            (root / "tests").mkdir()
            (root / "source" / "array.cpp").write_text(
                "char bytes[sizeof(double)] = {};\n", encoding="utf-8")
            (root / "tests" / "lambda.cpp").write_text(
                "auto f = [] { return 1; };\n", encoding="utf-8")
            with patch("audit_production_lambdas._head", return_value="example"):
                result = audit(root, expected_head="example")
            self.assertEqual(result["source_files_scanned"], 1)
            self.assertEqual(result["candidate_count"], 0)

    def test_fail_closed_on_real_production_lambda(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / "source").mkdir()
            (root / "source" / "lambda.cpp").write_text(
                "auto f = [=](int x) { return x; };\n", encoding="utf-8")
            with patch("audit_production_lambdas._head", return_value="example"):
                result = audit(root)
            self.assertEqual(result["candidate_count"], 1)
            self.assertEqual(result["candidates"][0]["line"], 1)

    def test_scanner_reports_multiline_location(self):
        found = find_lambda_candidates("int n = 1;\nauto f = [\n&n\n] { return n; };\n")
        self.assertEqual(found[0][0], 2)


if __name__ == "__main__":
    unittest.main()
