"""Negative-path checks for Phase E relocation safety and installed artifact identity."""
from __future__ import annotations

import os
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "integration" / "installation"))
from run_installed_acceptance import (  # noqa: E402
    inside, locate_executables, stage_relocated_install,
)


class InstalledAcceptanceToolTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.source = self.root / "source"
        self.scratch = self.root / "scratch"
        self.source.mkdir()
        self.scratch.mkdir()
        self.origin = self.scratch / "cmake-install"
        self.origin.mkdir()

    def test_relocation_preserves_tree_and_removes_old_prefix(self):
        (self.origin / "example.txt").write_text("relocated", encoding="utf-8")
        result = stage_relocated_install(self.origin, self.scratch, self.source)
        self.assertTrue(inside(result, self.scratch))
        self.assertEqual((result / "example.txt").read_text(), "relocated")
        self.assertFalse(self.origin.exists())

    def test_refuse_moving_unexpected_directory(self):
        foreign = self.scratch / "user-data"
        foreign.mkdir()
        with self.assertRaisesRegex(RuntimeError, "disposable name"):
            stage_relocated_install(foreign, self.scratch, self.source)
        self.assertTrue(foreign.exists())

    def test_refuse_moving_from_outside_explicit_scratch(self):
        foreign = self.root / "external-install"
        foreign.mkdir()
        with self.assertRaisesRegex(RuntimeError, "direct child"):
            stage_relocated_install(foreign, self.scratch, self.source)
        self.assertTrue(foreign.exists())

    def test_refuse_scratch_inside_source(self):
        nested = self.source / "scratch"
        nested.mkdir()
        (nested / "cmake-install").mkdir()
        with self.assertRaisesRegex(RuntimeError, "separate"):
            stage_relocated_install(nested / "cmake-install", nested, self.source)

    def test_refuse_symlinked_install_origin(self):
        real_dir = self.root / "real-install"
        real_dir.mkdir()
        self.origin.rmdir()
        try:
            self.origin.symlink_to(real_dir, target_is_directory=True)
        except (OSError, NotImplementedError):
            self.skipTest("Unable to create directory symlink on this Windows runner")
        with self.assertRaisesRegex(RuntimeError, "symlink"):
            stage_relocated_install(self.origin, self.scratch, self.source)
        self.assertTrue(real_dir.exists())

    def test_installed_gui_cli_identity_required(self):
        with self.assertRaisesRegex(RuntimeError, "executable missing"):
            locate_executables(self.origin)

    def test_installed_real_binaries_must_be_within_prefix(self):
        if sys.platform == "darwin":
            gui = self.origin / "tonatiuhpp-gui.app/Contents/MacOS/tonatiuhpp-gui"
            cli = self.origin / "bin/tonatiuhpp-cli"
        else:
            ext = ".exe" if os.name == "nt" else ""
            gui = self.origin / "bin" / ("tonatiuhpp-gui" + ext)
            cli = self.origin / "bin" / ("tonatiuhpp-cli" + ext)
        gui.parent.mkdir(parents=True, exist_ok=True)
        cli.parent.mkdir(parents=True, exist_ok=True)
        gui.write_bytes(b"test executable")
        cli.write_bytes(b"test executable")
        if os.name != "nt":
            gui.chmod(0o755)
            cli.chmod(0o755)
        self.assertEqual(locate_executables(self.origin), (gui, cli))


if __name__ == "__main__":
    unittest.main()
