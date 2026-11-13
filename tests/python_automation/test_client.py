
"""Hermetic subprocess contract tests: no scientific kernel is mocked."""

from __future__ import annotations

import os
from pathlib import Path
import stat
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "python"))
from tonatiuhpp_automation import CliError, TonatiuhClient


class ClientTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.cli = self.root / ("fake_cli.cmd" if os.name == "nt" else "fake_cli")

        if os.name == "nt":
            self.cli.write_text(
                '@echo off\r\necho %*\r\n',
                encoding="ascii"
            )
        else:
            self.cli.write_text(
                '#!/bin/sh\nprintf "%s\\n" "$@"\n',
                encoding="ascii"
            )
            self.cli.chmod(self.cli.stat().st_mode | stat.S_IXUSR)

        self.scene = self.root / "some scene.tnhpp"
        self.scene.write_text("test fixture", encoding="utf-8")
        self.client = TonatiuhClient(self.cli, timeout=10)

    def test_validate_path_with_spaces(self):
        result = self.client.validate_scene(self.scene)
        self.assertEqual(result.returncode, 0)
        self.assertIn(str(self.scene.resolve()), result.stdout)

    def test_trace_arguments(self):
        result = self.client.trace_scene(
            self.scene,
            rays=2048,
            seed=123456789,
            update_trackers=True
        )
        self.assertIn("--no-export", result.command)
        self.assertIn("--update-trackers", result.command)
        self.assertEqual(
            result.command[result.command.index("--seed") + 1],
            "123456789"
        )

    def test_export_is_restricted_to_empty_directory(self):
        folder = self.root / "photon output"
        folder.mkdir()

        result = self.client.trace_scene(
            self.scene,
            rays=2,
            seed=0,
            photon_export_dir=folder,
            photon_file_name="runA",
            photon_file_size=-1,
            photon_surfaces=("/World/heliostat",)
        )

        self.assertIn("--photon-surface", result.command)
        self.assertIn(str(folder.resolve()), result.command)

        (folder / "occupied").write_text("x", encoding="ascii")

        with self.assertRaises(ValueError):
            self.client.trace_scene(
                self.scene,
                rays=2,
                seed=0,
                photon_export_dir=folder
            )

    def test_invalid_values_rejected(self):
        for values in (
            {"rays": 0, "seed": 1},
            {"rays": True, "seed": 1},
            {"rays": 10, "seed": -1},
            {"rays": 10, "seed": True},
            {"rays": 10, "seed": 1, "photon_file_name": "run"}
        ):
            with self.subTest(values=values), self.assertRaises(ValueError):
                self.client.trace_scene(self.scene, **values)

        with self.assertRaises(FileNotFoundError):
            self.client.validate_scene(self.root / "missing.tnhpp")

    def test_script_passed_to_cli(self):
        script = self.root / "some script.tnhpps"
        script.write_text("// script", encoding="ascii")

        result = self.client.run_script(script)

        self.assertEqual(result.command[1], "run-script")
        self.assertIn(str(script.resolve()), result.command)

    def test_failure_preserves_exit_code(self):
        if os.name == "nt":
            self.cli.write_text(
                '@echo off\r\necho failed 1>&2\r\nexit /b 2\r\n',
                encoding="ascii"
            )
        else:
            self.cli.write_text(
                '#!/bin/sh\necho failed >&2\nexit 2\n',
                encoding="ascii"
            )

        with self.assertRaises(CliError) as error:
            self.client.validate_scene(self.scene)

        self.assertEqual(error.exception.result.returncode, 2)

    @unittest.skipIf(os.name == "nt", "POSIX timeout subprocess fixture")
    def test_timeout(self):
        self.cli.write_text(
            '#!/bin/sh\nsleep 2\n',
            encoding="ascii"
        )

        with self.assertRaises(CliError):
            TonatiuhClient(
                self.cli,
                timeout=0.05
            ).validate_scene(self.scene)


if __name__ == "__main__":
    unittest.main()
