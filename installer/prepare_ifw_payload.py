#!/usr/bin/env python3
"""Prepare Qt IFW package payload from the CMake install tree."""

import argparse
import shutil
import subprocess
from pathlib import Path


def parse_args() -> argparse.Namespace:
    repo_root = Path(__file__).resolve().parents[1]
    default_build_dir = repo_root / "build"
    default_staging_dir = repo_root / "build" / "install-staging"
    package_root = repo_root / "installer" / "packages" / "com.tonatiuh.app"
    default_package_data_dir = package_root / "data"

    parser = argparse.ArgumentParser(
        description="Stage Tonatiuh++ install output into Qt IFW package data."
    )
    parser.add_argument(
        "--build-dir",
        default=default_build_dir,
        help="CMake build directory containing an existing Tonatiuh++ build.",
    )
    parser.add_argument(
        "--config",
        default=None,
        help="CMake configuration for multi-config generators (Release/Debug).",
    )
    parser.add_argument(
        "--staging-dir",
        default=default_staging_dir,
        help="Directory used for temporary install staging.",
    )
    parser.add_argument(
        "--package-data-dir",
        default=default_package_data_dir,
        help="Qt IFW package data directory to populate.",
    )
    parser.add_argument(
        "--cmake",
        default="cmake",
        help="CMake executable to use.",
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


def remove_path(path: Path, verbose: bool = False) -> None:
    if not path.exists():
        return
    if path.is_dir():
        if verbose:
            print(f"Removing directory: {path}")
        shutil.rmtree(path)
    else:
        if verbose:
            print(f"Removing file: {path}")
        path.unlink()


def copy_tree(src: Path, dst: Path, verbose: bool = False) -> None:
    if dst.exists():
        remove_path(dst, verbose=verbose)
    if verbose:
        print(f"Copying staged install tree: {src} -> {dst}")
    shutil.copytree(src, dst, symlinks=True)


def main() -> None:
    args = parse_args()
    repo_root = Path(__file__).resolve().parents[1]
    build_dir = Path(args.build_dir).resolve()
    staging_dir = Path(args.staging_dir).resolve()
    package_data_dir = Path(args.package_data_dir).resolve()
    package_root = repo_root / "installer" / "packages" / "com.tonatiuh.app"
    default_package_data_dir = package_root / "data"

    if not build_dir.exists():
        raise SystemExit(
            f"Build directory not found: {build_dir}\n"
            "Create the CMake build tree first, for example: `cmake -S source -B build`."
        )

    if not (package_data_dir == default_package_data_dir or default_package_data_dir in package_data_dir.parents):
        raise SystemExit(
            f"Package data directory must be the intended IFW data path or a subdirectory of it: {default_package_data_dir}"
        )

    if package_root not in package_data_dir.parents and package_data_dir != package_root:
        raise SystemExit(
            f"Package data directory must reside under {package_root}: {package_data_dir}"
        )

    remove_path(package_data_dir, verbose=args.verbose)
    package_data_dir.mkdir(parents=True, exist_ok=True)

    remove_path(staging_dir, verbose=args.verbose)
    staging_dir.mkdir(parents=True, exist_ok=True)

    build_command = [args.cmake, "--build", str(build_dir)]
    if args.config:
        build_command += ["--config", args.config]
    run_command(build_command, verbose=args.verbose)

    install_command = [args.cmake, "--install", str(build_dir), "--prefix", str(staging_dir)]
    if args.config:
        install_command += ["--config", args.config]
    run_command(install_command, verbose=args.verbose)

    copy_tree(staging_dir, package_data_dir, verbose=args.verbose)

    print("Qt IFW package data staged successfully.")
    print(f"Staging directory: {staging_dir}")
    print(f"Package data directory: {package_data_dir}")


if __name__ == "__main__":
    main()
