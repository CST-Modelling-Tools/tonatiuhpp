# IFW Release Checklist

Use this checklist when preparing a Tonatiuh++ release that must be visible to the Qt Installer Framework MaintenanceTool.

## 1. Set the application version

- Update `source/CMakeLists.txt`:
  - `project(TonatiuhPP VERSION X.Y.Z.W LANGUAGES CXX)`
- Ensure the version matches the intended release exactly.

Example:
- App version: `0.1.8.18`
- Tag: `v0.1.8.18`

## 2. Create the release tag

- Create and push the Git tag in this format:
  - `vX.Y.Z`
  - or `vX.Y.Z.W` if using four components
- The tag must match the application version, with only the leading `v` added.
- Pushing a matching tag to GitHub triggers the repository `Release` workflow.

## 3. Build release payloads

Preferred path:

- Let the GitHub Actions `Release` workflow build all release payloads from the exact tagged commit.
- Before packaging, build runtime help from the Sphinx sources:
  - `python scripts/build_runtime_help.py`
  - Confirm `help/html/index.html` exists.
- Build and run the test suite before tagging:
  - `cmake --build build --target all`
  - `ctest --test-dir build --output-on-failure`
- On Windows installed-runtime test configurations, run CMake Install before CTest so `TONATIUHPP_TEST_EXECUTABLE` points at a fresh installed executable and matching runtime files.

The workflow currently publishes GitHub release distribution assets and IFW online repositories:

- Windows IFW installer and checksum
- Linux IFW installer and checksum
- macOS arm64 IFW installer and checksum
- Linux archive and checksum
- macOS arm64 archive and checksum
- IFW repository: `windows`
- IFW repository: `linux`
- IFW repository: `macos`

The macOS release artifact is built for Apple Silicon / arm64 Mac hardware.

The official update-capable distribution path on every platform is the IFW installer. Archive assets may remain available as manual convenience packages, but they are not the updater-enabled installation path.

## 4. Verify IFW repositories

Confirm GitHub Pages contains all platform repositories:

- `https://cst-modelling-tools.github.io/tonatiuhpp/windows/Updates.xml`
- `https://cst-modelling-tools.github.io/tonatiuhpp/linux/Updates.xml`
- `https://cst-modelling-tools.github.io/tonatiuhpp/macos/Updates.xml`

Each repository must contain:

- `Updates.xml`
- generated IFW package data for `com.tonatiuhpp.app`
- package metadata for the release version

## 5. Verify installer update configuration

Each IFW installer must point to its platform repository:

- Windows installer: `https://cst-modelling-tools.github.io/tonatiuhpp/windows`
- Linux installer: `https://cst-modelling-tools.github.io/tonatiuhpp/linux`
- macOS arm64 installer: `https://cst-modelling-tools.github.io/tonatiuhpp/macos`

The application does not implement GitHub release download logic for updates. It asks the installed IFW MaintenanceTool to check and apply updates.

## 6. Verify GitHub release distribution assets

GitHub release assets are for distribution and manual download, not for updater metadata.

Confirm the release belongs to:

- `https://github.com/CST-Modelling-Tools/tonatiuhpp`

Confirm the release page is for the correct tag:

- `https://github.com/CST-Modelling-Tools/tonatiuhpp/releases/tag/v<version>`

Confirm uploaded assets and checksum files match the workflow output for the tagged commit.

## 7. Final release-completion sanity check

Before declaring the release complete, verify:

- App version in `source/CMakeLists.txt` is correct
- Git tag matches the app version
- Runtime help was built and is present under `help/html`
- CTest passes for unit tests and supported headless smoke tests
- Windows installed-runtime CTest was run after CMake Install
- Release workflow completed successfully from the matching tag
- GitHub Pages deploy completed successfully
- All three IFW platform repositories contain `Updates.xml`
- Windows, Linux, and macOS arm64 IFW installers are attached to the GitHub release
- Distribution assets are uploaded to the correct GitHub release
- Windows `.tnhpp` and `.tnhpps` file associations launch the installed executable
- Windows executable and associated file types show the expected Tonatiuh++ icon
- `Help > Documentation` opens the packaged runtime help on each platform
- Linux and macOS artifacts launch on clean machines without developer environment variables
- The v0.1.8.25 installed-runtime headless automation checklist below passes on Windows, Ubuntu, and macOS after an IFW updater install

## 8. v0.1.8.25 installed-runtime headless automation validation

Run this checklist only after the `v0.1.8.25` IFW repositories are published and a latest published IFW baseline install, normally `v0.1.8.24`, has been updated through the Tonatiuh++ updater. Use the IFW install path, not archive assets, because the goal is to validate the updater-installed runtime.

Before each platform run, use a clean terminal without developer Qt, Coin, SoQt, or build-tree paths. Clear `QT_PLUGIN_PATH`, `QT_QPA_PLATFORM_PLUGIN_PATH`, and any temporary library-path overrides unless a step explicitly enables `QT_DEBUG_PLUGINS`.

### 8.1 Updater checks

From the baseline install:

- Start Tonatiuh++ and confirm the delayed startup update check is non-blocking.
- Open `Help > Updates`.
- Confirm the dialog shows the baseline installed version and a MaintenanceTool path.
- Click `Check`.
- Confirm `v0.1.8.25` is detected and `Install Updates` is enabled.
- Optionally run the platform MaintenanceTool directly with `check-updates`; output should indicate that updates are available.
- Click `Install Updates` in Tonatiuh++.
- Confirm Tonatiuh++ prompts for unsaved work, starts the MaintenanceTool, and closes before files are replaced.
- Complete the MaintenanceTool update.
- Restart Tonatiuh++ manually after the updater exits.

After the update:

- Open `Help > Updates` and confirm the installed version is `0.1.8.25`.
- Run `check-updates` again and confirm the install is up to date.
- Confirm the GUI executable launches from the installed location.
- Confirm the headless executable path below launches without extra runtime environment variables.

MaintenanceTool commands:

Windows PowerShell:

```powershell
$InstallRoot = Join-Path $env:USERPROFILE "tonatiuhpp"
$MaintenanceTool = Join-Path $InstallRoot "MaintenanceTool.exe"
if (-not (Test-Path $MaintenanceTool)) {
  $MaintenanceTool = Join-Path $InstallRoot "maintenancetool.exe"
}
& $MaintenanceTool check-updates
```

Ubuntu shell:

```bash
INSTALL_ROOT="${HOME}/tonatiuhpp"
for candidate in MaintenanceTool maintenancetool MaintenanceTool.run maintenancetool.run; do
  if [ -x "${INSTALL_ROOT}/${candidate}" ]; then
    MAINTENANCE_TOOL="${INSTALL_ROOT}/${candidate}"
    break
  fi
done
test -n "${MAINTENANCE_TOOL:-}"
"${MAINTENANCE_TOOL}" check-updates
```

macOS shell:

```bash
INSTALL_ROOT="${HOME}/tonatiuhpp"
for candidate in \
  "MaintenanceTool.app/Contents/MacOS/MaintenanceTool" \
  "maintenancetool.app/Contents/MacOS/maintenancetool" \
  "MaintenanceTool" \
  "maintenancetool"; do
  if [ -x "${INSTALL_ROOT}/${candidate}" ]; then
    MAINTENANCE_TOOL="${INSTALL_ROOT}/${candidate}"
    break
  fi
done
test -n "${MAINTENANCE_TOOL:-}"
"${MAINTENANCE_TOOL}" check-updates
```

### 8.2 Smoke fixture setup

Adjust `InstallRoot` or `INSTALL_ROOT` if the IFW installer used a non-default target directory.

Windows PowerShell:

```powershell
Remove-Item Env:\QT_PLUGIN_PATH -ErrorAction SilentlyContinue
Remove-Item Env:\QT_QPA_PLATFORM_PLUGIN_PATH -ErrorAction SilentlyContinue

$InstallRoot = Join-Path $env:USERPROFILE "tonatiuhpp"
$Tn = Join-Path $InstallRoot "bin\tonatiuhpp.exe"
$MaintenanceTool = Join-Path $InstallRoot "MaintenanceTool.exe"
if (-not (Test-Path $MaintenanceTool)) {
  $MaintenanceTool = Join-Path $InstallRoot "maintenancetool.exe"
}
$Scene = Join-Path $InstallRoot "examples\benchmarks\cylinder.tnhpp"
$Work = Join-Path $env:TEMP "tonatiuhpp-0.1.8.25-headless-rc"
New-Item -ItemType Directory -Force $Work | Out-Null

$BenchmarkConfig = Join-Path $Work "benchmark_config.json"
$BenchmarkResult = Join-Path $Work "benchmark_result.json"
$Script = Join-Path $Work "headless_smoke.tnhpps"
$ScriptResult = Join-Path $Work "script_result.json"

$SceneJs = $Scene -replace '\\','/'
$BenchmarkConfigJs = $BenchmarkConfig -replace '\\','/'
$BenchmarkResultJs = $BenchmarkResult -replace '\\','/'
$ScriptResultJs = $ScriptResult -replace '\\','/'
$Utf8NoBom = New-Object System.Text.UTF8Encoding $false

$BenchmarkJson = @"
{
  "benchmark": "benchmark_v1",
  "scene_file": "$SceneJs",
  "rays": 10,
  "seed": 123456789,
  "worker_count": 1,
  "chunk_size": 10,
  "target_side_id": 1,
  "target_bounds": {
    "x_min": -2.0,
    "x_max": 2.0,
    "y_min": -2.0,
    "y_max": 2.0
  },
  "target_grid": {
    "width": 4,
    "height": 4
  },
  "photon_export": false,
  "output_file": "$BenchmarkResultJs"
}
"@
[System.IO.File]::WriteAllText($BenchmarkConfig, $BenchmarkJson, $Utf8NoBom)

$ScriptText = @"
print("headless run-script smoke");

var validated = tn.validateScene("$SceneJs");
if (!validated) {
  throw new Error("tn.validateScene returned false");
}

var trace = tn.traceScene({
  scene: "$SceneJs",
  rays: 10,
  seed: 123456789,
  noExport: true
});
if (!trace || trace.rays_traced !== 10 || trace.photon_export !== false) {
  throw new Error("tn.traceScene did not return the expected no-export summary");
}
print("trace rays " + trace.rays_traced);

var benchmarkExitCode = tn.runBenchmark("$BenchmarkConfigJs");
if (benchmarkExitCode !== 0) {
  throw new Error("tn.runBenchmark returned " + benchmarkExitCode);
}

tn.writeJson("$ScriptResultJs", {
  schema: "tonatiuhpp.headless.result",
  version: 1,
  status: "ok",
  validation: {
    ok: validated,
    scene_file: "$SceneJs"
  },
  trace: trace,
  benchmark: {
    config_file: "$BenchmarkConfigJs",
    result_file: "$BenchmarkResultJs",
    exit_code: benchmarkExitCode
  }
});
"@
[System.IO.File]::WriteAllText($Script, $ScriptText, $Utf8NoBom)
```

Ubuntu shell:

```bash
unset QT_PLUGIN_PATH QT_QPA_PLATFORM_PLUGIN_PATH LD_LIBRARY_PATH

INSTALL_ROOT="${HOME}/tonatiuhpp"
TN="${INSTALL_ROOT}/bin/tonatiuhpp"
for candidate in MaintenanceTool maintenancetool MaintenanceTool.run maintenancetool.run; do
  if [ -x "${INSTALL_ROOT}/${candidate}" ]; then
    MAINTENANCE_TOOL="${INSTALL_ROOT}/${candidate}"
    break
  fi
done
test -n "${MAINTENANCE_TOOL:-}"
SCENE="${INSTALL_ROOT}/examples/benchmarks/cylinder.tnhpp"
WORK="${TMPDIR:-/tmp}/tonatiuhpp-0.1.8.25-headless-rc"
mkdir -p "${WORK}"

BENCHMARK_CONFIG="${WORK}/benchmark_config.json"
BENCHMARK_RESULT="${WORK}/benchmark_result.json"
SCRIPT="${WORK}/headless_smoke.tnhpps"
SCRIPT_RESULT="${WORK}/script_result.json"

cat > "${BENCHMARK_CONFIG}" <<EOF
{
  "benchmark": "benchmark_v1",
  "scene_file": "${SCENE}",
  "rays": 10,
  "seed": 123456789,
  "worker_count": 1,
  "chunk_size": 10,
  "target_side_id": 1,
  "target_bounds": {
    "x_min": -2.0,
    "x_max": 2.0,
    "y_min": -2.0,
    "y_max": 2.0
  },
  "target_grid": {
    "width": 4,
    "height": 4
  },
  "photon_export": false,
  "output_file": "${BENCHMARK_RESULT}"
}
EOF

cat > "${SCRIPT}" <<EOF
print("headless run-script smoke");

var validated = tn.validateScene("${SCENE}");
if (!validated) {
  throw new Error("tn.validateScene returned false");
}

var trace = tn.traceScene({
  scene: "${SCENE}",
  rays: 10,
  seed: 123456789,
  noExport: true
});
if (!trace || trace.rays_traced !== 10 || trace.photon_export !== false) {
  throw new Error("tn.traceScene did not return the expected no-export summary");
}
print("trace rays " + trace.rays_traced);

var benchmarkExitCode = tn.runBenchmark("${BENCHMARK_CONFIG}");
if (benchmarkExitCode !== 0) {
  throw new Error("tn.runBenchmark returned " + benchmarkExitCode);
}

tn.writeJson("${SCRIPT_RESULT}", {
  schema: "tonatiuhpp.headless.result",
  version: 1,
  status: "ok",
  validation: {
    ok: validated,
    scene_file: "${SCENE}"
  },
  trace: trace,
  benchmark: {
    config_file: "${BENCHMARK_CONFIG}",
    result_file: "${BENCHMARK_RESULT}",
    exit_code: benchmarkExitCode
  }
});
EOF
```

macOS shell:

```bash
unset QT_PLUGIN_PATH QT_QPA_PLATFORM_PLUGIN_PATH DYLD_LIBRARY_PATH

INSTALL_ROOT="${HOME}/tonatiuhpp"
TN="${INSTALL_ROOT}/TonatiuhPP.app/Contents/MacOS/TonatiuhPP"
for candidate in \
  "MaintenanceTool.app/Contents/MacOS/MaintenanceTool" \
  "maintenancetool.app/Contents/MacOS/maintenancetool" \
  "MaintenanceTool" \
  "maintenancetool"; do
  if [ -x "${INSTALL_ROOT}/${candidate}" ]; then
    MAINTENANCE_TOOL="${INSTALL_ROOT}/${candidate}"
    break
  fi
done
test -n "${MAINTENANCE_TOOL:-}"
SCENE="${INSTALL_ROOT}/TonatiuhPP.app/Contents/Resources/examples/benchmarks/cylinder.tnhpp"
WORK="${TMPDIR:-/tmp}/tonatiuhpp-0.1.8.25-headless-rc"
mkdir -p "${WORK}"

BENCHMARK_CONFIG="${WORK}/benchmark_config.json"
BENCHMARK_RESULT="${WORK}/benchmark_result.json"
SCRIPT="${WORK}/headless_smoke.tnhpps"
SCRIPT_RESULT="${WORK}/script_result.json"

cat > "${BENCHMARK_CONFIG}" <<EOF
{
  "benchmark": "benchmark_v1",
  "scene_file": "${SCENE}",
  "rays": 10,
  "seed": 123456789,
  "worker_count": 1,
  "chunk_size": 10,
  "target_side_id": 1,
  "target_bounds": {
    "x_min": -2.0,
    "x_max": 2.0,
    "y_min": -2.0,
    "y_max": 2.0
  },
  "target_grid": {
    "width": 4,
    "height": 4
  },
  "photon_export": false,
  "output_file": "${BENCHMARK_RESULT}"
}
EOF

cat > "${SCRIPT}" <<EOF
print("headless run-script smoke");

var validated = tn.validateScene("${SCENE}");
if (!validated) {
  throw new Error("tn.validateScene returned false");
}

var trace = tn.traceScene({
  scene: "${SCENE}",
  rays: 10,
  seed: 123456789,
  noExport: true
});
if (!trace || trace.rays_traced !== 10 || trace.photon_export !== false) {
  throw new Error("tn.traceScene did not return the expected no-export summary");
}
print("trace rays " + trace.rays_traced);

var benchmarkExitCode = tn.runBenchmark("${BENCHMARK_CONFIG}");
if (benchmarkExitCode !== 0) {
  throw new Error("tn.runBenchmark returned " + benchmarkExitCode);
}

tn.writeJson("${SCRIPT_RESULT}", {
  schema: "tonatiuhpp.headless.result",
  version: 1,
  status: "ok",
  validation: {
    ok: validated,
    scene_file: "${SCENE}"
  },
  trace: trace,
  benchmark: {
    config_file: "${BENCHMARK_CONFIG}",
    result_file: "${BENCHMARK_RESULT}",
    exit_code: benchmarkExitCode
  }
});
EOF
```

### 8.3 Headless command run

Run the full command set after the updater install completes.

Windows PowerShell:

```powershell
& $Tn --headless --help
& $Tn --headless validate-scene $Scene
& $Tn --headless trace-scene $Scene --rays 10 --seed 123456789 --no-export
& $Tn --headless benchmark $BenchmarkConfig
& $Tn --headless run-script $Script
```

Ubuntu and macOS shell:

```bash
"${TN}" --headless --help
"${TN}" --headless validate-scene "${SCENE}"
"${TN}" --headless trace-scene "${SCENE}" --rays 10 --seed 123456789 --no-export
"${TN}" --headless benchmark "${BENCHMARK_CONFIG}"
"${TN}" --headless run-script "${SCRIPT}"
```

### 8.4 Expected output and artifacts

All commands must exit with status `0`.

- `--headless --help` prints `Tonatiuh++ headless mode`, lists `validate-scene`, `trace-scene`, `benchmark`, and `run-script`, and lists the first headless script API.
- `validate-scene` prints `Scene validation succeeded:` followed by the absolute installed `cylinder.tnhpp` path.
- `trace-scene --no-export` prints `Trace completed.`, `scene_file:`, `rays: 10`, `seed: 123456789`, `photon_export: false`, `export_path: none`, `rays_traced: 10`, `elapsed_seconds:`, `rays_per_second:`, `worker_count:`, `chunk_count:`, and `chunk_size:`.
- `benchmark` prints `Benchmark completed.`, the same no-export and scheduling fields, `total_power_mw:`, `maximum_flux_mw_m2:`, `result_file:`, and `Result written:`.
- `run-script` prints `headless run-script smoke`, `trace rays 10`, benchmark summary lines, and `result_file:`.
- The benchmark JSON exists at `benchmark_result.json` and contains `schema_version`, `benchmark`, `scene_file`, `rays`, `seed`, `elapsed_seconds`, `rays_per_second`, `worker_count`, `chunk_count`, `chunk_size`, `total_power_mw`, `maximum_flux_mw_m2`, and `flux_grid_sha256`.
- The script JSON exists at `script_result.json` and contains `schema: tonatiuhpp.headless.result`, `version: 1`, `status: ok`, `validation.ok: true`, `trace.rays_traced: 10`, `trace.photon_export: false`, and `benchmark.exit_code: 0`.
- No photon export file is created by `trace-scene`, `benchmark`, or `run-script`; stdout must report `photon_export: false` and `export_path: none`.

Treat any stderr containing missing DLL/shared-library errors, `Scene load failed`, `Trace failed`, `Benchmark failed`, `Script execution failed`, or Qt platform plugin load failures as a release-blocking validation failure.

### 8.5 Runtime dependency and Qt plugin checks

Windows:

```powershell
Test-Path (Join-Path $InstallRoot "bin\Qt6Core.dll")
Test-Path (Join-Path $InstallRoot "bin\platforms\qwindows.dll")
& $Tn --headless --help
$env:QT_DEBUG_PLUGINS = "1"
& $Tn
& $MaintenanceTool check-updates
Remove-Item Env:\QT_DEBUG_PLUGINS -ErrorAction SilentlyContinue
```

Expected: the GUI launches, MaintenanceTool runs, Qt plugin diagnostics do not report a failed `qwindows` load, and the loaded plugin path is under the installed runtime.

Ubuntu:

```bash
test -x "${INSTALL_ROOT}/bin/tonatiuhpp"
test -x "${INSTALL_ROOT}/bin/tonatiuhpp-bin"
test -f "${INSTALL_ROOT}/bin/qt.conf"
test -f "${INSTALL_ROOT}/bin/platforms/libqxcb.so"
test -f "${INSTALL_ROOT}/lib/libQt6Core.so.6"
LD_LIBRARY_PATH="${INSTALL_ROOT}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}" ldd "${INSTALL_ROOT}/bin/tonatiuhpp-bin" | tee "${WORK}/ldd-tonatiuhpp.txt"
LD_LIBRARY_PATH="${INSTALL_ROOT}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}" ldd "${INSTALL_ROOT}/bin/platforms/libqxcb.so" | tee "${WORK}/ldd-libqxcb.txt"
! grep -q "not found" "${WORK}/ldd-tonatiuhpp.txt" "${WORK}/ldd-libqxcb.txt"
QT_DEBUG_PLUGINS=1 "${TN}"
QT_DEBUG_PLUGINS=1 "${MAINTENANCE_TOOL}" check-updates
```

Expected: `ldd` resolves bundled Qt dependencies through the installed `lib/` directory, the GUI launches on a desktop session, MaintenanceTool runs, Qt plugin diagnostics do not report a failed `xcb` load, and plugin paths are under the installed runtime.

macOS:

```bash
test -x "${TN}"
test -d "${INSTALL_ROOT}/TonatiuhPP.app/Contents/Frameworks"
find "${INSTALL_ROOT}/TonatiuhPP.app/Contents" -name 'libqcocoa.dylib' -print -quit | grep -q .
otool -L "${TN}" | tee "${WORK}/otool-tonatiuhpp.txt"
! grep -E '/Users/runner|/opt/Qt|third_party/_install|/build/' "${WORK}/otool-tonatiuhpp.txt"
QT_DEBUG_PLUGINS=1 "${TN}"
QT_DEBUG_PLUGINS=1 "${MAINTENANCE_TOOL}" check-updates
```

Expected: the app bundle contains Qt frameworks and the Cocoa platform plugin, `otool` does not show build-tree or CI runner dependency paths, the GUI launches, MaintenanceTool runs, Qt plugin diagnostics do not report a failed `cocoa` load, and plugin paths are inside the app bundle.

### 8.6 Rollback and failure notes

- Manual restart after a successful updater install is expected and is not a failure.
- If update detection fails, confirm the platform repository URL serves `Updates.xml`, confirm the installed MaintenanceTool path, preserve the `check-updates` output, and do not substitute GitHub release assets for the IFW updater path.
- If MaintenanceTool fails before replacing files, keep the baseline install intact, save the MaintenanceTool log/output, and retry only after confirming repository availability and network/proxy behavior.
- If the update partially installs or the executable no longer launches, preserve the install directory and updater output for diagnosis. Reinstall the latest published IFW baseline into a fresh directory or restore a VM snapshot before retrying the updater path.
- If only the GUI fails but headless commands pass, collect terminal output with `QT_DEBUG_PLUGINS=1`, OS crash details, GPU/OpenGL details, and the headless smoke command results.
- If headless commands fail because bundled dependencies or plugins are missing, treat the platform package as invalid; do not tag or publish replacement artifacts until the packaging cause is fixed and the updater path passes again.
