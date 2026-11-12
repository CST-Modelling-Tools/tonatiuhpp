#!/usr/bin/env python3
"""Phase E: relocated CMake-install scientific acceptance (test-enabled runtime).

Runs actual installed CLI and GUI binaries from an external, renamed prefix.
It does NOT install a production IFW payload or validate an updater/release.
All fixtures and Python comparators come from the source checkout; *executables,
plugins, runtime DLLs/frameworks and installed examples* must come from the
relocated prefix. Never runs the 500-million-ray heliostat benchmark.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import traceback

MODES = ("interactive-file", "seeded-interception", "interactive-flux")


def check(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def inside(path: Path, parent: Path) -> bool:
    return path == parent or parent in path.parents


def stage_relocated_install(origin: Path, scratch: Path, source: Path) -> Path:
    """Move only the explicitly named CI-created install root; never remove it."""
    check(scratch.is_dir() and not scratch.is_symlink(),
          "Scratch root must be an existing real directory")
    check(not inside(scratch, source) and not inside(source, scratch),
          "Install scratch root must be separate from the source checkout")
    check(origin.is_dir() and not origin.is_symlink(),
          "Install origin must exist and must not be a symlink")
    check(origin.parent == scratch,
          "Only a direct child of the explicit scratch root may be relocated")
    check(origin.name == "cmake-install",
          "Refusing to move an install root without the expected disposable name")
    dest_parent = Path(tempfile.mkdtemp(prefix="relocated_", dir=scratch))
    dest = dest_parent / "tonatiuhpp"
    shutil.move(str(origin), str(dest))
    check(dest.is_dir() and not origin.exists(),
          "Install relocation did not move the source tree to its new prefix")
    return dest


def locate_executables(prefix: Path) -> tuple[Path, Path]:
    if sys.platform == "darwin":
        bundle = prefix / "tonatiuhpp-gui.app"
        gui = bundle / "Contents" / "MacOS" / "tonatiuhpp-gui"
        cli = prefix / "bin" / "tonatiuhpp-cli"
        # Support an already-consolidated IFW layout without guessing its
        # runtime semantics: the observed path is recorded in evidence.
        if not cli.is_file():
            cli = bundle / "Contents" / "MacOS" / "tonatiuhpp-cli"
    else:
        ext = ".exe" if os.name == "nt" else ""
        gui = prefix / "bin" / ("tonatiuhpp-gui" + ext)
        cli = prefix / "bin" / ("tonatiuhpp-cli" + ext)
    for label, executable in (("GUI", gui), ("CLI", cli)):
        check(executable.is_file(), f"{label} executable missing from installed prefix: {executable}")
        resolved = executable.resolve(strict=True)
        check(inside(resolved, prefix.resolve(strict=True)),
              f"{label} executable resolves outside the relocated install: {resolved}")
        if os.name != "nt":
            check(os.access(executable, os.X_OK), f"{label} installed file is not executable")
    check(gui.resolve() != cli.resolve(), "GUI and CLI must be distinct installed executables")
    return gui, cli


def run_checked(name: str, command: list[str], cwd: Path,
                output: Path, timeout: int) -> None:
    out_path = output / (name + ".stdout.txt")
    err_path = output / (name + ".stderr.txt")
    print("INSTALLED_ACCEPTANCE_START " + name, flush=True)
    options = ({"creationflags": subprocess.CREATE_NEW_PROCESS_GROUP}
               if os.name == "nt" else {"start_new_session": True})
    with out_path.open("w", encoding="utf-8") as out, err_path.open("w", encoding="utf-8") as err:
        try:
            finished = subprocess.run(command, cwd=cwd, stdin=subprocess.DEVNULL,
                                      stdout=out, stderr=err, timeout=timeout,
                                      check=False, **options)
        except subprocess.TimeoutExpired as exc:
            raise RuntimeError(f"{name} timed out after {timeout}s; see {out_path} and {err_path}") from exc
    if finished.returncode:
        tail = err_path.read_text(encoding="utf-8", errors="replace")[-2500:]
        raise RuntimeError(f"{name} returned {finished.returncode}; stderr: {tail}; see {out_path}")
    print("INSTALLED_ACCEPTANCE_PASS " + name, flush=True)


def check_windows_installed_gui_dlls(executable: Path, *, is_windows: bool) -> None:
    """Validate the exact Release GUI-only Qt runtime dependency set."""
    if not is_windows:
        return
    for dll in ("Qt6OpenGL.dll", "Qt6PrintSupport.dll", "Qt6Widgets.dll"):
        check((executable.parent / dll).is_file(),
              f"Relocated native GUI is missing required DLL: {dll}")


def smoke_native_production_gui(gui: Path, scene_root: Path,
                                working: Path, output: Path) -> None:
    """Verify actual production GUI stays alive on a real native desktop.

    This is deliberately NOT a substitute for Phase D's widget/numeric
    acceptance, which requires test-only GUI hooks. No offscreen platform.
    """
    platform = {"win32": "windows", "darwin": "cocoa"}.get(sys.platform, "xcb")
    env = os.environ.copy()
    check(env.get("QT_QPA_PLATFORM") == platform,
          f"Production GUI needs real native Qt platform {platform!r}")
    for key in ("LD_LIBRARY_PATH", "LD_PRELOAD", "DYLD_LIBRARY_PATH",
                "DYLD_FALLBACK_LIBRARY_PATH", "DYLD_FRAMEWORK_PATH",
                "DYLD_FALLBACK_FRAMEWORK_PATH", "QT_PLUGIN_PATH",
                "QT_QPA_PLATFORM_PLUGIN_PATH", "QML2_IMPORT_PATH"):
        env.pop(key, None)
    if os.name == "nt":
        windows = Path(os.environ.get("SystemRoot", "C:/Windows"))
        env["PATH"] = os.pathsep.join(map(str, (
            gui.parent, windows / "System32", windows,
            windows / "System32" / "Wbem")))
    scene = scene_root / "examples" / "benchmarks" / "cylinder.tnhpp"
    check(scene.is_file(), "Production GUI must use an installed cylinder scene")
    stdout = output / "production-gui.stdout.txt"
    stderr = output / "production-gui.stderr.txt"
    print("INSTALLED_ACCEPTANCE_START production-native-gui-startup", flush=True)
    flags = ({"creationflags": subprocess.CREATE_NEW_PROCESS_GROUP}
             if os.name == "nt" else {"start_new_session": True})
    with stdout.open("w", encoding="utf-8") as out, stderr.open("w", encoding="utf-8") as err:
        p = subprocess.Popen([str(gui), str(scene)], cwd=working, env=env,
                             stdin=subprocess.DEVNULL, stdout=out, stderr=err,
                             **flags)
        try:
            deadline = time.monotonic() + 7.0
            while time.monotonic() < deadline:
                code = p.poll()
                if code is not None:
                    raise RuntimeError(f"Production GUI exited prematurely: {code}; "
                                       f"stderr={stderr.read_text(errors='replace')[-2000:]}")
                time.sleep(0.2)
        finally:
            if p.poll() is None:
                p.terminate()
                try:
                    p.wait(timeout=12)
                except subprocess.TimeoutExpired:
                    p.kill()
                    p.wait(timeout=10)
    print("INSTALLED_ACCEPTANCE_PASS production-native-gui-startup "
          "(native window-process liveness only, not GUI dialog/numeric acceptance)",
          flush=True)


def run_suite(args: argparse.Namespace) -> None:
    source = args.source_root.resolve(strict=True)
    # Preserve the original path spelling so the relocation guard can reject
    # symlinked install roots instead of silently resolving them.
    origin = args.install_origin.absolute()
    scratch = args.scratch_root.resolve(strict=True)
    output = args.output_root.resolve()
    output.mkdir(parents=True, exist_ok=True)
    check(not inside(output, source / "examples") and not inside(output, scratch),
          "Evidence output must not be inside installed payload or tracked examples")
    report_path = output / "installed_acceptance.json"
    report: dict = {
        "schema": "tonatiuhpp.phase-e.relocated-install.v1",
        "status": "RUNNING",
        "platform": sys.platform,
        "source_root": str(source),
        "install_origin": str(origin),
        "started_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "checks": [],
        "scope": "Relocated BUILD_TESTING=ON CMake install, not production IFW",
    }

    def save():
        report["finished_utc"] = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
        report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")

    save()
    try:
        prefix = stage_relocated_install(origin, scratch, source)
        report["relocated_prefix"] = str(prefix)
        gui, cli = locate_executables(prefix)
        report["gui"] = str(gui)
        report["cli"] = str(cli)
        if args.production_smoke:
            report["scope"] = ("Relocated BUILD_TESTING=OFF CMake production runtime: "
                               "real CLI scientific tests plus bounded native GUI startup; "
                               "not IFW installer execution")
        scene_root = prefix
        if sys.platform == "darwin" and not (prefix / "examples").is_dir():
            scene_root = prefix / "tonatiuhpp-gui.app" / "Contents" / "Resources"
        for scene in ("cylinder.tnhpp", "sphere.tnhpp"):
            example = scene_root / "examples" / "benchmarks" / scene
            check(example.is_file(), f"Installed scientific example missing: {example}")
        # Preflight the GUI-only dependency set; CLI loading by itself
        # cannot establish that the installed GUI can start.
        check_windows_installed_gui_dlls(gui, is_windows=(os.name == "nt"))
        save()
        mesa_staged = False
        if args.windows_ci_mesa:
            check(os.name == "nt" and os.environ.get("GITHUB_ACTIONS") == "true",
                  "Windows Mesa deployment is restricted to Windows GitHub Actions")
            mesa_helper = source / "scripts" / "install_ci_windows_mesa.ps1"
            check(mesa_helper.is_file(), "Missing SHA256-pinned Windows Mesa helper")
            run_checked("installed-mesa-stage",
                        ["pwsh", "-NoProfile", "-File", str(mesa_helper),
                         "-Action", "install", "-TargetBin", str(gui.parent)],
                        output, output, 75)
            mesa_staged = True
        scripts = source / "tests"
        check((scripts / "integration" / "cli" / "cli_process_repeatability.py").is_file(),
              "Missing accepted CLI process comparator")
        check((scripts / "integration" / "gui" / "run_native_gui_acceptance.py").is_file(),
              "Missing accepted genuine GUI comparator")
        working = output / "foreign-working-directory"
        working.mkdir(exist_ok=True)
        cli_cmd = [
            sys.executable, str(scripts / "integration/cli/cli_process_repeatability.py"),
            "--gui", str(gui), "--cli", str(cli), "--source-root", str(source),
            "--scene-root", str(scene_root),
            "--output-root", str(output / "cli"), "--mode", "installed",
        ]
        run_checked("installed-cli", cli_cmd, working, output, 780)
        report["checks"].append("installed-cli")
        save()
        if args.production_smoke:
            # Production BUILD_TESTING=OFF does not contain Phase D test-only
            # Qt automation hooks. A genuine native GUI startup/liveness smoke
            # is possible; claiming dialog-level scientific acceptance here
            # would be false. Scientific numerics come from the actual
            # production CLI above, not a test-enabled binary.
            smoke_native_production_gui(gui, scene_root, working, output)
            report["checks"].append("production-native-gui-startup")
            save()
        else:
            for mode in MODES:
                cmd = [
                    sys.executable, str(scripts / "integration/gui/run_native_gui_acceptance.py"),
                    "--mode", mode, "--gui", str(gui), "--cli", str(cli),
                    "--source-root", str(source), "--scene-root", str(scene_root),
                    "--installed-prefix", str(prefix),
                    "--output-root", str(output / "gui"),
                ]
                run_checked("installed-gui-" + mode, cmd, working, output, 195)
                report["checks"].append("installed-gui-" + mode)
                save()
        report["status"] = "PASS"
        print("INSTALLED_ACCEPTANCE_RESULT=PASS " + str(report_path), flush=True)
    except BaseException as exc:
        report["status"] = "FAIL"
        report["reason"] = str(exc)
        print("INSTALLED_ACCEPTANCE_RESULT=FAIL " + str(exc), file=sys.stderr, flush=True)
        raise
    finally:
        if locals().get("mesa_staged", False):
            try:
                run_checked("installed-mesa-remove",
                            ["pwsh", "-NoProfile", "-File", str(mesa_helper),
                             "-Action", "remove", "-TargetBin", str(gui.parent)],
                            output, output, 75)
            except BaseException as cleanup_error:
                report["status"] = "FAIL"
                report["cleanup_error"] = str(cleanup_error)
                print("INSTALLED_ACCEPTANCE_CLEANUP_FAIL " + str(cleanup_error),
                      file=sys.stderr, flush=True)
                raise
        save()
        print("INSTALLED_ACCEPTANCE_EVIDENCE=" + str(report_path), flush=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--install-origin", type=Path, required=True)
    parser.add_argument("--scratch-root", type=Path, required=True)
    parser.add_argument("--output-root", type=Path, required=True)
    parser.add_argument("--production-smoke", action="store_true",
                        help="BUILD_TESTING=OFF: production CLI scientific checks "
                             "and bounded real-GUI startup only (no test-only hooks)")
    parser.add_argument("--windows-ci-mesa", action="store_true",
                        help="Stage SHA256-pinned software WGL only for Windows CI GUI tests")
    args = parser.parse_args()
    try:
        run_suite(args)
    except (OSError, ValueError, RuntimeError) as exc:
        print("FAIL: installed-runtime acceptance: " + str(exc), file=sys.stderr, flush=True)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
