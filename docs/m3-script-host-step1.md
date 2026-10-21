# M3.2 — ScriptHost extraction, step 1 (local validation pending)

**Baseline:** `master` after merged M3.1 PR #32, merge `38c2994dfac75478a71d20f9506dd85d5e08f8cb` (2026-10-20). Hosted PR #32 Windows/Ubuntu/macOS CI passed; the maintainer's 33/33 scientific and 12/12 selected headless tests, native GUI Run/Flux Analysis, and GUI cancellation/recovery were successful. The M3.1 documents committed in PR #32 still show those latter milestones as pending and mislabel some Qt millisecond elapsed values as seconds; correct those records with a later M3.2 documentation update before PR, not by rewriting M3.1 source.

**Scope of this patch:** an initial M3.2 implementation step, **not M3.2/M3.3 completion**. This step intentionally makes no changes to GUI scene factory logic, ray propagation, RNG, `SimulationSession`, photons, numerical flux, or benchmark references.

## Implementation

- New `source/application/script/ScriptHost.{h,cpp}` owns one `QJSEngine` for each script entry point, and exposes a common `evaluate(source, fileName, line)`, error formatting, local path resolver and logging hooks. Uses Qt Core and Qt Qml; no Qt Widgets, SoQt, `MainWindow`, or `SceneTreeModel` headers.
- `ScriptWindow.cpp`, legacy `main.cpp` `.tnhpps` execution, and `HeadlessScriptHost.cpp` now all evaluate through `ScriptHost`. Existing GUI `tn`/`tonatiuh`, `NodeObject`, editor `print`/`printTimed`, and true-headless JS `tn` bootstrap remain deliberately unchanged. Legacy still creates a `MainWindow`, pending the scene API migration.
- `ScriptHost` registers `DataObject` in each engine. `DataObject::s_engine` and `DataObject::setEngine` are removed; array creation uses `qjsEngine(this)`. `DataObject.read/write` resolve relative to the active host's real script directory and raise normal JavaScript errors instead of displaying `QMessageBox`. Absolute paths work unchanged. The line-oriented CSV-like representation is retained.
- Added four bounded GTests under `ScientificScriptHost.*` for independent engines/arrays, relative file round trips, C++ file-error source locations, syntax errors and separate print sinks. Target links Qt6 Qml.

## Remaining M3.2 work (must not be claimed complete)

1. `NodeObject` still uses **process-global** `s_mainWindow` / `s_engine`, and the GUI still exposes `MainWindow` itself as `tn`. Per-host scene wrappers and a GUI-independent scene/domain `ScriptApi` are the next integration boundary.
2. Headless bootstrap still exposes only `tn.writeJson`, `tn.validateScene`, `tn.runBenchmark` and `tn.traceScene`. GUI `tn.getScene`, `tn.Clear`, `tn.InsertScene`, factory selection, `tn.FindInterception` and exporter controls are **not** available through the shared domain API yet. The added `DataObject` constructor is host-level capability, not scene parity.
3. `ScriptWindow` still temporarily modifies process-wide `project:` search paths for legacy non-DataObject users, and `NodeObject` retains borrowed scene pointers; their lifetime/path isolation must be handled in the next migration.
4. `ScriptRayTracer::Trace()` still returns success without tracing. Do not redirect new scripts to it; audit before retirement.
5. The 19 tracked `.tnhpps` examples are compatibility references; they were inventoried but **not executed** as a parity sweep. M3.3 will compare equivalent effective scenes and scripts between GUI and headless clients.

## Windows local validation

From `C:\OpenSource\tonatiuhpp` after applying the patch onto clean merged `master`:

```powershell
cmake --build build --config Release --target tonatiuhpp tonatiuhpp_scientific_equivalence_tests --parallel 8
ctest --test-dir build -C Release -R '^scientific[.]ScientificScriptHost[.]' --output-on-failure
ctest --test-dir build -C Release -R '^headless[.](run_script_cylinder_scene|run_script_rejects_gui_api|run_script_missing_file)$' --output-on-failure
ctest --test-dir build -C Release -R '^scientific[.]' --output-on-failure
```

Then run a short GUI editor script and a legacy non-`--headless` command-line `.tnhpps` using **only** `DataObject` + `print`; ensure both create the output next to the script and do not show a file dialog. Keep their input files small. Verify a deliberately missing input reports filename/line without crashing. Use ordinary Windows PowerShell quoting. Do not run large facility sweeps, the million-ray repeatability benchmark, or the frozen 500-million-ray benchmark.

The patch has only undergone static/context/whitespace verification in an isolated Linux container without Qt dependencies. **No Windows build, CTest, GUI smoke, hosted CI, PR, or branch was created here.** Update `PROJECT_STATUS.md` with local evidence and the actual package state in the eventual implementation PR.

Suggested conventional commit after acceptance:

`refactor(script): extract shared JavaScript host and isolate data IO`

Body: `Migrate GUI, legacy CLI and true-headless script evaluation to a common QJSEngine host. Resolve DataObject arrays and files per host rather than through static engine pointers or GUI dialogs. Add focused host isolation, path, and error regressions while deferring scene/script API parity to the remaining M3.2 packages.`
