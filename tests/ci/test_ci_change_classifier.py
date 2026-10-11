#!/usr/bin/env python3
"""Small, cross-platform safety checks for the CI change classifier."""

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
from ci_change_classifier import check_docs, is_docs_only, read_changed_paths


class ChangeClassifierTests(unittest.TestCase):
    def test_markdown_only(self):
        self.assertTrue(is_docs_only(["PROJECT_STATUS.md", "docs/release notes.md"]))

    def test_empty_diff_is_full_ci(self):
        self.assertFalse(is_docs_only([]))

    def test_source_change_is_full_ci(self):
        self.assertFalse(is_docs_only(["source/application/main.cpp"]))

    def test_mixed_docs_source_is_full_ci(self):
        self.assertFalse(is_docs_only(["docs/a.md", "source/CMakeLists.txt"]))

    def test_workflow_change_is_full_ci(self):
        self.assertFalse(is_docs_only(["docs/a.md", ".github/workflows/ci.yml"]))

    def test_rust_doc_source_is_full_ci(self):
        self.assertFalse(is_docs_only(["help/Sphinx/main/index.rst"]))

    def test_unrecognized_extension_is_full_ci(self):
        self.assertFalse(is_docs_only(["docs/benchmark_config_schema_v1.json"]))

    def test_uppercase_extension_is_full_ci(self):
        self.assertFalse(is_docs_only(["README.MD"]))

    def test_parent_paths_are_not_documentation(self):
        self.assertFalse(is_docs_only(["../README.md"]))

    def test_nul_delimited_names_preserve_spaces(self):
        with tempfile.TemporaryDirectory() as temp:
            changed = Path(temp) / "paths"
            changed.write_bytes(b"docs/a b.md\0PROJECT_STATUS.md\0")
            self.assertEqual(read_changed_paths(changed), ["docs/a b.md", "PROJECT_STATUS.md"])

    def test_empty_nul_input_is_empty_diff(self):
        with tempfile.TemporaryDirectory() as temp:
            changed = Path(temp) / "paths"
            changed.write_bytes(b"")
            self.assertEqual(read_changed_paths(changed), [])

    def test_unterminated_input_is_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            changed = Path(temp) / "paths"
            changed.write_bytes(b"docs/a.md")
            with self.assertRaisesRegex(ValueError, "NUL-terminated"):
                read_changed_paths(changed)

    def test_utf8_and_merge_marker_validation(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            doc = root / "notes.md"
            doc.write_text("# Header\nUseful explanation\n", encoding="utf-8")
            self.assertEqual(check_docs(["notes.md"], root), 1)
            doc.write_text("<<<<<<< HEAD\nconflict\n>>>>>>> branch\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "conflict marker"):
                check_docs(["notes.md"], root)
            doc.write_bytes(b"\xff")
            with self.assertRaises(UnicodeDecodeError):
                check_docs(["notes.md"], root)

    def test_deleted_markdown_is_permitted(self):
        with tempfile.TemporaryDirectory() as temp:
            self.assertEqual(check_docs(["removed.md"], Path(temp)), 0)

    def test_non_markdown_document_check_is_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(ValueError, "Markdown-only"):
                check_docs(["src.cpp"], Path(temp))


if __name__ == "__main__":
    unittest.main()
