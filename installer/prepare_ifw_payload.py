#!/usr/bin/env python3
"""Prepare Qt IFW package payload from the CMake install tree."""

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

PACKAGE_ID = "com.tonatiuhpp.app"


def parse_args() -> argparse.Namespace:
    repo_root = Path(__file__).resolve().parents[1]
    default_build_dir = repo_root / "build"
    default_staging_dir = repo_root / "build" / "install-staging"
    package_root = repo_root / "installer" / "packages" / PACKAGE_ID
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
        "--windeployqt",
        default=None,
        help="Explicit path to windeployqt executable.",
    )
    parser.add_argument(
        "--skip-build",
        action="store_true",
        help="Skip CMake build step and use an existing build tree.",
    )
    parser.add_argument(
        "--skip-install",
        action="store_true",
        help="Skip CMake install step and use an existing staged install tree.",
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


def resolve_tool(tool_name: str) -> Path:
    candidate = Path(tool_name)
    if candidate.exists():
        return candidate.resolve()
    resolved = shutil.which(tool_name)
    if resolved:
        return Path(resolved).resolve()
    raise SystemExit(f"Required tool not found: {tool_name}")


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


def find_staged_application(staging_dir: Path) -> tuple[Path, bool]:
    # Search recursively for TonatiuhPP.app bundle
    for app_bundle in staging_dir.rglob("TonatiuhPP.app"):
        if app_bundle.is_dir():
            return app_bundle, True

    bin_dir = staging_dir / "bin"
    candidates = [
        bin_dir / "tonatiuhpp.exe",
        bin_dir / "TonatiuhPP.exe",
        bin_dir / "tonatiuhpp",
        bin_dir / "TonatiuhPP",
    ]

    for candidate in candidates:
        if candidate.exists():
            return candidate, False

    raise SystemExit(
        f"Staged Tonatiuh++ executable not found in {staging_dir}.\n"
        "Ensure the CMake install step produced the application under staging/bin/ or as TonatiuhPP.app."
    )


def deploy_windows(staged_exe: Path, windeployqt_path: str | None = None, verbose: bool = False) -> None:
    if windeployqt_path:
        windeployqt = Path(windeployqt_path)
        if not windeployqt.exists():
            raise SystemExit(f"windeployqt not found at: {windeployqt}")
    else:
        windeployqt = resolve_tool("windeployqt")

    target_dir = staged_exe.parent
    cmd = [
        str(windeployqt),
        "--dir",
        str(target_dir),
        "--no-translations",
        str(staged_exe),
    ]
    if verbose:
        cmd += ["--verbose", "1"]
    run_command(cmd, verbose=verbose)


def deploy_macos(app_bundle: Path, verbose: bool = False) -> None:
    macdeployqt = resolve_tool("macdeployqt")
    cmd = [str(macdeployqt), str(app_bundle)]
    if verbose:
        cmd.append("-verbose=1")
    run_command(cmd, verbose=verbose)


def prune_windows_payload(staging_dir: Path, verbose: bool = False) -> None:
    removed_any = False

    for import_lib in staging_dir.rglob("*.lib"):
        remove_path(import_lib, verbose=verbose)
        removed_any = True

    qmltooling_dir = staging_dir / "bin" / "qmltooling"
    if qmltooling_dir.exists():
        remove_path(qmltooling_dir, verbose=verbose)
        removed_any = True

    qml_dir = staging_dir / "bin" / "qml"
    if qml_dir.exists() and not any(qml_dir.iterdir()):
        remove_path(qml_dir, verbose=verbose)
        removed_any = True

    if verbose and not removed_any:
        print("No removable Windows payload artifacts were found.")


def verify_linux_bundling(staging_dir: Path, verbose: bool = False) -> None:
    lib_dir = staging_dir / "lib"
    platforms_dir = staging_dir / "bin" / "platforms"
    qt_lib_found = False
    if lib_dir.exists():
        for item in lib_dir.iterdir():
            if item.name.startswith("libQt6"):
                qt_lib_found = True
                break

    if not qt_lib_found:
        raise SystemExit(
            "Linux staging did not include bundled Qt runtime libraries.\n"
            "Ensure Linux Qt deployment is enabled in the CMake install step and rerun the staging script."
        )

    if not platforms_dir.exists():
        raise SystemExit(
            "Linux staging did not include Qt platform plugins.\n"
            "Ensure Linux Qt deployment is enabled in the CMake install step and rerun the staging script."
        )

    if verbose:
        print("Linux Qt runtime deployment appears present in staging.")


def main() -> None:
    args = parse_args()
    repo_root = Path(__file__).resolve().parents[1]
    build_dir = Path(args.build_dir).resolve()
    staging_dir = Path(args.staging_dir).resolve()
    package_data_dir = Path(args.package_data_dir).resolve()
    package_root = repo_root / "installer" / "packages" / PACKAGE_ID
    default_package_data_dir = package_root / "data"

    if not build_dir.exists():
        raise SystemExit(
            f"Build directory not found: {build_dir}\n"
            "Create the CMake build tree first, for example: `cmake -S source -B build`."
        )

    if args.skip_install and not staging_dir.exists():
        raise SystemExit(
            f"Staging directory must exist when --skip-install is used: {staging_dir}"
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

    if not args.skip_install:
        remove_path(staging_dir, verbose=args.verbose)
        staging_dir.mkdir(parents=True, exist_ok=True)
    elif not staging_dir.exists():
        raise SystemExit(
            f"Staging directory must exist when --skip-install is used: {staging_dir}"
        )

    if not args.skip_build:
        build_command = [args.cmake, "--build", str(build_dir)]
        if args.config:
            build_command += ["--config", args.config]
        run_command(build_command, verbose=args.verbose)
    elif args.verbose:
        print("Skipping build step as requested.")

    if not args.skip_install:
        install_command = [args.cmake, "--install", str(build_dir), "--prefix", str(staging_dir)]
        if args.config:
            install_command += ["--config", args.config]
        run_command(install_command, verbose=args.verbose)
    elif args.verbose:
        print("Skipping install step as requested; using existing staging tree.")

    staged_target, is_bundle = find_staged_application(staging_dir)
    if sys.platform.startswith("win"):
        if args.verbose:
            print(f"Deploying Qt runtime on Windows for {staged_target}")
        deploy_windows(staged_target, args.windeployqt, verbose=args.verbose)
        if args.verbose:
            print("Pruning Windows staging payload")
        prune_windows_payload(staging_dir, verbose=args.verbose)
    elif sys.platform == "darwin":
        if not is_bundle:
            raise SystemExit(
                "macOS runtime deployment requires a .app bundle."
                " Current staging did not produce TonatiuhPP.app."
            )
        if args.verbose:
            print(f"Deploying Qt runtime on macOS for {staged_target}")
        deploy_macos(staged_target, verbose=args.verbose)
    else:
        if args.verbose:
            print("Verifying Linux Qt runtime bundling in staged output")
        verify_linux_bundling(staging_dir, verbose=args.verbose)

    copy_tree(staging_dir, package_data_dir, verbose=args.verbose)

    print("Qt IFW package data staged successfully.")
    print(f"Staging directory: {staging_dir}")
    print(f"Package data directory: {package_data_dir}")


if __name__ == "__main__":
    main()
