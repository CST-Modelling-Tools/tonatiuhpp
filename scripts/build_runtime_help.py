#!/usr/bin/env python3
"""Build the installed GUI help from the Sphinx source tree."""

from __future__ import annotations

import argparse
import importlib.util
import shutil
import subprocess
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(
        description="Build help/Sphinx into help/html for Tonatiuh++ runtime help."
    )
    parser.add_argument(
        "--source-dir",
        type=Path,
        default=root / "help" / "Sphinx",
        help="Sphinx source directory.",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=root / "help" / "html",
        help="HTML output directory used by the Tonatiuh++ GUI.",
    )
    parser.add_argument(
        "--doctree-dir",
        type=Path,
        default=root / "help" / "Sphinx" / "_build" / "doctrees",
        help="Sphinx doctree cache directory.",
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Remove the HTML output directory before building.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    source_dir = args.source_dir.resolve()
    output_dir = args.output_dir.resolve()
    doctree_dir = args.doctree_dir.resolve()

    if not source_dir.is_dir():
        print(f"Sphinx source directory not found: {source_dir}", file=sys.stderr)
        return 2

    if importlib.util.find_spec("sphinx") is None:
        print(
            "Sphinx is not installed. Install it with: python -m pip install \"sphinx>=7,<9\"",
            file=sys.stderr,
        )
        return 4

    if args.clean and output_dir.exists():
        shutil.rmtree(output_dir)

    output_dir.mkdir(parents=True, exist_ok=True)
    doctree_dir.mkdir(parents=True, exist_ok=True)

    command = [
        sys.executable,
        "-m",
        "sphinx",
        "-b",
        "html",
        "-d",
        str(doctree_dir),
        str(source_dir),
        str(output_dir),
    ]

    try:
        subprocess.run(command, check=True)
    except subprocess.CalledProcessError as exc:
        return exc.returncode

    index_file = output_dir / "index.html"
    if not index_file.is_file():
        print(f"Expected Sphinx output missing: {index_file}", file=sys.stderr)
        return 3

    print(f"Runtime help built: {index_file}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
