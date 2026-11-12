"""Synthetic, bounded tests for CMake's CTest-output cleanup guard."""
from __future__ import annotations

import os
from pathlib import Path
import subprocess
import tempfile
import unittest

GUARD = Path(__file__).resolve().parents[1] / "cmake" / "PrepareTestOutput.cmake"


class OutputGuardTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="tonatiuhpp-c-guard-")
        self.addCleanup(self.temp.cleanup)
        self.work = Path(self.temp.name)
        self.root = self.work / "build" / "tests"
        self.root.mkdir(parents=True)
        (self.root / "CTestTestfile.cmake").write_text("# configured fixture\n")
        self.other = self.work / "outside"
        self.other.mkdir()
        (self.other / "valuable.txt").write_text("KEEP\n")

    def invoke(self, output: Path, leaf="safe-output", root=None):
        script = self.work / "run-guard.cmake"
        path = GUARD.as_posix()
        script.write_text(f'include([==[{path}]==])\n'
                          'tonatiuhpp_prepare_test_output("${OUTPUT}" "${ROOT}" "${LEAF}")\n')
        return subprocess.run(["cmake", f"-DOUTPUT={output}", f"-DROOT={root or self.root}",
                               f"-DLEAF={leaf}", "-P", str(script)],
                              capture_output=True, text=True, timeout=15)

    def test_known_fixture_clears_and_recreates_directory(self):
        output = self.root / "safe-output"
        output.mkdir()
        (output / "stale.txt").write_text("old")
        result = self.invoke(output)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(output.is_dir())
        self.assertFalse((output / "stale.txt").exists())
        self.assertEqual((self.other / "valuable.txt").read_text(), "KEEP\n")

    def test_new_directory_can_be_created_safely(self):
        output = self.root / "safe-output"
        result = self.invoke(output)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(output.is_dir())

    def test_arbitrary_outside_directory_is_rejected(self):
        result = self.invoke(self.other)
        self.assertNotEqual(result.returncode, 0)
        self.assertTrue((self.other / "valuable.txt").exists())

    def test_build_root_itself_is_rejected(self):
        result = self.invoke(self.root)
        self.assertNotEqual(result.returncode, 0)
        self.assertTrue((self.root / "CTestTestfile.cmake").exists())

    def test_parent_directory_alias_is_rejected(self):
        result = self.invoke(self.root / ".." / "outside", leaf="safe-output")
        self.assertNotEqual(result.returncode, 0)
        self.assertTrue((self.other / "valuable.txt").exists())

    def test_leaf_directory_traversal_is_rejected(self):
        result = self.invoke(self.root / "safe-output", leaf="../safe-output")
        self.assertNotEqual(result.returncode, 0)

    def test_missing_configured_ctest_root_is_rejected(self):
        (self.root / "CTestTestfile.cmake").unlink()
        result = self.invoke(self.root / "safe-output")
        self.assertNotEqual(result.returncode, 0)

    def test_symlinked_output_is_rejected_without_following_it(self):
        link = self.root / "safe-output"
        try:
            link.symlink_to(self.other, target_is_directory=True)
        except (OSError, NotImplementedError) as exc:
            self.skipTest(f"Directory symlinks unavailable: {exc}")
        result = self.invoke(link)
        self.assertNotEqual(result.returncode, 0)
        self.assertTrue(link.is_symlink())
        self.assertEqual((self.other / "valuable.txt").read_text(), "KEEP\n")


    def test_symlink_inside_fixture_cannot_remove_external_data(self):
        output = self.root / "safe-output"
        output.mkdir()
        child = output / "linked-outside"
        try:
            child.symlink_to(self.other, target_is_directory=True)
        except (OSError, NotImplementedError) as exc:
            self.skipTest(f"Directory symlinks unavailable: {exc}")
        result = self.invoke(output)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue((self.other / "valuable.txt").exists())
        self.assertFalse(child.exists())


if __name__ == "__main__":
    unittest.main()
