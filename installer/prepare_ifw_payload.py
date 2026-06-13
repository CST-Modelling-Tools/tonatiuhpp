#!/usr/bin/env python3
"""Prepare Qt IFW package payload from the CMake install tree."""

import argparse
import os
import plistlib
import re
import shutil
import subprocess
import sys
from pathlib import Path

PACKAGE_ID = "com.tonatiuhpp.app"
MACOS_APP_BUNDLE_NAME = "TonatiuhPP.app"
LINUX_QT_PLUGIN_DIRECTORIES = (
    "platforms",
    "xcbglintegrations",
    "imageformats",
    "tls",
    "iconengines",
    "platformthemes",
    "styles",
    "generic",
    "networkinformation",
    "egldeviceintegrations",
    "wayland-decoration-client",
    "wayland-graphics-integration-client",
    "wayland-shell-integration",
    "sqldrivers",
)


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


def find_macos_app_bundle(staging_dir: Path) -> Path:
    app_bundles = [path for path in staging_dir.rglob(MACOS_APP_BUNDLE_NAME) if path.is_dir()]
    if len(app_bundles) != 1:
        found = sorted(str(path.relative_to(staging_dir)) for path in staging_dir.rglob("*.app") if path.is_dir())
        details = "\n".join(f"  - {path}" for path in found) or "  <none>"
        raise SystemExit(
            f"Expected exactly one macOS app bundle named {MACOS_APP_BUNDLE_NAME} in {staging_dir}.\n"
            f"Found app bundles:\n{details}"
        )
    return app_bundles[0]


def validate_macos_app_bundle(app_bundle: Path) -> None:
    contents_dir = app_bundle / "Contents"
    info_plist = contents_dir / "Info.plist"
    macos_dir = contents_dir / "MacOS"
    if not contents_dir.is_dir():
        raise SystemExit(f"macOS app bundle is missing Contents directory: {app_bundle}")
    if not info_plist.is_file():
        raise SystemExit(f"macOS app bundle is missing Info.plist: {info_plist}")
    if not macos_dir.is_dir():
        raise SystemExit(f"macOS app bundle is missing Contents/MacOS directory: {macos_dir}")

    try:
        info = plistlib.loads(info_plist.read_bytes())
    except Exception as exc:
        raise SystemExit(f"macOS Info.plist is not readable: {info_plist}: {exc}") from exc

    executable_name = info.get("CFBundleExecutable")
    if not executable_name:
        raise SystemExit(f"macOS Info.plist is missing CFBundleExecutable: {info_plist}")

    executable_path = macos_dir / executable_name
    if not executable_path.is_file():
        raise SystemExit(f"macOS bundle executable is missing: {executable_path}")
    if not os.access(executable_path, os.X_OK):
        raise SystemExit(f"macOS bundle executable is not executable: {executable_path}")


def find_staged_application(staging_dir: Path) -> tuple[Path, bool]:
    if sys.platform == "darwin":
        app_bundle = find_macos_app_bundle(staging_dir)
        validate_macos_app_bundle(app_bundle)
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
        f"Ensure the CMake install step produced the application under staging/bin/ or as {MACOS_APP_BUNDLE_NAME}."
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


def qt_version_to_integer(version: str) -> int:
    match = re.match(r"^(\d+)\.(\d+)\.(\d+)", version)
    if not match:
        raise SystemExit(f"Invalid Qt version marker: {version!r}")
    major, minor, patch = (int(part) for part in match.groups())
    return (major << 16) | (minor << 8) | patch


def require_relative_to(path: Path, parent: Path, description: str) -> None:
    resolved_path = path.resolve()
    resolved_parent = parent.resolve()
    try:
        resolved_path.relative_to(resolved_parent)
    except ValueError as exc:
        raise SystemExit(
            f"{description} resolves outside the bundled Linux runtime:\n"
            f"  dependency: {resolved_path}\n"
            f"  bundled lib dir: {resolved_parent}"
        ) from exc


def run_linux_ldd(path: Path, lib_dir: Path) -> str:
    ldd = shutil.which("ldd")
    if not ldd:
        raise SystemExit("Linux payload validation requires ldd.")

    env = os.environ.copy()
    env["LD_LIBRARY_PATH"] = f"{lib_dir}{os.pathsep}{env.get('LD_LIBRARY_PATH', '')}".rstrip(os.pathsep)
    result = subprocess.run(
        [ldd, str(path)],
        check=False,
        capture_output=True,
        env=env,
        text=True,
    )
    output = f"{result.stdout}\n{result.stderr}".strip()
    if result.returncode != 0:
        raise SystemExit(f"ldd failed for {path}:\n{output}")
    return output


def verify_linux_qt_links(
    path: Path,
    lib_dir: Path,
    require_qt_dependency: bool = False,
) -> None:
    output = run_linux_ldd(path, lib_dir)
    qt_dependency_seen = False
    for line in output.splitlines():
        if "libQt6" not in line:
            continue
        qt_dependency_seen = True
        if "not found" in line:
            raise SystemExit(f"Bundled Linux payload has an unresolved Qt dependency in {path}:\n{line}")
        if "=>" not in line:
            continue
        resolved = line.split("=>", 1)[1].strip().split(" ", 1)[0]
        if not resolved.startswith("/"):
            continue
        require_relative_to(Path(resolved), lib_dir, f"Qt dependency for {path}")

    if require_qt_dependency and not qt_dependency_seen:
        raise SystemExit(
            f"Bundled Linux Qt plugin does not link against Qt 6 runtime libraries: {path}"
        )


def find_linux_qt_plugin_files(bin_dir: Path) -> list[Path]:
    plugin_files: list[Path] = []
    for plugin_dir_name in LINUX_QT_PLUGIN_DIRECTORIES:
        plugin_dir = bin_dir / plugin_dir_name
        if plugin_dir.is_dir():
            plugin_files.extend(
                sorted(path for path in plugin_dir.rglob("*.so") if path.is_file())
            )
    return plugin_files


def find_linux_bundled_so_files(bin_dir: Path) -> list[Path]:
    return sorted(path for path in bin_dir.rglob("*.so") if path.is_file())


def verify_linux_qt_plugin_versions(plugin_files: list[Path], qt_version: str) -> None:
    expected_version = qt_version_to_integer(qt_version)
    checked = 0
    mismatches: list[str] = []
    version_re = re.compile(rb'"version"\s*:\s*(\d+)')

    for plugin in plugin_files:
        data = plugin.read_bytes()
        versions = {int(match.group(1)) for match in version_re.finditer(data)}
        if not versions:
            continue
        checked += 1
        if expected_version not in versions:
            shown_versions = ", ".join(str(version) for version in sorted(versions))
            mismatches.append(f"  - {plugin}: plugin metadata version(s) {shown_versions}")

    if mismatches:
        details = "\n".join(mismatches)
        raise SystemExit(
            f"Bundled Linux Qt plugin version mismatch; expected Qt {qt_version} "
            f"metadata value {expected_version}.\n{details}"
        )
    if checked == 0:
        print(
            "Warning: no Qt plugin metadata version strings were found in the bundled Linux plugins; "
            "relying on bundled plugin layout and ldd Qt dependency validation.",
            file=sys.stderr,
        )


def verify_linux_bundling(staging_dir: Path, verbose: bool = False) -> None:
    bin_dir = staging_dir / "bin"
    lib_dir = staging_dir / "lib"
    launcher = bin_dir / "tonatiuhpp"
    real_binary = bin_dir / "tonatiuhpp-bin"
    qt_conf = bin_dir / "qt.conf"
    qt_version_marker = bin_dir / "qt-runtime-version.txt"
    platform_plugin = bin_dir / "platforms" / "libqxcb.so"
    qt_core = lib_dir / "libQt6Core.so.6"

    if not launcher.exists():
        raise SystemExit(
            "Linux staging did not include the Tonatiuh++ launcher script.\n"
            "Ensure the CMake install step installed bin/tonatiuhpp."
        )
    if not real_binary.exists():
        raise SystemExit(
            "Linux staging did not include the real Tonatiuh++ binary.\n"
            "Ensure the CMake install step installed bin/tonatiuhpp-bin."
        )
    if not qt_conf.exists():
        raise SystemExit(
            "Linux staging did not include bin/qt.conf.\n"
            "The release payload must point Qt at the bundled plugin directory."
        )
    if not qt_core.exists():
        raise SystemExit(
            "Linux staging did not include bundled Qt runtime library lib/libQt6Core.so.6."
        )
    if not platform_plugin.exists():
        raise SystemExit(
            "Linux staging did not include bundled Qt xcb platform plugin "
            "bin/platforms/libqxcb.so."
        )
    if not qt_version_marker.exists():
        raise SystemExit(
            "Linux staging did not include bin/qt-runtime-version.txt."
        )

    qt_conf_text = qt_conf.read_text(encoding="utf-8", errors="replace")
    if not re.search(r"(?im)^\s*Prefix\s*=\s*\.\.\s*$", qt_conf_text):
        raise SystemExit("Linux bin/qt.conf must set Prefix=..")
    if not re.search(rf"(?im)^\s*Plugins\s*=\s*{re.escape(bin_dir.name)}\s*$", qt_conf_text):
        raise SystemExit(f"Linux bin/qt.conf must set Plugins={bin_dir.name}")
    if not re.search(rf"(?im)^\s*Libraries\s*=\s*{re.escape(lib_dir.name)}\s*$", qt_conf_text):
        raise SystemExit(f"Linux bin/qt.conf must set Libraries={lib_dir.name}")

    launcher_text = launcher.read_text(encoding="utf-8", errors="replace")
    if "QT_BUNDLED_PLUGIN_DIR" not in launcher_text:
        raise SystemExit(
            "Linux launcher does not configure the bundled Qt plugin directory.\n"
            "Ensure scripts/tonatiuhpp.sh is installed as bin/tonatiuhpp."
        )
    if "/usr/lib" in launcher_text and "qt6/plugins" in launcher_text:
        raise SystemExit(
            "Linux launcher still contains the old system Qt plugin search path."
        )

    qt_version = qt_version_marker.read_text(encoding="utf-8", errors="replace").strip()
    if not qt_version:
        raise SystemExit("Linux Qt runtime version marker is empty.")

    expected_qt_version = os.environ.get("TONATIUH_QT_VERSION", "").strip()
    if expected_qt_version and qt_version != expected_qt_version:
        raise SystemExit(
            f"Linux staged Qt version marker ({qt_version}) does not match "
            f"the build Qt version ({expected_qt_version})."
        )

    core_files = sorted(path.name for path in lib_dir.glob("libQt6Core.so.6*"))
    if not any(qt_version in name for name in core_files):
        shown_core_files = "\n".join(f"  - {name}" for name in core_files) or "  <none>"
        raise SystemExit(
            f"Bundled libQt6Core files do not match Qt version marker {qt_version}:\n"
            f"{shown_core_files}"
        )

    plugin_files = find_linux_qt_plugin_files(bin_dir)
    if not plugin_files:
        raise SystemExit("Linux staging did not include any bundled Qt plugin .so files.")
    verify_linux_qt_plugin_versions(plugin_files, qt_version)

    qt_plugin_paths = set(plugin_files)
    qt_linked_files = [real_binary]
    qt_linked_files.extend(find_linux_bundled_so_files(bin_dir))
    for binary in dict.fromkeys(qt_linked_files):
        verify_linux_qt_links(
            binary,
            lib_dir,
            require_qt_dependency=binary in qt_plugin_paths,
        )

    if verbose:
        print(
            f"Linux staging bundles Qt {qt_version} runtime "
            f"and {len(plugin_files)} Qt plugin(s)."
        )


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
                f" Current staging did not produce {MACOS_APP_BUNDLE_NAME}."
            )
        if args.verbose:
            print(f"Deploying Qt runtime on macOS for {staged_target}")
        deploy_macos(staged_target, verbose=args.verbose)
        validate_macos_app_bundle(staged_target)
    else:
        if args.verbose:
            print("Verifying Linux Qt runtime bundling in staged output")
        verify_linux_bundling(staging_dir, verbose=args.verbose)

    copy_tree(staging_dir, package_data_dir, verbose=args.verbose)

    if sys.platform == "darwin":
        packaged_bundle = find_macos_app_bundle(package_data_dir)
        validate_macos_app_bundle(packaged_bundle)

    print("Qt IFW package data staged successfully.")
    print(f"Staging directory: {staging_dir}")
    print(f"Package data directory: {package_data_dir}")


if __name__ == "__main__":
    main()
