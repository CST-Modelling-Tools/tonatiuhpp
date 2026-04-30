#!/usr/bin/env python3
"""Generate Qt IFW installer for Tonatiuh++ using binarycreator."""

import argparse
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from sync_ifw_metadata import read_project_version, render_version_template

IFW_REPOSITORY_URL_TOKEN = "@TONATIUHPP_IFW_REPOSITORY_URL@"
DEFAULT_REPOSITORY_BASE_URL = "https://cst-modelling-tools.github.io/tonatiuhpp"


def detect_platform() -> str:
    if sys.platform.startswith("win"):
        return "windows"
    if sys.platform == "darwin":
        return "macos"
    return "linux"


def platform_repository_url(base_url: str, platform: str) -> str:
    return f"{base_url.rstrip('/')}/{platform}"


def parse_args() -> argparse.Namespace:
    repo_root = Path(__file__).resolve().parents[1]
    installer_dir = repo_root / "installer"
    default_config_xml = installer_dir / "config" / "config.xml"
    default_packages_dir = installer_dir / "packages"
    default_output_dir = installer_dir / "output"
    default_output_name = f"TonatiuhPP-{read_project_version(repo_root)}-Installer"

    parser = argparse.ArgumentParser(
        description="Generate Qt IFW installer for Tonatiuh++."
    )
    parser.add_argument(
        "--binarycreator",
        required=True,
        help="Path to Qt IFW binarycreator executable or command name.",
    )
    parser.add_argument(
        "--config-xml",
        default=default_config_xml,
        help="Path to Qt IFW config.xml file.",
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
        "--output-dir",
        default=default_output_dir,
        help="Directory to place the generated installer.",
    )
    parser.add_argument(
        "--output-name",
        default=default_output_name,
        help="Base name for the installer output file (without extension).",
    )
    parser.add_argument(
        "--repository-base-url",
        default=DEFAULT_REPOSITORY_BASE_URL,
        help="Base URL that contains platform IFW repositories.",
    )
    parser.add_argument(
        "--repository-platform",
        choices=["auto", "windows", "linux", "macos"],
        default="auto",
        help="Platform repository to embed in config.xml.",
    )
    parser.add_argument(
        "--repository-url",
        default=None,
        help="Explicit IFW repository URL. Overrides --repository-base-url and --repository-platform.",
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


def validate_installer_skeleton(config_xml: Path, packages_dir: Path) -> None:
    if not config_xml.exists():
        raise SystemExit(f"Qt IFW config.xml not found: {config_xml}")

    package_meta = packages_dir / "com.tonatiuh.app" / "meta" / "package.xml"
    if not package_meta.exists():
        raise SystemExit(f"Qt IFW package.xml not found: {package_meta}")

    package_data = packages_dir / "com.tonatiuh.app" / "data"
    if not package_data.exists() or not any(package_data.iterdir()):
        raise SystemExit(
            f"Qt IFW package data directory is empty: {package_data}\n"
            "Run the staging script first: python installer/prepare_ifw_payload.py"
        )


def resolve_binarycreator(binarycreator: str) -> Path:
    candidate = Path(binarycreator)
    if candidate.exists():
        return candidate.resolve()

    found = shutil.which(binarycreator)
    if found:
        return Path(found).resolve()

    raise SystemExit(f"binarycreator not found: {binarycreator}")


def render_ifw_metadata(
    config_xml_template: Path,
    packages_dir_template: Path,
    package_data_dir: Path | None,
    repository_url: str,
    verbose: bool = False,
) -> tuple[tempfile.TemporaryDirectory[str], Path, Path, str]:
    version = read_project_version(Path(__file__).resolve().parents[1])
    temp_dir = tempfile.TemporaryDirectory(prefix="tonatiuhpp-ifw-")
    temp_root = Path(temp_dir.name)
    rendered_config_xml = temp_root / "config" / "config.xml"
    rendered_packages_dir = temp_root / "packages"

    if verbose:
        print(f"Rendering Qt IFW metadata for Tonatiuh++ version {version}")

    render_version_template(config_xml_template, rendered_config_xml, version)
    config_text = rendered_config_xml.read_text(encoding="utf-8")
    if IFW_REPOSITORY_URL_TOKEN not in config_text:
        raise SystemExit(f"IFW repository URL token missing from {config_xml_template}")
    rendered_config_xml.write_text(
        config_text.replace(IFW_REPOSITORY_URL_TOKEN, repository_url),
        encoding="utf-8",
        newline="\n",
    )
    shutil.copytree(packages_dir_template, rendered_packages_dir)
    render_version_template(
        packages_dir_template / "com.tonatiuh.app" / "meta" / "package.xml",
        rendered_packages_dir / "com.tonatiuh.app" / "meta" / "package.xml",
        version,
    )
    if package_data_dir:
        rendered_data_dir = rendered_packages_dir / "com.tonatiuh.app" / "data"
        if rendered_data_dir.exists():
            shutil.rmtree(rendered_data_dir)
        shutil.copytree(package_data_dir, rendered_data_dir, symlinks=True)

    return temp_dir, rendered_config_xml, rendered_packages_dir, version


def main() -> None:
    args = parse_args()
    config_xml_template = Path(args.config_xml).resolve()
    packages_dir_template = Path(args.packages_dir).resolve()
    package_data_dir = Path(args.package_data_dir).resolve() if args.package_data_dir else None
    output_dir = Path(args.output_dir).resolve()
    binarycreator = resolve_binarycreator(args.binarycreator)
    if package_data_dir and (not package_data_dir.is_dir() or not any(package_data_dir.iterdir())):
        raise SystemExit(f"Qt IFW package data directory is empty or missing: {package_data_dir}")
    repository_platform = detect_platform() if args.repository_platform == "auto" else args.repository_platform
    repository_url = args.repository_url or platform_repository_url(args.repository_base_url, repository_platform)
    temp_dir, config_xml, packages_dir, project_version = render_ifw_metadata(
        config_xml_template, packages_dir_template, package_data_dir, repository_url, verbose=args.verbose
    )

    try:
        validate_installer_skeleton(config_xml, packages_dir)

        output_dir.mkdir(parents=True, exist_ok=True)

        output_file = output_dir / args.output_name

        cmd = [
            str(binarycreator),
            "--config", str(config_xml),
            "--packages", str(packages_dir),
            str(output_file),
        ]

        if args.verbose:
            print(f"Generating installer for project version {project_version}: {output_file}")
        run_command(cmd, verbose=args.verbose)

        print("Qt IFW installer generated successfully.")
        print(f"Project version: {project_version}")
        print(f"IFW repository URL: {repository_url}")
        print(f"Output directory: {output_dir}")
        print(f"Installer file: {output_file}")
    finally:
        temp_dir.cleanup()


if __name__ == "__main__":
    main()
