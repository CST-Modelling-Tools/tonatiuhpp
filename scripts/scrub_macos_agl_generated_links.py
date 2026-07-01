#!/usr/bin/env python3
"""Remove stale AGL framework references from generated macOS build files."""

from __future__ import annotations

import argparse
import os
import platform
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
AGL_REFERENCE_RE = re.compile(r"\bAGL\b|AGL\.framework")


def _display_path(path: Path) -> str:
    try:
        return str(path.resolve().relative_to(ROOT))
    except ValueError:
        return str(path)


def _remove_macos_agl_link_flags(text: str) -> str:
    """Remove AGL framework link flags while preserving other link flags."""
    cleaned = text

    # CMake list forms such as "-framework;AGL".
    for token in (
        ";-framework;AGL",
        "-framework;AGL;",
        "-framework;AGL",
        '";-framework;AGL"',
        '"-framework;AGL"',
    ):
        cleaned = cleaned.replace(token, "")

    replacements = [
        r"(^|[\s;=\"'])\"?(?:(?:LINKER:)?SHELL:)?-framework(?:\s+|\\ |\$ |[ =])AGL\"?(?=$|[\s;,\"'])",
        r"(^|[\s;=\"'])\"?(?:LINKER:)?-Wl,-framework,AGL\"?(?=$|[\s;,\"'])",
        r"(^|[\s;=\"'])\"?LINKER:-framework,AGL\"?(?=$|[\s;,\"'])",
    ]
    for pattern in replacements:
        cleaned = re.sub(pattern, r"\1", cleaned)

    cleaned = re.sub(
        r"(^|[=;\s])[\"']?(?:[^ \t\r\n;\"']*/)?AGL\.framework(?:/[^ \t\r\n;\"']*)?[\"']?",
        r"\1",
        cleaned,
        flags=re.MULTILINE,
    )

    return cleaned


def _generated_main_build_link_files(build_dir: Path) -> list[Path]:
    candidates: list[Path] = []

    def add(path: Path) -> None:
        if path.is_file() and path not in candidates:
            candidates.append(path)

    add(build_dir / "build.ninja")
    add(build_dir / "SunPath" / "CMakeFiles" / "SunPath.dir" / "link.txt")

    if build_dir.exists():
        for pattern in ("*.ninja", "link.txt"):
            for path in build_dir.rglob(pattern):
                add(path)

        cmake_files_dir = build_dir / "CMakeFiles"
        if cmake_files_dir.exists():
            for path in cmake_files_dir.rglob("*.cmake"):
                add(path)

    return candidates


def _find_agl_references(paths: list[Path]) -> list[tuple[Path, list[str]]]:
    matches: list[tuple[Path, list[str]]] = []
    for path in paths:
        try:
            lines = path.read_text(encoding="utf-8").splitlines()
        except (OSError, UnicodeDecodeError):
            continue

        agl_lines = []
        for line in lines:
            if AGL_REFERENCE_RE.search(line):
                agl_lines.append(line.strip())

        if agl_lines:
            matches.append((path, agl_lines[:3]))

    return matches


def scrub_generated_macos_agl_references(build_dir: Path) -> int:
    if not build_dir.exists():
        print(f"[main-build] Build directory not found: {_display_path(build_dir)}", file=sys.stderr)
        return 2

    candidates = _generated_main_build_link_files(build_dir)
    modified: list[Path] = []

    for path in candidates:
        try:
            original = path.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError):
            continue

        cleaned = _remove_macos_agl_link_flags(original)
        if cleaned != original:
            path.write_text(cleaned, encoding="utf-8")
            modified.append(path)

    if modified:
        ninja_manifest = build_dir / "build.ninja"
        if ninja_manifest.exists():
            # Keep the post-configure scrubbed Ninja manifest current.
            os.utime(ninja_manifest, None)

        print("[main-build] Removed unavailable AGL framework references from generated build files:")
        for path in modified:
            print(f"  - {_display_path(path)}")

    remaining = _find_agl_references(candidates)
    if remaining:
        print("[main-build] AGL references remain in generated build/link files:", file=sys.stderr)
        for path, lines in remaining:
            print(f"  - {_display_path(path)}", file=sys.stderr)
            for line in lines:
                print(f"      {line}", file=sys.stderr)
        return 1

    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Scrub unavailable AGL framework references from generated macOS build files."
    )
    parser.add_argument("--build-dir", default="build", help="Main CMake build directory.")
    parser.add_argument("--force", action="store_true", help="Run even when the host is not macOS.")
    args = parser.parse_args(argv)

    if platform.system() != "Darwin" and not args.force:
        return 0

    build_dir = Path(args.build_dir)
    if not build_dir.is_absolute():
        build_dir = ROOT / build_dir

    return scrub_generated_macos_agl_references(build_dir)


if __name__ == "__main__":
    raise SystemExit(main())
