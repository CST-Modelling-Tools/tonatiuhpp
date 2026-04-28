#!/usr/bin/env python3
"""Generate a Qt IFW online repository for Tonatiuh++."""

import argparse
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from sync_ifw_metadata import read_project_version, render_version_template


def detect_platform() -> str:
    if sys.platform.startswith("win"):
        return "windows"
    if sys.platform == "darwin":
        return "macos"
    return "linux"


def parse_args() -> argparse.Namespace:
    repo_root = Path(__file__).resolve().parents[1]
    installer_dir = repo_root / "installer"
    default_packages_dir = installer_dir / "packages"

    parser = argparse.ArgumentParser(
        description="Generate a platform Qt IFW online repository for Tonatiuh++."
    )
    parser.add_argument(
        "--repogen",
        required=True,
        help="Path to Qt IFW repogen executable or command name.",
    )
    parser.add_argument(
        "--packages-dir",
        default=default_packages_dir,
        help="Path to Qt IFW packages directory.",
    )
    parser.add_argument(
        "--package-data-dir",
        default=None,
        help="Optional package data directory to use instead of packages/com.tonatiuh.app/data.",
    )
    parser.add_argument(
        "--repository-dir",
        required=True,
        help="Directory where repogen will create the online repository.",
    )
    parser.add_argument(
        "--platform",
        choices=["auto", "windows", "linux", "macos"],
        default="auto",
        help="Platform label for logging and CI artifact layout.",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Print detailed status messages.",
    )
    return parser.parse_args()


def run_command(cmd: list[str], verbose: bool = False) -> None:
    if verbose:
        print("Running:", " ".join(str(x) for x in cmd))
    subprocess.run(cmd, check=True)


def resolve_repogen(repogen: str) -> Path:
    candidate = Path(repogen)
    if candidate.exists():
        return candidate.resolve()

    found = shutil.which(repogen)
    if found:
        return Path(found).resolve()

    raise SystemExit(f"repogen not found: {repogen}")


def remove_path(path: Path, verbose: bool = False) -> None:
    if not path.exists():
        return
    if verbose:
        print(f"Removing: {path}")
    if path.is_dir():
        shutil.rmtree(path)
    else:
        path.unlink()


def validate_packages(packages_dir: Path) -> None:
    package_meta = packages_dir / "com.tonatiuh.app" / "meta" / "package.xml"
    package_data = packages_dir / "com.tonatiuh.app" / "data"

    if not package_meta.exists():
        raise SystemExit(f"Qt IFW package.xml not found: {package_meta}")
    if not package_data.exists() or not any(package_data.iterdir()):
        raise SystemExit(f"Qt IFW package data directory is empty: {package_data}")


def render_packages(
    packages_dir_template: Path,
    package_data_dir: Path | None,
    verbose: bool = False,
) -> tuple[tempfile.TemporaryDirectory[str], Path, str]:
    version = read_project_version(Path(__file__).resolve().parents[1])
    temp_dir = tempfile.TemporaryDirectory(prefix="tonatiuhpp-ifw-repo-")
    rendered_packages_dir = Path(temp_dir.name) / "packages"

    if verbose:
        print(f"Rendering Qt IFW packages for Tonatiuh++ version {version}")

    shutil.copytree(packages_dir_template, rendered_packages_dir, symlinks=True)
    render_version_template(
        packages_dir_template / "com.tonatiuh.app" / "meta" / "package.xml",
        rendered_packages_dir / "com.tonatiuh.app" / "meta" / "package.xml",
        version,
    )

    if package_data_dir:
        rendered_data_dir = rendered_packages_dir / "com.tonatiuh.app" / "data"
        remove_path(rendered_data_dir, verbose=verbose)
        shutil.copytree(package_data_dir, rendered_data_dir, symlinks=True)

    return temp_dir, rendered_packages_dir, version


def main() -> None:
    args = parse_args()
    platform = detect_platform() if args.platform == "auto" else args.platform
    packages_dir_template = Path(args.packages_dir).resolve()
    package_data_dir = Path(args.package_data_dir).resolve() if args.package_data_dir else None
    repository_dir = Path(args.repository_dir).resolve()
    repogen = resolve_repogen(args.repogen)

    temp_dir, packages_dir, project_version = render_packages(
        packages_dir_template, package_data_dir, verbose=args.verbose
    )

    try:
        validate_packages(packages_dir)
        remove_path(repository_dir, verbose=args.verbose)
        repository_dir.parent.mkdir(parents=True, exist_ok=True)

        cmd = [
            str(repogen),
            "-p",
            str(packages_dir),
            str(repository_dir),
        ]
        run_command(cmd, verbose=args.verbose)

        updates_xml = repository_dir / "Updates.xml"
        if not updates_xml.exists():
            raise SystemExit(f"repogen did not produce Updates.xml: {updates_xml}")

        print("Qt IFW repository generated successfully.")
        print(f"Platform: {platform}")
        print(f"Project version: {project_version}")
        print(f"Repository directory: {repository_dir}")
    finally:
        temp_dir.cleanup()


if __name__ == "__main__":
    main()
