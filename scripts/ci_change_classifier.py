#!/usr/bin/env python3
"""Conservatively select lightweight CI for documentation-only diffs.

Input is git diff --name-only --no-renames -z. Anything other than an
exclusively Markdown (.md) diff requires the full native CI matrix.
"""

import argparse
from pathlib import Path
import sys


def read_changed_paths(path):
    data = Path(path).read_bytes()
    if not data:
        return []
    if not data.endswith(b"\0"):
        raise ValueError("Changed-path input must be NUL-terminated")
    return [entry.decode("utf-8") for entry in data[:-1].split(b"\0")]


def is_docs_only(paths):
    return bool(paths) and all(
        path.endswith(".md") and path not in (".", "..")
        and not path.startswith("../") and not Path(path).is_absolute()
        for path in paths
    )


def check_docs(paths, repo_root):
    if not is_docs_only(paths):
        raise ValueError("Documentation check requires a Markdown-only diff")
    checked = 0
    for name in paths:
        file = repo_root / name
        if file.is_symlink():
            raise ValueError(f"Not a regular documentation file: {name}")
        if not file.exists():
            # Deleting a Markdown document is still a documentation-only change.
            continue
        if not file.is_file():
            raise ValueError(f"Not a regular documentation file: {name}")
        content = file.read_text(encoding="utf-8")
        if any(line.startswith(("<<<<<<< ", ">>>>>>> "))
               for line in content.splitlines()):
            raise ValueError(f"Unresolved merge conflict marker in {name}")
        checked += 1
    return checked


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("changed_paths", type=Path,
                        help="NUL-delimited changed paths produced by git diff -z")
    parser.add_argument("--check-docs", action="store_true",
                        help="Check changed Markdown files for UTF-8 and merge markers")
    args = parser.parse_args(argv)
    paths = read_changed_paths(args.changed_paths)
    if args.check_docs:
        checked = check_docs(paths, Path(__file__).resolve().parents[1])
        print(f"PASS: Markdown-only change; verified {checked} existing document(s)")
    else:
        print(f"docs_only={'true' if is_docs_only(paths) else 'false'}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (ValueError, OSError, UnicodeError) as exc:
        sys.exit(f"CI change classification failed: {exc}")
