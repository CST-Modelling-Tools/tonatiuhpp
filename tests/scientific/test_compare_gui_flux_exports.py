#!/usr/bin/env python3
"""Standard-library negative tests for independent GUI Flux Analysis exports."""

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from compare_gui_flux_exports import (
    FluxExportError,
    compare_exports,
    load_flux_grid,
)


class GuiFluxExportTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.first = Path(self.temp.name) / "first.txt"
        self.second = Path(self.temp.name) / "second.txt"
        self.good = b"12.25\t0\t\n1.5\t3\t\n"
        self.first.write_bytes(self.good)
        self.second.write_bytes(self.good)

    def test_real_native_trailing_tab_accepted(self):
        grid, contents = load_flux_grid(self.first, 2, 2)
        self.assertEqual(grid, ((12.25, 0.0), (1.5, 3.0)))
        self.assertEqual(contents, self.good)

    def test_independent_identical_exports_pass(self):
        summary = compare_exports(self.first, self.second, 2, 2)
        self.assertEqual(summary["positive_cells"], 3)
        self.assertEqual(summary["maximum_flux_w_m2"], 12.25)
        self.assertEqual(len(summary["sha256"]), 64)

    def test_missing_export_rejected(self):
        self.second.unlink()
        with self.assertRaises(FileNotFoundError):
            compare_exports(self.first, self.second, 2, 2)

    def test_same_file_rejected(self):
        with self.assertRaisesRegex(FluxExportError, "distinct"):
            compare_exports(self.first, self.first, 2, 2)

    def test_mismatching_flux_rejected(self):
        self.second.write_bytes(b"12.25\t0\t\n1.5\t4\t\n")
        with self.assertRaisesRegex(FluxExportError, "differs at"):
            compare_exports(self.first, self.second, 2, 2)

    def test_mismatching_bytes_with_equal_values_rejected(self):
        self.second.write_bytes(b"12.2500\t0\t\n1.5\t3\t\n")
        with self.assertRaisesRegex(FluxExportError, "export bytes differ"):
            compare_exports(self.first, self.second, 2, 2)

    def test_wrong_row_count_rejected(self):
        with self.assertRaisesRegex(FluxExportError, "expected 3 rows"):
            compare_exports(self.first, self.second, 3, 2)

    def test_wrong_column_count_rejected(self):
        self.first.write_bytes(b"12.25\t0\t1\t\n1.5\t3\t\n")
        with self.assertRaisesRegex(FluxExportError, "expected 2 columns"):
            load_flux_grid(self.first, 2, 2)

    def test_zero_only_map_rejected(self):
        self.first.write_bytes(b"0\t0\t\n0\t0\t\n")
        with self.assertRaisesRegex(FluxExportError, "zero-only"):
            load_flux_grid(self.first, 2, 2)

    def test_nan_rejected(self):
        self.first.write_bytes(b"nan\t0\t\n1.5\t3\t\n")
        with self.assertRaisesRegex(FluxExportError, "non-finite"):
            load_flux_grid(self.first, 2, 2)

    def test_infinity_rejected(self):
        self.first.write_bytes(b"inf\t0\t\n1.5\t3\t\n")
        with self.assertRaisesRegex(FluxExportError, "non-finite"):
            load_flux_grid(self.first, 2, 2)

    def test_negative_flux_rejected(self):
        self.first.write_bytes(b"-1\t0\t\n1.5\t3\t\n")
        with self.assertRaisesRegex(FluxExportError, "negative"):
            load_flux_grid(self.first, 2, 2)

    def test_invalid_utf8_rejected(self):
        self.first.write_bytes(b"\xff\t0\t\n1.5\t3\t\n")
        with self.assertRaisesRegex(FluxExportError, "UTF-8"):
            load_flux_grid(self.first, 2, 2)

    def test_empty_cell_rejected(self):
        self.first.write_bytes(b"12.25\t\t\n1.5\t3\t\n")
        with self.assertRaisesRegex(FluxExportError, "empty cell"):
            load_flux_grid(self.first, 2, 2)

    def test_invalid_text_rejected(self):
        self.first.write_bytes(b"oops\t0\t\n1.5\t3\t\n")
        with self.assertRaisesRegex(FluxExportError, "invalid numeric"):
            load_flux_grid(self.first, 2, 2)

    def test_invalid_dimensions_rejected(self):
        with self.assertRaisesRegex(FluxExportError, ">= 2"):
            compare_exports(self.first, self.second, 1, 2)

    def test_windows_newline_pair_accepted(self):
        self.first.write_bytes(self.good.replace(b"\n", b"\r\n"))
        self.second.write_bytes(self.good.replace(b"\n", b"\r\n"))
        self.assertEqual(compare_exports(self.first, self.second, 2, 2)["positive_cells"], 3)


if __name__ == "__main__":
    unittest.main()
