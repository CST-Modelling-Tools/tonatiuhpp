# M3.2 — Shared mutable scene-document API (Step 3)

**State:** Implementation patch prepared on 2026-10-20 against the locally
accepted M3.2 Steps 1–2 feature branch. Windows compilation, CTest and native
GUI acceptance for this step are **not yet performed**.

## New shared contract

- `ScriptSceneServices` now exposes `tn.getScene()`, `tn.Clear()` and
  `tn.InsertScene(NodeObject)` through the **same QtQml/Coin API** in GUI, legacy
  CLI and true-headless `run-script`.
- `ScriptSceneDocument` is a per-invocation, GUI-independent in-memory scene
  document for true-headless scripts. It owns the scene via `LoadedScene`,
  inserts nodes at the document layout root, and repairs tracker-parent links.
- The GUI adapter retains the existing `MainWindow` document, scene-tree model,
  selection, undo, viewport refresh and modified-document policy; it does not
  instantiate a parallel GUI document. A GUI-only JavaScript forwarding proxy
  maps the three names to the shared facade and delegates other script calls
  back to `MainWindow` until later migration packages.
- `NodeObject` now takes a Coin reference to every wrapped node and releases it
  in its destructor. The GUI `Document::ClearScene` now releases **only the
  document's own Coin reference**, instead of draining other owners' refs;
  this is required for old GUI views and script wrappers to coexist safely
  across `tn.Clear()`. Previously exposed scene objects can stay alive, but
  they do **not** become the new active scene. New scene handles must be
  obtained again after clearing. The native GUI repeat-clear smoke is a
  required acceptance gate for this ownership change.
- Both `tn` and `tonatiuh` resolve to the same facade within a host. Valid
  scene operations preserve the historical `undefined` return for `Clear` and
  `InsertScene`. Invalid insertion raises a JavaScript error, with source line.

## Differences and deferred work

- The GUI continues to honor the current scene-tree selection for insertion.
  The nonvisual headless adapter always inserts under the scene layout root.
  Parity tests must use an unselected/root GUI scene.
- `tn.traceScene({scene: ...})` and `tn.validateScene(file)` remain **file-based**.
  In-memory script edits are not yet connected to tracing, interception, or
  export; this is an open M3.2/M3.3 integration gate.
- GUI-only methods including screenshot, selection and UI dialogs are still
  delegated to the legacy `MainWindow` implementation. Headless calls continue
  to fail explicitly when unsupported. `fileSave`, scene file-open/load, native
  interception, additional exporter operations and full sample-script parity
  are later work, **not** claimed by this patch.
- The 19 tracked `.tnhpps` examples remain regression inputs; this slice
  covers their common scene-create/edit entry points, not complete execution
  of every facility sweep or scripted simulation.
- Do not change photon output, RNG, tracker defaults, scientific reference
  files, or the frozen 500-million-ray benchmark for this package.

## Local acceptance order (Windows/MSVC Release)

1. Apply only to the existing `refactor/m3-script-host-step1` worktree **after**
   M3.2 Step 2; run `git apply --check --unidiff-zero` first.
2. Incrementally build `tonatiuhpp` and
   `tonatiuhpp_scientific_equivalence_tests` in Release.
3. Run `ctest --test-dir build -C Release -R '^scientific[.]ScientificScript' --output-on-failure`;
   **9 scripting CTests** expected (4 host + 2 scene-factory + 3 document).
4. Run `ctest --test-dir build -C Release -R '^headless[.]run_script_(scene_document|cylinder_scene|rejects_gui_api|missing_file)$' --output-on-failure`;
   **4 selected headless CTests** expected (1 new + 3 existing).
5. Run `ctest --test-dir build -C Release -R '^scientific[.]' --output-on-failure`;
   **42 scientific tests** expected if discovery matches the validated Step 2
   build (39 existing + 3 new).
6. Execute the bundled `m3_2_scene_document_parity.tnhpps` and
   `m3_2_scene_document_repeat.tnhpps` via both `-i` (legacy GUI-backed CLI)
   and `--headless run-script`. Require exit `0`, identical success messages.
   Execute `m3_2_scene_document_error.tnhpps` via both paths; require exit `1`,
   line `2` and the same invalid-NodeObject explanation without a GUI prompt.
7. Open the parity script in the native GUI Script Editor. Require a successful
   log, node `M3_2_Document_Smoke` visible in the scene tree, sun/scene changes,
   and a responsive GUI. Run the repeat script and confirm `M3_2_After_Clear`
   is visible and the previous node is no longer in the active scene tree.

Only after these checks is Step 3 **locally accepted**. No hosted CI or PR
should be triggered yet. Batch the eventual M3.2 PR and documentation, and
leave the frozen benchmark unchanged.

Proposed eventual commit: `refactor(script): share mutable scene document operations`
