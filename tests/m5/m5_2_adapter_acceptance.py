#!/usr/bin/env python3
"""M5.2 bounded, cross-platform, two-entrypoint scientific acceptance.

Never runs a benchmark, alters frozen references, edits source-tree fixtures,
commits, pushes or contacts GitHub. All output is in a fresh unique test root.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import traceback

TIMEOUT = 90
EXPECTED_M3 = {
    "m3_3_gui_headless_parity": {
        "marker": "M3.3 parity fixture passed:",
        "files": ("m3_3_parity_result.csv",),
    },
    "m3_3_tracker_parity": {
        "marker": "M3.3 tracker parity fixture passed:",
        "files": ("m3_3_tracker_result.csv",),
    },
    "m3_flux_shared_parity": {
        "marker": "M3 integrated fixed-seed flux parity script passed",
        "files": ("m3_flux_front.txt", "m3_flux_back.txt"),
    },
}


def require(ok: bool, why: str) -> None:
    if not ok:
        raise AssertionError(why)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def nonempty_file(path: Path, why: str = "missing or empty output") -> Path:
    require(path.is_file() and path.stat().st_size > 0, f"{why}: {path}")
    return path


def normalize_bytes(path: Path) -> bytes:
    return nonempty_file(path).read_bytes().replace(b"\r\n", b"\n")


class Suite:
    def __init__(self, args: argparse.Namespace):
        self.args = args
        self.root = Path(args.source_root).resolve()
        self.gui = Path(args.gui).resolve()
        self.cli = Path(args.cli).resolve()
        self.output_parent = Path(args.output_root).resolve()
        self.output_parent.mkdir(parents=True, exist_ok=True)
        self.runroot = Path(tempfile.mkdtemp(prefix=f"m5_2_{args.mode}_", dir=self.output_parent))
        self.journal: dict = {"schema": "tonatiuhpp.m5.2.two-entrypoint.v1",
                              "mode": args.mode, "source_root": str(self.root),
                              "gui": str(self.gui), "cli": str(self.cli),
                              "started_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
                              "checks": [], "result": "IN_PROGRESS"}
        self.comparator = self.load_comparator()
        self.scenes = {
            "cylinder": self.root / "examples/benchmarks/cylinder.tnhpp",
            "sphere_material_plugin": self.root / "examples/benchmarks/sphere.tnhpp",
            "specular_two_reflections": self.root / "tests/scientific/fixtures/specular-two-reflections.tnhpp",
            "one_axis_tracker": self.root / "tests/scientific/fixtures/one-axis-tracker.tnhpp",
        }
        self.env = os.environ.copy()
        # Diagnostics must be opt-in for each one short explicit trace only.
        self.env.pop("TONATIUHPP_A0_TRACE_SIGNATURE_FILE", None)
        self.env.pop("TONATIUHPP_A0_RAY_HISTORY_FILE", None)
        self.env.pop("TONATIUHPP_M3_TEST_FILE_DATA_FAULT", None)
        self.prepare_installed_env()

    def prepare_installed_env(self) -> None:
        if self.args.mode != "installed":
            return
        # Installed-runtime acceptance must not pass by borrowing uninstalled
        # Tonatiuh++ / third_party / Qt SDK DLLs from the developer's PATH.
        for var in ("LD_LIBRARY_PATH", "DYLD_LIBRARY_PATH", "DYLD_FALLBACK_LIBRARY_PATH", "QT_PLUGIN_PATH"):
            self.env.pop(var, None)
        if os.name == "nt":
            windir = Path(os.environ.get("SystemRoot", "C:/Windows"))
            exe_dir = self.cli.parent
            self.env["PATH"] = os.pathsep.join(map(str, (
                exe_dir, self.gui.parent, windir / "System32", windir,
                windir / "System32" / "Wbem",
            )))
            self.env.pop("QT_QPA_PLATFORM_PLUGIN_PATH", None)

    def load_comparator(self):
        path = nonempty_file(self.root / "tests/scientific/compare_photon_exports.py", "M4 canonical photon comparator missing")
        spec = importlib.util.spec_from_file_location("m5_2_canonical_photons", path)
        require(spec is not None and spec.loader is not None, "Cannot import canonical photon comparator")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        require(callable(getattr(module, "read_export", None)), "Canonical comparator has no read_export")
        return module

    def log(self, name: str, detail: str = "") -> None:
        print(f"M5.2 PASS {name}" + (f": {detail}" if detail else ""), flush=True)
        self.journal["checks"].append({"name": name, "result": "PASS", "detail": detail})
        self.save()

    def save(self) -> None:
        self.journal["finished_utc"] = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())
        (self.runroot / "evidence.json").write_text(
            json.dumps(self.journal, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    def run(self, adapter: str, args: list[str], *, cwd: Path | None = None,
            env_extra: dict[str, str] | None = None, expected: int = 0,
            contains: str | None = None, timeout: int = TIMEOUT) -> subprocess.CompletedProcess:
        require(adapter in {"gui", "cli"}, f"Invalid adapter: {adapter}")
        binary = self.gui if adapter == "gui" else self.cli
        command = [str(binary)] + (["--headless"] if adapter == "gui" else []) + list(args)
        env = dict(self.env)
        if env_extra:
            env.update(env_extra)
        place = cwd if cwd is not None else self.runroot
        place.mkdir(parents=True, exist_ok=True)
        try:
            done = subprocess.run(command, cwd=place, env=env, text=True,
                                  encoding="utf-8", errors="replace", capture_output=True,
                                  timeout=timeout, check=False)
        except subprocess.TimeoutExpired as exc:
            raise AssertionError(f"{adapter} timed out in {timeout}s: {args[:3]}") from exc
        require(done.returncode == expected,
                f"{adapter} {args[:3]} returned {done.returncode}, expected {expected}\n"
                f"stdout:\n{done.stdout[-3000:]}\nstderr:\n{done.stderr[-3000:]}")
        if contains:
            require(contains in done.stdout + done.stderr,
                    f"{adapter} {args[:3]} did not emit required marker {contains!r}\n"
                    f"stdout:\n{done.stdout[-2500:]}\nstderr:\n{done.stderr[-2500:]}")
        return done

    def trace(self, adapter: str, case: str, rays: int, *, trackers: bool = False):
        out = self.runroot / "trace_signatures" / case / adapter
        out.mkdir(parents=True, exist_ok=False)
        sig = out / "hits.json"
        hist = out / "history.json"
        args = ["trace-scene", str(self.scenes[case]), "--rays", str(rays), "--seed", "123456789", "--no-export"]
        if trackers:
            args.append("--update-trackers")
        done = self.run(adapter, args,
                        env_extra={"TONATIUHPP_A0_TRACE_SIGNATURE_FILE": str(sig),
                                   "TONATIUHPP_A0_RAY_HISTORY_FILE": str(hist)},
                        contains="Trace completed.")
        values = {}
        for line in done.stdout.splitlines():
            key, sep, value = line.partition(":")
            if sep and key in ("scene_file", "rays", "seed", "photon_export",
                               "rays_traced", "chunk_count", "chunk_size", "tracker_update"):
                values[key] = value.strip()
        require(values.get("rays_traced") == str(rays), f"{adapter} {case} did not complete every ray")
        require(values.get("photon_export") == "false", "No-export trace accidentally enabled photon export")
        if trackers:
            require(values.get("tracker_update") == "enabled", "Tracker update was not active")
        hs = json.loads(nonempty_file(sig).read_text(encoding="utf-8"))
        rh = json.loads(nonempty_file(hist).read_text(encoding="utf-8"))
        require(hs.get("schema") == "tonatiuhpp.a0.native-hits.v1", "Hit schema unexpected")
        require(rh.get("schema") == "tonatiuhpp.a0.native-ray-histories.v1", "Ray history schema unexpected")
        require(int(hs.get("hit_count", "0")) > 0, f"{case}: no physical hits captured")
        require(int(rh.get("ray_count", "0")) == rays, f"{case}: history did not include every ray")
        return values, hs, rh

    def check_traces(self) -> None:
        cases = [("cylinder", False), ("specular_two_reflections", False),
                 ("one_axis_tracker", True), ("sphere_material_plugin", False)]
        if self.args.mode == "installed":
            cases = [("cylinder", False), ("sphere_material_plugin", False)]
        for case, trackers in cases:
            left = self.trace("gui", case, 2048, trackers=trackers)
            right = self.trace("cli", case, 2048, trackers=trackers)
            require(left == right,
                    f"{case}: GUI-binary QCore and standalone QCore effective inputs/hits/histories differ")
            self.log(f"native-trace-{case}", "2048 seeded rays; exact scientific hit and ordered-history parity")

    def check_usage_and_scene_loading(self) -> None:
        require(self.gui.is_file() and self.cli.is_file() and self.gui != self.cli,
                "Two distinct executable files are required")
        for adapter in ("gui", "cli"):
            self.run(adapter, ["--help"], contains="Tonatiuh++ headless mode")
            self.run(adapter, ["validate-scene", str(self.scenes["cylinder"])],
                     contains="Scene validation succeeded:")
            self.run(adapter, ["validate-scene", str(self.scenes["sphere_material_plugin"])],
                     contains="Scene validation succeeded:")
            missing = self.runroot / "definitely_missing_m5_2.tnhpp"
            require(not missing.exists(), "Invalid missing-scene fixture")
            self.run(adapter, ["validate-scene", str(missing)], expected=1,
                     contains="Scene validation failed:")
            self.run(adapter, ["not-an-m5-command"], expected=2, contains="Unknown headless command")
        self.log("entrypoints-and-real-material-plugin", "QCore help, positive/negative scene loading and usage")

    def script_fixture(self, adapter: str, stem: str) -> tuple[bytes, ...]:
        fixture = self.root / "tests/scripts" / (stem + ".tnhpps")
        nonempty_file(fixture, "Tracked M3 scientific fixture missing")
        dest = self.runroot / "scripts" / stem / adapter
        dest.mkdir(parents=True, exist_ok=False)
        path = dest / fixture.name
        shutil.copyfile(fixture, path)
        elsewhere = dest / "foreign_cwd"
        result = self.run(adapter, ["run-script", str(path)], cwd=elsewhere,
                          contains=EXPECTED_M3[stem]["marker"], timeout=180)
        require(result.returncode == 0, "Script did not complete")
        artifacts = []
        for filename in EXPECTED_M3[stem]["files"]:
            artifact = nonempty_file(dest / filename, "Script-relative scientific output absent")
            require(not (elsewhere / filename).exists(), "Script output incorrectly relative to process CWD")
            artifacts.append(normalize_bytes(artifact))
            if filename.startswith("m3_flux_"):
                rows = artifact.read_text(encoding="utf-8").splitlines()
                require(len(rows) == 8, "Flux grid must contain exactly eight rows")
                data = []
                for row in rows:
                    fields = row.strip().split()
                    require(len(fields) == 8, "Flux grid must contain eight cells per row")
                    numbers = [float(x) for x in fields]
                    require(all(math.isfinite(x) and x >= 0 for x in numbers), "Flux grid has nonfinite/negative cells")
                    data.extend(numbers)
                if filename.endswith("front.txt"):
                    require(any(x > 0 for x in data), "Front flux grid has no signal")
            if filename == "m3_3_parity_result.csv":
                rows = artifact.read_text(encoding="utf-8").splitlines()
                require(len(rows) == 6 and rows[0] == "schema,m3.3-script-parity-v1", "Interception CSV schema invalid")
                require(float(rows[5].split(",", 1)[1]) > 0, "Interception CSV power is not positive")
            if filename == "m3_3_tracker_result.csv":
                rows = artifact.read_text(encoding="utf-8").splitlines()
                require(len(rows) == 12 and rows[0] == "schema,m3.3-tracker-parity-v1", "Tracker CSV schema invalid")
                require(rows[8] != rows[9], "Tracker angles did not change with sun azimuth")
        return tuple(artifacts)

    def check_script_parity(self) -> None:
        stems = list(EXPECTED_M3)
        if self.args.mode == "installed":
            stems = ["m3_3_gui_headless_parity", "m3_flux_shared_parity"]
        for stem in stems:
            x = self.script_fixture("gui", stem)
            y = self.script_fixture("cli", stem)
            require(x == y, f"{stem}: independent executables produced different scientific script artifacts")
            self.log("script-" + stem, "script-relative artifact byte equivalence; bounded native domain API")

    def check_cancel_recovery(self) -> None:
        cases = [
            ("m3_3_cancel_recover", 0, "M3.3 scripted cancellation/recovery PASSED:"),
            ("m3_3_cancel_inactive", 1, "tn.cancelTrace failed: no active script trace"),
            ("m3_3_cancel_invalid", 1, "options.noExport must be true"),
            ("m3_3_cancel_unfinished", 1, "unfinished trace was cancelled and joined"),
        ]
        if self.args.mode == "installed":
            cases = cases[:1]
        results: dict[str, dict[str, bytes | int]] = {}
        for name, expected, marker in cases:
            results[name] = {}
            for adapter in ("gui", "cli"):
                dest = self.runroot / "cancel" / name / adapter
                dest.mkdir(parents=True, exist_ok=False)
                fixture = self.root / "tests/scripts" / (name + ".tnhpps")
                shutil.copyfile(nonempty_file(fixture), dest / fixture.name)
                shutil.copyfile(nonempty_file(self.scenes["cylinder"]), dest / "cylinder.tnhpp")
                run = self.run(adapter, ["run-script", str(dest / fixture.name)],
                               cwd=dest / "foreign", expected=expected, contains=marker,
                               timeout=120)
                if expected != 0:
                    if name not in {"m3_3_cancel_unfinished"}:
                        require(f"{name}.tnhpps:2" in run.stderr.replace("\\", "/"),
                                f"{name}: script error did not report source line 2")
                else:
                    artifact = nonempty_file(dest / "m3_3_cancel_recovery.csv")
                    rows = artifact.read_text(encoding="utf-8").splitlines()
                    require(len(rows) == 7 and rows[0] == "schema,m3.3-script-cancel-v1",
                            "Cancellation/recovery scientific artifact invalid")
                    results[name][adapter] = normalize_bytes(artifact)
            if expected == 0:
                require(results[name]["gui"] == results[name]["cli"],
                        "Cancellation/recovery scientific result differs between executables")
            self.log("script-cancel-" + name, "matched completion/error contract and resource cleanup")

    def export_dir(self, adapter: str, flavor: str) -> Path:
        dest = self.runroot / "file_export" / flavor / adapter
        dest.mkdir(parents=True, exist_ok=False)
        return dest

    def file_export(self, adapter: str, flavor: str) -> Path:
        dest = self.export_dir(adapter, flavor)
        if flavor == "direct":
            self.run(adapter, ["trace-scene", str(self.scenes["cylinder"]),
                               "--rays", "2048", "--seed", "0",
                               "--photon-export-dir", str(dest),
                               "--photon-surface", "//Node/Shape", "--update-trackers"],
                     contains="Trace completed.", timeout=90)
        else:
            template = nonempty_file(self.root / "tests/scripts/m4_cli_file_export_parity.tnhpps.in")
            script_dir = self.runroot / "file_scripts" / adapter
            script_dir.mkdir(parents=True, exist_ok=False)
            script = script_dir / "m5_2_file_parity.tnhpps"
            src = template.read_text(encoding="utf-8")
            require(src.count("@SCENE_FILE@") == 1 and src.count("@EXPORT_DIRECTORY@") == 1,
                    "M4 canonical File script template unexpected")
            def qpath(p: Path) -> str:
                return p.resolve().as_posix().replace("\\", "/").replace("'", "\\'")
            script.write_text(src.replace("@SCENE_FILE@", qpath(self.scenes["cylinder"]))
                                  .replace("@EXPORT_DIRECTORY@", qpath(dest)), encoding="utf-8")
            self.run(adapter, ["run-script", str(script)],
                     contains="M4 CLI/script File photon parity fixture completed.", timeout=90)
        nonempty_file(dest / "PhotonMap.dat")
        nonempty_file(dest / "PhotonMap_parameters.txt")
        return dest

    def check_export_parity(self) -> None:
        dirs = {f"{adapter}-{flavor}": self.file_export(adapter, flavor)
                for adapter, flavor in (("gui", "direct"), ("cli", "direct"),
                                        ("gui", "script"), ("cli", "script"))}
        observations = {}
        for label, folder in dirs.items():
            records, power, count, surfaces = self.comparator.read_export(folder)
            require(count > 0 and math.isfinite(power) and power > 0 and bool(surfaces),
                    f"{label}: no scientific File signal")
            observations[label] = (records, power, count, surfaces)
        ref = observations["gui-direct"]
        for label, data in observations.items():
            require(data[:3] == ref[:3], f"{label}: File physical photon records/power differ from GUI CLI baseline")
        sha = {label: {p.name: sha256(p) for p in folder.iterdir() if p.is_file()}
               for label, folder in dirs.items()}
        self.journal["file_export_hashes"] = sha
        self.log("file-export-four-path-parity", f"{ref[2]} canonical photons; exact multiset and power equivalence; SHA256 recorded")
        # Fail closed against overwrite, including unrelated existing files.
        for adapter in ("gui", "cli"):
            occupied = self.runroot / "occupied" / adapter
            occupied.mkdir(parents=True, exist_ok=False)
            sentinel = occupied / "keep.txt"
            sentinel.write_bytes(b"M5.2: source must be preserved\n")
            before = sha256(sentinel)
            self.run(adapter, ["trace-scene", str(self.scenes["cylinder"]),
                               "--rays", "2048", "--seed", "0",
                               "--photon-export-dir", str(occupied)], expected=1,
                     contains="Photon export failed")
            require(sha256(sentinel) == before and not (occupied / "PhotonMap.dat").exists(),
                    f"{adapter}: rejected export changed an existing document")
        self.log("file-export-fail-closed", "both adapters preserve occupied directory contents")

    def finish(self) -> None:
        try:
            for label, path in self.scenes.items():
                nonempty_file(path, f"Required scene fixture {label} missing")
            self.check_usage_and_scene_loading()
            self.check_traces()
            self.check_script_parity()
            self.check_cancel_recovery()
            self.check_export_parity()
            self.journal["result"] = "PASS"
            self.save()
            print(f"M5.2 {self.args.mode.upper()} TWO-ENTRYPOINT SCIENTIFIC GATES PASSED", flush=True)
            print(f"M5.2 durable evidence: {self.runroot / 'evidence.json'}", flush=True)
        except BaseException as e:
            self.journal["result"] = "FAIL"
            self.journal["failure"] = str(e)
            self.save()
            print(f"M5.2 FAILURE: {e}", file=sys.stderr, flush=True)
            print(f"Evidence retained at: {self.runroot / 'evidence.json'}", file=sys.stderr, flush=True)
            raise


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--gui", required=True, help="Historical GUI executable, called with --headless")
    p.add_argument("--cli", required=True, help="Standalone QCoreApplication executable")
    p.add_argument("--source-root", required=True, help="Tonatiuh++ source repository root")
    p.add_argument("--output-root", required=True, help="Fresh acceptance evidence parent outside source fixtures")
    p.add_argument("--mode", choices=("build", "installed"), default="build")
    args = p.parse_args()
    Suite(args).finish()


if __name__ == "__main__":
    main()
