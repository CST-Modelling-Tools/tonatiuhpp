#!/usr/bin/env python3
"""Generate Qt IFW installer for Tonatiuh++ using binarycreator."""

import argparse
import shutil
import subprocess
from pathlib import Path


def parse_args() -> argparse.Namespace:
    repo_root = Path(__file__).resolve().parents[1]
    installer_dir = repo_root / "installer"
    default_config_xml = installer_dir / "config" / "config.xml"
    default_packages_dir = installer_dir / "packages"
    default_output_dir = installer_dir / "output"
    default_output_name = "TonatiuhPP-Installer"

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


def main() -> None:
    args = parse_args()
    config_xml = Path(args.config_xml).resolve()
    packages_dir = Path(args.packages_dir).resolve()
    output_dir = Path(args.output_dir).resolve()
    binarycreator = resolve_binarycreator(args.binarycreator)

    validate_installer_skeleton(config_xml, packages_dir)

    output_dir.mkdir(parents=True, exist_ok=True)

    # Determine output file path; let binarycreator add platform-specific extension
    output_file = output_dir / args.output_name

    cmd = [
        str(binarycreator),
        "--config", str(config_xml),
        "--packages", str(packages_dir),
        str(output_file),
    ]

    if args.verbose:
        print(f"Generating installer: {output_file}")
    run_command(cmd, verbose=args.verbose)

    print("Qt IFW installer generated successfully.")
    print(f"Output directory: {output_dir}")
    print(f"Installer file: {output_file} (with platform-specific extension)")


if __name__ == "__main__":
    main()
