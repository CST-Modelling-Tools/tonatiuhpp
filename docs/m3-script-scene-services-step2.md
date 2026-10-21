# M3.2 — Per-host NodeObject scene/factory isolation (Step 2)

**Prerequisite:** M3.2 Step 1, including both legacy print/ownership corrections,
validated locally on Windows: Release builds, 37/37 scientific, 3/3 selected
headless script cases, native GUI editor, and both CLI success/error smoke cases.
The baseline is the feature branch containing those UNCOMMITTED changes, not
clean `master`.

## Implemented in this patch

- `NodeObject` no longer holds process-global `MainWindow*`/`QJSEngine*`, nor
  includes GUI widget or `PluginManager` headers. It uses `qjsEngine(this)` and
  engine-local `ScriptSceneServices` to wrap nodes and create factories.
- `ScriptSceneServices` is a Qt Core/Qml + Coin boundary that receives scene,
  typed plugin creation and optional interception callbacks per host.
- `GuiScriptSceneAdapter` binds the existing GUI document and plugin maps to
  that interface, with `QPointer<MainWindow>` guards. Legacy CLI and GUI editor
  install it independently. Their existing `tn` objects remain untouched.
- `CorePluginRegistry::createSceneNode` offers equivalent factory creation to
  true-headless scripts, including loaded plugin factories, without a widget.
  The true-headless host registers `NodeObject` and a lazy factory-creation
  callback; it does **not** yet expose `tn.Clear`, `tn.InsertScene`, or
  `tn.getScene`.
- Missing factory names and invalid `setPart` arguments report JavaScript
  errors instead of dereferencing null pointers.
- Two focused `ScientificScriptSceneServices` tests cover independent per-host
  factory contexts and unknown-factory errors.

## Scope limits and acceptance

This is **not full M3.2**. GUI still exposes `MainWindow` as `tn` and the
headless `tn` object still has its established `traceScene`/benchmark/JSON/
validate-only methods. The shared mutable scene/document API, scripted save,
interception and exporter parity, borrowed node lifetime after document
replacement, and end-to-end script parity remain open. Do not claim that
all 19 tracked scripts execute headlessly yet.

Run locally on Windows in this order:

```powershell
cmake --build build --config Release --target tonatiuhpp tonatiuhpp_scientific_equivalence_tests --parallel 8
ctest --test-dir build -C Release -R '^scientific[.]ScientificScript(SceneServices|Host)[.]' --output-on-failure
ctest --test-dir build -C Release -R '^headless[.](run_script_cylinder_scene|run_script_rejects_gui_api|run_script_missing_file)$' --output-on-failure
ctest --test-dir build -C Release -R '^scientific[.]' --output-on-failure
```

After automated acceptance, open in the native GUI Script Editor a short
synthetic scene script that exercises `new NodeObject`, `createShape`,
`insertSurface("Planar")`, `insertProfile("Box")`, `tn.Clear()`, `tn.InsertScene()`
and `tn.getScene()`; check the new document and an error-free log. Keep the
existing Step 1 DataObject smoke available as a separate regression.

No frozen 500M benchmark, large facility sweep, hosted CI or PR should be run
until local results pass and the full M3.2 code/doc scope is ready for review.
This patch is statically checked only; native Qt/MSVC compilation is pending.

Suggested eventual conventional commit (after all M3.2 work):

`refactor(script): unify per-host scene and node APIs`
