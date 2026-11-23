#!/usr/bin/env python3
"""Read-only lexical audit for C++ lambda expressions in Tonatiuh++ source/.

Usage:
  python scripts/audit_production_lambdas.py --root .
  python scripts/audit_production_lambdas.py --self-test
  python scripts/audit_production_lambdas.py --root . --expected-head <commit-sha>

No writes, build, CTest, benchmark, staging, commit, network, or external packages.
This is a conservative lexical screen, not a replacement for a compiler AST.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import subprocess
import sys
import unittest

CPP_EXTENSIONS = {".cpp", ".cc", ".cxx", ".c", ".h", ".hh", ".hpp", ".hxx", ".ipp", ".inl", ".tpp"}


def _mask_span(chars: list[str], begin: int, end: int) -> None:
    for pos in range(begin, end):
        if chars[pos] not in "\r\n":
            chars[pos] = " "


def mask_non_code(source: str) -> str:
    """Keep every character offset and newline while masking C++ comments/literals."""
    result = list(source)
    index = 0
    end = len(source)
    while index < end:
        c = source[index]
        if source.startswith("//", index):
            stop = source.find("\n", index)
            stop = end if stop == -1 else stop
            _mask_span(result, index, stop)
            index = stop
            continue
        if source.startswith("/*", index):
            close = source.find("*/", index + 2)
            stop = end if close == -1 else close + 2
            _mask_span(result, index, stop)
            index = stop
            continue
        # C++11 raw strings may use a prefix; matching at R handles all of them.
        if c == "R" and index + 1 < end and source[index + 1] == '"':
            opening = source.find("(", index + 2, min(end, index + 20))
            if opening != -1:
                delimiter = source[index + 2:opening]
                if len(delimiter) <= 16 and not re.search(r'[\\\s()"]', delimiter):
                    closing_marker = ")" + delimiter + '"'
                    close = source.find(closing_marker, opening + 1)
                    stop = end if close == -1 else close + len(closing_marker)
                    _mask_span(result, index, stop)
                    index = stop
                    continue
        if c in ('"', "'"):
            quote = c
            cursor = index + 1
            while cursor < end:
                if source[cursor] == "\\":
                    cursor += 2
                    continue
                if source[cursor] == quote:
                    cursor += 1
                    break
                cursor += 1
            stop = min(cursor, end)
            _mask_span(result, index, stop)
            index = stop
            continue
        index += 1
    return "".join(result)


def _balanced_after(text: str, pos: int, opening: str, closing: str) -> int | None:
    if pos >= len(text) or text[pos] != opening:
        return None
    depth = 1
    cursor = pos + 1
    while cursor < len(text) and depth:
        if text[cursor] == opening:
            depth += 1
        elif text[cursor] == closing:
            depth -= 1
        cursor += 1
    return cursor if depth == 0 else None


def _skip_space(text: str, pos: int) -> int:
    while pos < len(text) and text[pos].isspace():
        pos += 1
    return pos


def _captures_possible(value: str) -> bool:
    value = value.strip()
    if not value:
        return True
    if value.startswith(("&", "=", "*this", "this", "...")):
        return True
    # Identifiers and init captures, but not array subscripts with numeric indices.
    return re.match(r"[A-Za-z_]\w*\b", value) is not None


def _has_lambda_body(text: str, pos: int) -> bool:
    """Check capture/declarator prefix for a lambda function body."""
    cursor = _skip_space(text, pos)
    # A lambda introducer cannot be followed directly by an assignment.
    # For example, `char bytes[sizeof(double)] = {};` declares an array,
    # and its brace initializer must not be mistaken for a lambda body.
    if cursor < len(text) and text[cursor] == "=":
        return False
    if cursor < len(text) and text[cursor] == "<":
        after = _balanced_after(text, cursor, "<", ">")
        if after is None:
            return False
        cursor = _skip_space(text, after)
    if cursor < len(text) and text[cursor] == "(":
        after = _balanced_after(text, cursor, "(", ")")
        if after is None:
            return False
        cursor = _skip_space(text, after)
    # C++ qualifiers, trailing return and requires-clauses may appear here.
    # Scan a bounded suffix; semicolon/assignment/expression close before a
    # candidate body all indicate this is not a lambda declarator.
    end = min(len(text), cursor + 750)
    depth_paren = depth_square = 0
    while cursor < end:
        c = text[cursor]
        if c == "(":
            depth_paren += 1
        elif c == ")":
            if depth_paren == 0:
                return False
            depth_paren -= 1
        elif c == "[":
            depth_square += 1
        elif c == "]":
            if depth_square == 0:
                return False
            depth_square -= 1
        elif not depth_paren and not depth_square:
            if c == "{":
                return True
            if c in ";}":
                return False
            # A plain call or array access may have another statement's body
            # later; do not follow the next statement after a comma either.
            if c == ",":
                return False
        cursor += 1
    return False


def find_lambda_candidates(source: str) -> list[tuple[int, str]]:
    """Return conservative syntax candidates as (line, source excerpt)."""
    text = mask_non_code(source)
    candidates: list[tuple[int, str]] = []
    pos = 0
    while True:
        opening = text.find("[", pos)
        if opening == -1:
            break
        pos = opening + 1
        if text.startswith("[[", opening):
            continue
        if re.search(r"\boperator\s*$", text[max(0, opening - 30):opening]):
            continue
        closing = _balanced_after(text, opening, "[", "]")
        if closing is None or closing - opening > 1500:
            continue
        capture_text = text[opening + 1:closing - 1]
        if not _captures_possible(capture_text):
            continue
        previous = text[opening - 1] if opening else ""
        # An array declaration `name[] { ... }` is not a lambda expression.
        if not capture_text.strip() and (previous.isalnum() or previous in "_]"):
            continue
        if not _has_lambda_body(text, closing):
            continue
        line = source.count("\n", 0, opening) + 1
        excerpt = source[opening:min(len(source), opening + 110)].splitlines()[0].strip()
        candidates.append((line, excerpt))
    return candidates


def _head(root: Path) -> str:
    proc = subprocess.run(["git", "-C", str(root), "rev-parse", "HEAD"],
                          capture_output=True, text=True, encoding="utf-8", timeout=15)
    if proc.returncode:
        raise ValueError("Could not resolve Git HEAD: " + proc.stderr.strip())
    return proc.stdout.strip()


def audit(root: Path, expected_head: str | None = None) -> dict:
    root = root.resolve(strict=True)
    source_dir = root / "source"
    if not source_dir.is_dir():
        raise ValueError("Expected source/ directory: " + str(source_dir))
    head = _head(root)
    if expected_head and head != expected_head:
        raise ValueError("Wrong Git HEAD: expected " + expected_head + ", found " + head)
    paths = sorted(p for p in source_dir.rglob("*") if p.suffix.lower() in CPP_EXTENSIONS)
    if not paths:
        raise ValueError("No production C/C++ files found under source/")
    found = []
    for path in paths:
        if path.is_symlink():
            raise ValueError("Refusing to scan linked source path: " + str(path))
        if not path.is_file():
            continue
        content = path.read_text(encoding="utf-8-sig")
        for line, excerpt in find_lambda_candidates(content):
            found.append({"file": path.relative_to(root).as_posix(),
                          "line": line, "excerpt": excerpt})
    return {"revision": head, "source_files_scanned": len(paths),
            "candidate_count": len(found), "candidates": found,
            "audit_kind": "read-only conservative C++ lexical screen"}


class ScannerTests(unittest.TestCase):
    def test_detects_standard_and_generic_lambdas(self):
        expressions = [
            "auto f = [] { return 1; };",
            "QTimer::singleShot(2, [this] { run(); });",
            "auto f = [&](int i) mutable -> int { return i; };",
            "auto f = [x = getX(), &y] (int z) { return x + y + z; };",
            "auto f = []<typename T>(T x) { return x; };",
            "auto f = [\n&x, y\n](int z) noexcept { return y; };",
        ]
        for code in expressions:
            with self.subTest(code=code):
                self.assertEqual(len(find_lambda_candidates(code)), 1)

    def test_rejects_non_lambda_syntax(self):
        source = '''
            double operator[](int i) const { return items[i]; }
            auto a = values[0] < values[1] ? 3 : 5;
            QRegularExpression("[0-9]{1,10}");
            /* [this] () { not a lambda } */
            // [&] { not a lambda }
            setStyleSheet(R"(QFrame[inFocus=true] { border: 1px; })");
            int entries[] {1,2,3};
            auto x = objects[position](arg);
            [[nodiscard]] int value();
        '''
        self.assertEqual(find_lambda_candidates(source), [])

    def test_rejects_array_initializer_with_sizeof(self):
        expressions = [
            "char bytes[sizeof(double)] = {};",
            "unsigned char storage[sizeof(double)] = {0};",
            "char bytes[sizeof(double)]\n    = {};",
            "auto x = values[index] = {};",
        ]
        for code in expressions:
            with self.subTest(code=code):
                self.assertEqual(find_lambda_candidates(code), [])

    def test_line_numbers_and_escaped_strings(self):
        source = '''// [] { ignore }
const char* s = "\\\" [&]{} ";
auto y = [=](int x) { return x; };
'''
        self.assertEqual(find_lambda_candidates(source)[0][0], 3)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path("."), help="Tonatiuh++ checkout")
    parser.add_argument("--expected-head", default=None, help="Require exact Git SHA")
    parser.add_argument("--json", action="store_true", help="Print machine-readable JSON")
    parser.add_argument("--self-test", action="store_true", help="Run built-in scanner tests")
    args = parser.parse_args()
    if args.self_test:
        suite = unittest.defaultTestLoader.loadTestsFromTestCase(ScannerTests)
        return 0 if unittest.TextTestRunner(verbosity=2).run(suite).wasSuccessful() else 1
    try:
        result = audit(args.root, args.expected_head)
    except (OSError, UnicodeError, ValueError, subprocess.TimeoutExpired) as exc:
        print("ERROR: " + str(exc), file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        print("Git HEAD: " + result["revision"])
        print("Production C/C++ files scanned: " + str(result["source_files_scanned"]))
        print("Lambda-shaped candidates: " + str(result["candidate_count"]))
        for entry in result["candidates"]:
            print(f"  {entry['file']}:{entry['line']}: {entry['excerpt']}")
        print("PASS: no lambda-shaped candidates" if not result["candidate_count"] else
              "REVIEW REQUIRED: inspect candidates against C++ grammar")
    return 0 if not result["candidate_count"] else 1


if __name__ == "__main__":
    sys.exit(main())
