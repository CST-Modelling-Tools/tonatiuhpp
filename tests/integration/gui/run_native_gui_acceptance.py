#!/usr/bin/env python3
"""Bounded display-backed acceptance of the real Tonatiuh++ GUI.

Only tests disposable build-tree outputs. It NEVER invokes the 500M benchmark.
No GUI success is accepted without the native Qt report and numerical evidence.
"""
from __future__ import annotations

import argparse
import importlib.util
import json
import math
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
import time

MODES = ("interactive-file", "seeded-interception", "interactive-flux")
GUI_TIMEOUT_SECONDS = 145  # Above the native helper's 120-second watchdog.
CLI_TIMEOUT_SECONDS = 90
IDEAL_PLANAR_POWER_W = 8.0 * 8.0 * 1000.0
# The 2048/4096-ray fixtures sample a finite sun aperture and sunshape.
# This conservative bound tests the independently known 8x8 m^2 absorber
# illuminated at 1000 W/m^2 at zenith; it is NOT a fitted golden output.
PLANAR_RELATIVE_TOLERANCE = 0.20


class AcceptanceError(RuntimeError):
    """The GUI did not demonstrate its scientific acceptance contract."""


def require(condition: bool, detail: str) -> None:
    if not condition:
        raise AcceptanceError(detail)


def numeric(value: object, name: str, *, allow_zero: bool = False) -> float:
    require(type(value) in (int, float), f"{name} is not numeric")
    number = float(value)
    require(math.isfinite(number) and (number >= 0 if allow_zero else number > 0),
            f"{name} must be finite and {'nonnegative' if allow_zero else 'positive'}")
    return number


def close_to_analytic_power(value: float, name: str) -> None:
    require(abs(value - IDEAL_PLANAR_POWER_W) <=
            PLANAR_RELATIVE_TOLERANCE * IDEAL_PLANAR_POWER_W,
            f"{name}: {value:.8g} W outside the independent 64,000 W "
            f"8 m x 8 m x 1000 W/m^2 reference (+/-20%)")


def validate_native_report(report: dict, mode: str) -> None:
    require(isinstance(report, dict), "Native GUI report must be a JSON object")
    require(report.get("schema") == "tonatiuhpp.m5.native-gui-presenter.v1",
            "Unexpected native Qt report schema")
    require(report.get("mode") == mode, "Native GUI report mode mismatch")
    require(report.get("status") == "PASS", "Native GUI helper did not report PASS")
    require(report.get("gui_backend") == "QApplication/MainWindow/actual Qt dialogs",
            "Report does not identify the real Qt-widget backend")
    require(report.get("ray_dialog_accepted") is True,
            "Actual GUI Ray Tracing dialog was not accepted")
    expected_seed = 123456789 if mode == "interactive-flux" else 0
    require(type(report.get("fixed_seed")) is int and
            report["fixed_seed"] == expected_seed,
            "Actual GUI Ray Tracing dialog used an unexpected seed")


def validate_interception(artifact: Path) -> dict:
    rows = artifact.read_text(encoding="utf-8-sig").splitlines()
    expected = [
        "schema,m3.3-script-parity-v1",
        "surface,//Node/M3_3_Parity/Receiver/Shape",
        "rays,2048",
        "sun_azimuth_deg,180",
        "sun_elevation_deg,90",
    ]
    require(len(rows) == 6 and rows[:5] == expected and
            rows[5].startswith("power_w,"),
            "GUI Script Editor interception artifact has wrong scene, seed inputs, or schema")
    try:
        power = float(rows[5].split(",", 1)[1])
    except ValueError as exc:
        raise AcceptanceError("GUI interception power_w is not numeric") from exc
    numeric(power, "GUI interception power")
    close_to_analytic_power(power, "GUI Script Editor interception power")
    return {"intercepted_power_w": power, "analytic_reference_w": IDEAL_PLANAR_POWER_W}


def read_flux_grid(path: Path, *, require_signal: bool) -> list[list[float]]:
    rows = path.read_text(encoding="utf-8-sig").splitlines()
    require(len(rows) == 8, f"{path.name}: expected eight flux rows")
    grid = []
    for y, line in enumerate(rows):
        cells = line.split("\t")
        if cells and cells[-1] == "":
            cells.pop()  # Native TSV exporter emits a trailing tab.
        require(len(cells) == 8, f"{path.name}: expected eight flux columns in row {y}")
        row = []
        for x, cell in enumerate(cells):
            require(cell.strip() != "", f"{path.name}: blank flux cell ({x}, {y})")
            try:
                value = float(cell)
            except ValueError as exc:
                raise AcceptanceError(f"{path.name}: nonnumeric flux cell ({x}, {y})") from exc
            row.append(numeric(value, f"{path.name} flux cell ({x}, {y})", allow_zero=True))
        grid.append(row)
    if require_signal:
        require(any(v > 0 for row in grid for v in row), f"{path.name}: no receiver signal")
    return grid


def validate_interactive_flux(report: dict, script_dir: Path, report_dir: Path) -> dict:
    rows = report.get("plot_cells_yx")
    require(isinstance(rows, list) and len(rows) == 8,
            "Native GUI Qt color map does not have eight rows")
    cells = []
    for y, row in enumerate(rows):
        require(isinstance(row, list) and len(row) == 8,
                f"Native GUI Qt color map row {y} does not contain eight columns")
        cells.extend(numeric(cell, f"Qt color map ({x}, {y})", allow_zero=True)
                     for x, cell in enumerate(row))
    require(any(cell > 0 for cell in cells), "Qt color map contains no receiver signal")
    total_flux = sum(cells)
    peak_flux = max(cells)
    # The receiver is 8x8 m^2 with an 8x8 bin grid: each planar cell is 1 m^2.
    # Integrate the actual widget color map independently of displayed labels.
    close_to_analytic_power(total_flux, "Integrated real Qt flux-map power")
    require(peak_flux <= 4.0 * 1000.0, "Qt peak flux is implausibly high for the test absorber")
    power = numeric(report.get("displayed_power_w"), "Displayed Qt total power")
    peak = numeric(report.get("displayed_peak_flux"), "Displayed Qt peak flux")
    require(math.isclose(total_flux, power, rel_tol=0.0001, abs_tol=0.1),
            f"Qt displayed power {power} W disagrees with 8x8 plot integral {total_flux} W")
    require(math.isclose(peak_flux, peak, rel_tol=0.0001, abs_tol=0.1),
            f"Qt displayed peak {peak} W/m^2 disagrees with plot maximum {peak_flux} W/m^2")
    require(math.isclose(numeric(report.get("plot_sum"), "Qt reported plot sum"),
                         total_flux, rel_tol=1e-12, abs_tol=1e-8),
            "Qt report plot sum differs from its grid")
    require(math.isclose(numeric(report.get("plot_max"), "Qt reported plot maximum"),
                         peak_flux, rel_tol=1e-12, abs_tol=1e-8),
            "Qt report plot maximum differs from its grid")
    png = report_dir / "interactive_flux_plot.png"
    require(isinstance(report.get("plot_png"), str) and
            Path(report["plot_png"]).resolve() == png.resolve(),
            "Qt plot PNG path escaped the report directory")
    require(png.read_bytes().startswith(b"\x89PNG\r\n\x1a\n"),
            "Real GUI plot PNG is absent or has an invalid signature")
    front = read_flux_grid(script_dir / "m3_flux_front.txt", require_signal=True)
    read_flux_grid(script_dir / "m3_flux_back.txt", require_signal=False)
    script_power = sum(sum(row) for row in front)  # Same 1 m^2 cell areas.
    close_to_analytic_power(script_power, "GUI Script Editor front-flux integral")
    require(abs(script_power - total_flux) <= 0.20 * IDEAL_PLANAR_POWER_W,
            "Independent native GUI Flux Dialog and GUI Script Editor total powers disagree")
    return {"qt_plot_integral_w": total_flux, "qt_display_power_w": power,
            "qt_peak_flux_w_m2": peak_flux, "gui_script_front_integral_w": script_power,
            "analytic_reference_w": IDEAL_PLANAR_POWER_W}


def load_canonical_photon_comparator(source_root: Path):
    path = source_root / "tests/scientific/compare_photon_exports.py"
    require(path.is_file(), "Canonical binary-photon comparator is missing")
    spec = importlib.util.spec_from_file_location("phase_d_photons", path)
    require(spec is not None and spec.loader is not None, "Cannot load canonical photon comparator")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.read_export


def validate_gui_cli_photons(gui_dir: Path, cli_dir: Path, source_root: Path) -> dict:
    read_export = load_canonical_photon_comparator(source_root)
    gui_records, gui_power, gui_count, gui_surfaces = read_export(gui_dir)
    cli_records, cli_power, cli_count, cli_surfaces = read_export(cli_dir)
    require(gui_count > 0 and cli_count > 0 and gui_surfaces and cli_surfaces,
            "Native GUI/CLI scientific photon data are empty")
    require(gui_records == cli_records,
            f"Real GUI vs CLI photon records differ: {gui_count} vs {cli_count}")
    require(math.isclose(gui_power, cli_power, rel_tol=1e-12, abs_tol=0.0),
            "Real GUI vs CLI per-photon power differs")
    return {"gui_photon_records": gui_count, "cli_photon_records": cli_count,
            "photon_power_w": gui_power, "canonical_gui_cli_equivalent": True}


def run_process(command: list[str], cwd: Path, env: dict[str, str],
                stdout: Path, stderr: Path, timeout: int) -> None:
    options = {"start_new_session": True} if os.name != "nt" else {
        "creationflags": subprocess.CREATE_NEW_PROCESS_GROUP}
    with stdout.open("w", encoding="utf-8") as out, stderr.open("w", encoding="utf-8") as err:
        process = subprocess.Popen(command, cwd=cwd, env=env, stdin=subprocess.DEVNULL,
                                   stdout=out, stderr=err, **options)
        try:
            result = process.wait(timeout=timeout)
        except subprocess.TimeoutExpired as exc:
            if os.name != "nt":
                os.killpg(process.pid, signal.SIGKILL)
            else:
                process.kill()
            process.wait(timeout=10)
            raise AcceptanceError(f"Process exceeded {timeout}s timeout: {command[0]}") from exc
    require(result == 0,
            f"Process returned {result}: {command[0]} (see {stdout} and {stderr})")


def run_acceptance(args: argparse.Namespace) -> Path:
    output_parent = args.output_root.resolve()
    output_parent.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix=f"native_{args.mode.replace('-', '_')}_",
                                 dir=output_parent))
    summary = {"schema": "tonatiuhpp.phase-d.native-gui.v1", "mode": args.mode,
               "status": "RUNNING", "started_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
               "evidence_dir": str(root)}
    summary_path = root / "acceptance.json"

    def save():
        summary_path.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")

    save()
    try:
        gui = args.gui.resolve(strict=True)
        cli = args.cli.resolve(strict=True)
        source = args.source_root.resolve(strict=True)
        require(gui != cli and gui.is_file() and cli.is_file(),
                "Native GUI and CLI must be two distinct real executables")
        require(os.environ.get("QT_QPA_PLATFORM", "").lower() not in ("offscreen", "minimal"),
                "GUI acceptance requires a real display; Qt offscreen/minimal is not valid")
        if sys.platform.startswith("linux"):
            require(bool(os.environ.get("DISPLAY")), "Linux GUI acceptance needs X11/GLX (DISPLAY)")
        scene_root = args.scene_root.resolve(strict=True) if args.scene_root else source
        scene = scene_root / "examples/benchmarks/cylinder.tnhpp"
        require(scene.is_file(), "Controlled installed/cylinder startup scene is missing")
        if args.installed_prefix:
            prefix = args.installed_prefix.resolve(strict=True)
            require(prefix != source and source not in prefix.parents,
                    "Installed prefix must be outside source tree")
            require(prefix == scene_root, "Installed GUI acceptance must use installed scenes")
            for executable in (gui, cli):
                require(prefix in executable.parents,
                        "GUI and CLI must be physically located in the installed prefix")
        export_dir = root / "gui_export"
        export_dir.mkdir()
        script_dir = root / "script"
        script_dir.mkdir()
        script_path = script_dir / ("m3_3_gui_headless_parity.tnhpps" if
                                    args.mode == "seeded-interception" else
                                    "m3_flux_shared_parity.tnhpps")
        if args.mode != "interactive-file":
            fixture = source / "tests/scripts" / script_path.name
            require(fixture.is_file(), "Controlled GUI Script Editor fixture is missing")
            shutil.copyfile(fixture, script_path)
        settings_dir = root / "settings"
        settings_dir.mkdir()
        env = os.environ.copy()
        if args.installed_prefix:
            # The test harness lives in the source tree, but subprocesses must
            # not borrow uninstalled Qt/Coin/SoQt libraries or Qt plugins.
            for key in ("LD_LIBRARY_PATH", "LD_PRELOAD", "DYLD_LIBRARY_PATH",
                        "DYLD_FALLBACK_LIBRARY_PATH", "DYLD_FRAMEWORK_PATH",
                        "DYLD_FALLBACK_FRAMEWORK_PATH", "QT_PLUGIN_PATH",
                        "QT_QPA_PLATFORM_PLUGIN_PATH", "QML2_IMPORT_PATH",
                        "QT_QPA_FONTDIR"):
                env.pop(key, None)
            if os.name == "nt":
                system_root = Path(os.environ.get("SystemRoot", "C:/Windows"))
                env["PATH"] = os.pathsep.join((
                    str(cli.parent), str(gui.parent),
                    str(system_root / "System32"), str(system_root),
                    str(system_root / "System32" / "Wbem")))
        for name in ("TONATIUHPP_A0_TRACE_SIGNATURE_FILE", "TONATIUHPP_A0_RAY_HISTORY_FILE",
                     "TONATIUHPP_M3_TEST_FILE_DATA_FAULT"):
            env.pop(name, None)
        env.update({
            "TONATIUHPP_M5_GUI_NATIVE_REPORT": str(root / "native_report.json"),
            "TONATIUHPP_M5_GUI_NATIVE_MODE": args.mode,
            "TONATIUHPP_M5_GUI_NATIVE_EXPORT_DIR": str(export_dir),
            "TONATIUHPP_M5_GUI_NATIVE_SCRIPT": str(script_path) if args.mode != "interactive-file" else "",
            "TONATIUHPP_M5_GUI_NATIVE_SETTINGS_DIR": str(settings_dir),
            "XDG_CONFIG_HOME": str(settings_dir),
        })
        if os.name == "nt":
            env["APPDATA"] = str(settings_dir)
            env["LOCALAPPDATA"] = str(settings_dir)
        summary["gui_executable"] = str(gui)
        summary["startup_scene"] = str(scene)
        save()
        run_process([str(gui), str(scene)], root, env, root / "gui_stdout.txt",
                    root / "gui_stderr.txt", GUI_TIMEOUT_SECONDS)
        report = json.loads((root / "native_report.json").read_text(encoding="utf-8"))
        validate_native_report(report, args.mode)
        if args.mode == "interactive-file":
            require((export_dir / "PhotonMap.dat").is_file(), "Real GUI photon binary is missing")
            cli_dir = root / "cli_export"
            cli_dir.mkdir()
            cli_env = dict(env)
            for name in tuple(cli_env):
                if name.startswith("TONATIUHPP_M5_GUI_NATIVE_"):
                    cli_env.pop(name, None)
            run_process([str(cli), "trace-scene", str(scene), "--rays", "2048",
                         "--seed", "0", "--photon-export-dir", str(cli_dir)],
                        root, cli_env, root / "cli_stdout.txt", root / "cli_stderr.txt",
                        CLI_TIMEOUT_SECONDS)
            summary["numerical"] = validate_gui_cli_photons(export_dir, cli_dir, source)
            require(report.get("photon_records") == summary["numerical"]["gui_photon_records"],
                    "Real GUI helper photon count disagrees with canonical 48-byte record decoder")
        elif args.mode == "seeded-interception":
            summary["numerical"] = validate_interception(script_dir / "m3_3_parity_result.csv")
        else:
            summary["numerical"] = validate_interactive_flux(report, script_dir, root)
        # Make scientific numbers visible in CTest -V and hosted CI logs, not
        # only in the retained build-tree acceptance.json evidence.
        print("GUI_ACCEPTANCE_NUMERICAL=" +
              json.dumps(summary["numerical"], sort_keys=True), flush=True)
        summary["status"] = "PASS"
        return root
    except (OSError, ValueError, KeyError, ImportError, AcceptanceError) as exc:
        summary["status"] = "FAIL"
        summary["reason"] = str(exc)
        raise
    finally:
        summary["finished_utc"] = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
        save()
        print(f"GUI_ACCEPTANCE_EVIDENCE={root}", flush=True)


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=MODES, required=True)
    parser.add_argument("--gui", type=Path, required=True)
    parser.add_argument("--cli", type=Path, required=True)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--scene-root", type=Path,
                        help="Installed example root; defaults to source tree for build acceptance")
    parser.add_argument("--installed-prefix", type=Path,
                        help="Sanitize child library/plugin lookup and require relocated installed executables")
    parser.add_argument("--output-root", type=Path, required=True)
    args = parser.parse_args(argv)
    try:
        evidence = run_acceptance(args)
    except (OSError, ValueError, KeyError, ImportError, AcceptanceError) as exc:
        print(f"FAIL: native GUI {args.mode}: {exc}", file=sys.stderr)
        return 1
    print(f"PASS: native GUI {args.mode}: actual Qt widgets and numerical acceptance; {evidence}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
