#!/usr/bin/env python3
"""Generate a Qt IFW online repository for Tonatiuh++."""

import argparse
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path

from sync_ifw_metadata import (
    default_release_date,
    read_project_version,
    render_ifw_template,
    validate_release_date,
)

PACKAGE_ID = "com.tonatiuhpp.app"
APPLICATION_NAME = "Tonatiuh++"


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
        help=f"Optional package data directory to use instead of packages/{PACKAGE_ID}/data.",
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
        "--release-date",
        default=default_release_date(),
        help="Release date to write to IFW package metadata in YYYY-MM-DD format.",
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
    package_root = packages_dir / PACKAGE_ID
    package_meta = package_root / "meta" / "package.xml"
    package_data = package_root / "data"

    if not package_meta.exists():
        raise SystemExit(f"Qt IFW package.xml not found: {package_meta}")
    if not package_data.exists() or not any(package_data.iterdir()):
        raise SystemExit(f"Qt IFW package data directory is empty: {package_data}")


def render_packages(
    packages_dir_template: Path,
    package_data_dir: Path | None,
    release_date: str,
    verbose: bool = False,
) -> tuple[tempfile.TemporaryDirectory[str], Path, str]:
    version = read_project_version(Path(__file__).resolve().parents[1])
    temp_dir = tempfile.TemporaryDirectory(prefix="tonatiuhpp-ifw-repo-")
    rendered_packages_dir = Path(temp_dir.name) / "packages"

    if verbose:
        print(
            f"Rendering Qt IFW packages for Tonatiuh++ version {version} "
            f"released on {release_date}"
        )

    shutil.copytree(packages_dir_template, rendered_packages_dir, symlinks=True)
    render_ifw_template(
        packages_dir_template / PACKAGE_ID / "meta" / "package.xml",
        rendered_packages_dir / PACKAGE_ID / "meta" / "package.xml",
        version,
        release_date,
    )

    if package_data_dir:
        rendered_data_dir = rendered_packages_dir / PACKAGE_ID / "data"
        remove_path(rendered_data_dir, verbose=verbose)
        shutil.copytree(package_data_dir, rendered_data_dir, symlinks=True)

    return temp_dir, rendered_packages_dir, version


def set_child_text(parent: ET.Element, tag: str, text: str, before_tag: str) -> None:
    child = parent.find(tag)
    if child is None:
        child = ET.Element(tag)
        children = list(parent)
        for index, existing_child in enumerate(children):
            if existing_child.tag == before_tag:
                parent.insert(index, child)
                break
        else:
            parent.append(child)
    child.text = text


def normalize_updates_xml(updates_xml: Path, version: str, release_date: str) -> None:
    tree = ET.parse(updates_xml)
    root = tree.getroot()
    if root.tag != "Updates":
        raise SystemExit(f"Unexpected IFW Updates.xml root: {root.tag}")

    set_child_text(root, "ApplicationName", APPLICATION_NAME, "PackageUpdate")
    set_child_text(root, "ApplicationVersion", version, "PackageUpdate")

    package_updates = root.findall("PackageUpdate")
    if len(package_updates) != 1:
        raise SystemExit(
            f"Expected exactly one IFW PackageUpdate, found {len(package_updates)}"
        )

    package_update = package_updates[0]
    package_name = package_update.findtext("Name")
    if package_name != PACKAGE_ID:
        raise SystemExit(
            f"Unexpected IFW package name in Updates.xml: {package_name}"
        )

    set_child_text(package_update, "ReleaseDate", release_date, "Default")

    ET.indent(tree, space=" ")
    tree.write(updates_xml, encoding="utf-8", xml_declaration=True)


def main() -> None:
    args = parse_args()
    platform = detect_platform() if args.platform == "auto" else args.platform
    packages_dir_template = Path(args.packages_dir).resolve()
    package_data_dir = Path(args.package_data_dir).resolve() if args.package_data_dir else None
    repository_dir = Path(args.repository_dir).resolve()
    release_date = validate_release_date(args.release_date)
    repogen = resolve_repogen(args.repogen)

    temp_dir, packages_dir, project_version = render_packages(
        packages_dir_template, package_data_dir, release_date, verbose=args.verbose
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
        normalize_updates_xml(updates_xml, project_version, release_date)

        print("Qt IFW repository generated successfully.")
        print(f"Platform: {platform}")
        print(f"Project version: {project_version}")
        print(f"Release date: {release_date}")
        print(f"Repository directory: {repository_dir}")
    finally:
        temp_dir.cleanup()


if __name__ == "__main__":
    main()
