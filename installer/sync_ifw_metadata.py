#!/usr/bin/env python3
"""Render Qt IFW metadata from the authoritative Tonatiuh++ project version."""

from __future__ import annotations

import argparse
import re
from pathlib import Path


VERSION_TOKEN = "@TONATIUHPP_VERSION@"


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def read_project_version(root: Path | None = None) -> str:
    root = root or repo_root()
    cmake_file = root / "source" / "CMakeLists.txt"
    text = cmake_file.read_text(encoding="utf-8")
    match = re.search(r"project\s*\(\s*TonatiuhPP\s+VERSION\s+([0-9][0-9.]*)\b", text)
    if not match:
        raise SystemExit(f"Could not determine Tonatiuh++ project version from {cmake_file}")
    return match.group(1)


def normalize_tag(tag: str) -> str:
    return tag[1:] if tag.startswith("v") else tag


def check_tag_matches_version(tag: str, version: str) -> None:
    normalized = normalize_tag(tag)
    if normalized != version:
        raise SystemExit(
            f"Git tag version mismatch: tag '{tag}' resolves to '{normalized}', "
            f"but project version is '{version}'."
        )


def render_version_template(src: Path, dst: Path, version: str) -> None:
    text = src.read_text(encoding="utf-8")
    rendered = text.replace(VERSION_TOKEN, version)
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_text(rendered, encoding="utf-8", newline="\n")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Read or validate the Tonatiuh++ project version for IFW packaging."
    )
    parser.add_argument(
        "--print-version",
        action="store_true",
        help="Print the authoritative Tonatiuh++ project version.",
    )
    parser.add_argument(
        "--check-tag",
        help="Validate that the given Git tag matches the project version. A leading 'v' is allowed.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    version = read_project_version()

    if args.check_tag:
        check_tag_matches_version(args.check_tag, version)

    if args.print_version:
        print(version)


if __name__ == "__main__":
    main()
