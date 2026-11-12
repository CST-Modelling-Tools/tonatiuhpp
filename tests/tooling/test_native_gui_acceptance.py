"""Negative-path checks of Phase D numerical oracles (no Qt/display required)."""
from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest

GUI_TESTS = Path(__file__).resolve().parents[1] / "integration/gui"
sys.path.insert(0, str(GUI_TESTS))
from run_native_gui_acceptance import (  # noqa: E402
    AcceptanceError, IDEAL_PLANAR_POWER_W,
    validate_interception, validate_interactive_flux, validate_native_report,
)


class NativeGuiAcceptanceToolTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.script = self.root / "script"
        self.script.mkdir()
        self.report = {
            "schema": "tonatiuhpp.m5.native-gui-presenter.v1",
            "mode": "interactive-flux", "status": "PASS",
            "gui_backend": "QApplication/MainWindow/actual Qt dialogs",
            "ray_dialog_accepted": True, "fixed_seed": 123456789,
            "plot_cells_yx": [[1000.0] * 8 for _ in range(8)],
            "plot_sum": 64000.0, "plot_max": 1000.0,
            "displayed_power_w": 64000.0, "displayed_peak_flux": 1000.0,
            "plot_png": str(self.root / "interactive_flux_plot.png"),
        }
        (self.root / "interactive_flux_plot.png").write_bytes(b"\x89PNG\r\n\x1a\n" + b"test")
        (self.script / "m3_flux_front.txt").write_text(("1000\t" * 8 + "\n") * 8)
        (self.script / "m3_flux_back.txt").write_text(("0\t" * 8 + "\n") * 8)
        self.interception = self.script / "m3_3_parity_result.csv"
        self.interception.write_text(
            "schema,m3.3-script-parity-v1\n"
            "surface,//Node/M3_3_Parity/Receiver/Shape\n"
            "rays,2048\nsun_azimuth_deg,180\nsun_elevation_deg,90\n"
            "power_w,64000\n")

    def test_interception_analytic_reference_passes(self):
        self.assertEqual(validate_interception(self.interception)["intercepted_power_w"],
                         IDEAL_PLANAR_POWER_W)

    def test_interception_wrong_physical_power_rejected(self):
        self.interception.write_text(self.interception.read_text().replace("64000", "6400"))
        with self.assertRaisesRegex(AcceptanceError, "64,000 W"):
            validate_interception(self.interception)

    def test_interception_wrong_scene_metadata_rejected(self):
        self.interception.write_text(self.interception.read_text().replace("rays,2048", "rays,500000000"))
        with self.assertRaisesRegex(AcceptanceError, "wrong scene"):
            validate_interception(self.interception)

    def test_real_qt_flux_analytic_reference_passes(self):
        result = validate_interactive_flux(self.report, self.script, self.root)
        self.assertEqual(result["qt_plot_integral_w"], IDEAL_PLANAR_POWER_W)
        self.assertEqual(result["gui_script_front_integral_w"], IDEAL_PLANAR_POWER_W)

    def test_gui_plot_inflated_power_rejected(self):
        self.report["plot_cells_yx"] = [[10000.0] * 8 for _ in range(8)]
        with self.assertRaisesRegex(AcceptanceError, "64,000 W"):
            validate_interactive_flux(self.report, self.script, self.root)

    def test_gui_display_power_disagrees_with_grid_rejected(self):
        self.report["displayed_power_w"] = 60000.0
        with self.assertRaisesRegex(AcceptanceError, "disagrees"):
            validate_interactive_flux(self.report, self.script, self.root)

    def test_gui_peak_disagrees_with_grid_rejected(self):
        self.report["displayed_peak_flux"] = 2000.0
        with self.assertRaisesRegex(AcceptanceError, "disagrees"):
            validate_interactive_flux(self.report, self.script, self.root)

    def test_gui_script_flux_disagrees_with_analytic_reference_rejected(self):
        (self.script / "m3_flux_front.txt").write_text(("10\t" * 8 + "\n") * 8)
        with self.assertRaisesRegex(AcceptanceError, "64,000 W"):
            validate_interactive_flux(self.report, self.script, self.root)

    def test_gui_negative_flux_rejected(self):
        self.report["plot_cells_yx"][2][3] = -1
        with self.assertRaisesRegex(AcceptanceError, "nonnegative"):
            validate_interactive_flux(self.report, self.script, self.root)

    def test_plot_png_missing_rejected(self):
        (self.root / "interactive_flux_plot.png").unlink()
        with self.assertRaises(FileNotFoundError):
            validate_interactive_flux(self.report, self.script, self.root)

    def test_plot_png_outside_evidence_directory_rejected(self):
        self.report["plot_png"] = str(self.root.parent / "not-test-output.png")
        with self.assertRaisesRegex(AcceptanceError, "escaped"):
            validate_interactive_flux(self.report, self.script, self.root)

    def test_native_report_requires_actual_qt_pass(self):
        validate_native_report(self.report, "interactive-flux")
        for key, value in (("status", "RUNNING"), ("gui_backend", "Qt offscreen"),
                           ("ray_dialog_accepted", False), ("fixed_seed", 0)):
            changed = dict(self.report)
            changed[key] = value
            with self.subTest(key=key), self.assertRaises(AcceptanceError):
                validate_native_report(changed, "interactive-flux")

    def test_native_report_wrong_mode_rejected(self):
        with self.assertRaisesRegex(AcceptanceError, "mode mismatch"):
            validate_native_report(self.report, "interactive-file")


if __name__ == "__main__":
    unittest.main()
