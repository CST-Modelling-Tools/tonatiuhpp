#!/usr/bin/env python3
"""Behavior-based test identity guard: reject reintroduced milestone filenames."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
TESTS = ROOT / "tests"
MILESTONE = re.compile(r"(?:^|[_-])(?:m[345]|p[1-9])(?:[_-]|\.|$)", re.IGNORECASE)


class TestNamesAreBehavioral(unittest.TestCase):
    def test_no_milestone_named_test_files(self):
        bad = sorted(str(path.relative_to(ROOT)) for path in TESTS.rglob("*")
                     if path.is_file() and MILESTONE.search(path.name))
        self.assertEqual(bad, [], "Use behavioral test file names, not old milestone IDs")

    def test_no_milestone_named_registered_tests(self):
        names = []
        for path in TESTS.rglob("CTest.cmake"):
            content = path.read_text(encoding="utf-8")
            names += re.findall(r"\badd_test\s*\(\s*NAME\s+([^\s)]+)", content)
        for path in TESTS.rglob("*.cpp"):
            content = path.read_text(encoding="utf-8", errors="replace")
            names += [".".join(item) for item in re.findall(
                r"\bTEST(?:_F|_P)?\s*\(\s*([A-Za-z_][A-Za-z_0-9]*)\s*,\s*([A-Za-z_][A-Za-z_0-9]*)",
                content)]
        bad = sorted(name for name in names if MILESTONE.search(name))
        self.assertEqual(bad, [], "CTest/GoogleTest names must identify behavior")


    def test_no_milestone_test_owned_scene_or_result_identifiers(self):
        # The G1 cleanup deliberately excludes versioned schemas and test-hook
        # environment keys; block reintroduction of old scene/output names.
        forbidden = re.compile(
            r"(?i)\bm[345](?:_[a-z0-9]+)+\.(?:csv|txt)\b|"
            r"\bM[345]_[A-Za-z0-9_]+\b"
        )
        bad = []
        for path in TESTS.rglob("*"):
            if not path.is_file() or path.resolve() == Path(__file__).resolve():
                continue
            try:
                content = path.read_text(encoding="utf-8")
            except (UnicodeDecodeError, OSError):
                continue
            # This is a historical *directory format*, not a new scene ID
            # or output CSV. The frozen 500M auditor deliberately requires
            # m5_full_<10 hex>, and its synthetic fixture tests that contract.
            # Keep the exception exact to these files and identifiers.
            benchmark_directory_contract = {
                "tests/scientific/audit_500m_benchmark.py": {"m5_full_"},
                "tests/scientific/test_audit_500m_benchmark.py": {"m5_full_1234567890"},
                "tests/cmake/run_heliostat_benchmark.cmake": {"m5_full_"},
            }
            rel = path.relative_to(ROOT).as_posix()
            # The CMake exception is valid ONLY for the original opt-in full
            # benchmark output contract. Do not exempt routine test outputs,
            # additional milestone markers or changes to the frozen run name.
            if rel == "tests/cmake/run_heliostat_benchmark.cmake":
                required = (
                    'if(MODE STREQUAL "full")\n'
                    '  set(_folder "${OUTPUT_DIR}/m5_full_${_id}")\n'
                    'else()\n'
                    '  set(_folder "${OUTPUT_DIR}/heliostat_${MODE}_${_id}")\n'
                    'endif()'
                )
                allowed_lines = {
                    "# Historical full-run directories retain m5_full_<10 hex> for the",
                    'set(_folder "${OUTPUT_DIR}/m5_full_${_id}")',
                }
                uses = {line.strip() for line in content.splitlines()
                        if "m5_full_" in line}
                if required not in content or not uses or uses - allowed_lines:
                    bad.append(f"{rel}: frozen full-run directory contract changed")
            matches = sorted(set(forbidden.findall(content)) -
                             benchmark_directory_contract.get(rel, set()))
            if matches:
                bad.append(f"{rel}: {', '.join(matches[:8])}")
        self.assertEqual(bad, [], "Remove test-owned milestone artifact/scene IDs")


    def test_no_milestone_coded_active_test_interfaces(self):
        # Distinct from the frozen benchmark directory identity m5_full_*.
        # Check active CMake, GUI, diagnostic and test evidence interfaces;
        # retaining historical M3/M5 prose and reference provenance is fine.
        forbidden = re.compile(
            r"TONATIUHPP_(?:M3|M5|A0)_[A-Z_]+|"
            r"\btonatiuhpp\.(?:m5|phase-d)\.|"
            r"\btonatiuhpp_m5_|"
            r"\bTONATIUHPP_M5_(?:SIMULATION|SCRIPTING)_|"
            r"\b(?:M3|M4)_(?:SHARED_RUN_PYTHON)|"
            r"\bPHASE3B2B[12]A?_PYTHON|"
            r"\bm3(?:\.3|-exit-phase3a)-(?:script-|tracker-|exit-)|"
            r"\bm3\.3-(?:script|tracker)-|"
            r"\bm3-exit-phase3a-v1"
        )
        bad = []
        for base in (ROOT / "source", ROOT / "tests"):
            for path in base.rglob("*"):
                if not path.is_file() or path.resolve() == Path(__file__).resolve():
                    continue
                if path.suffix.lower() not in {".cpp", ".h", ".py", ".cmake", ".in", ".tnhpps", ".txt"}:
                    continue
                try:
                    content = path.read_text(encoding="utf-8")
                except (UnicodeError, OSError):
                    continue
                found = sorted(set(forbidden.findall(content)))
                if found:
                    bad.append(f"{path.relative_to(ROOT)}: {', '.join(found[:6])}")
        self.assertEqual(bad, [], "Replace active milestone-coded test interfaces")


if __name__ == "__main__":
    unittest.main()
