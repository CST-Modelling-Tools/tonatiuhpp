# M3.2 Step 4A — Tracked legacy script compatibility baseline

**State:** source patch prepared; Windows/Qt runtime validation pending. This is
not the M3.2 or M3.3 exit gate. Apply after the locally accepted Steps 1–3 and
the Step 3 error-origin correction on the existing feature branch. No files in
`examples/` are modified by this patch.

## Inventory (the 19 tracked `.tnhpps` scripts)

The repository currently contains 19 scripts under `examples/`. The script
calls below were counted from the tracked source (including commented mentions
where noted). This inventory guides **test selection**, not a blanket claim of
working headless parity.

| Script family | Files | Primary dependencies | Step 4A execution |
| --- | ---: | --- | --- |
| `devices/CPC` | 2 | NodeObject, `tn.Clear`, `tn.InsertScene`, sun/camera/terrain edits, standard and FunctionXYZ shapes | **Both execute unchanged in bounded tests** |
| `facilities/Fresnel` | 2 | Complex scene generator, dynamic shape/material plugins, GUI-oriented save mention; interception sweep | Compile only; no facility construction or tracing |
| `facilities/Proteas/2020` | 3 | DataObject CSV input, tracker armatures, optional OBJ rendering, interception | Compile only; sidecar data/OBJ and headless GUI-dialog dependencies not yet qualified |
| `facilities/Proteas/SIWAR` | 4 | DataObject CSV, tower generation, tracker setup, interception sweep | Compile only; large runs excluded |
| `facilities/SolarPilot` | 2 | Layout CSV, tower generation, air/sun settings, interception | Compile only; missing input/layout not simulated |
| `facilities/procedural` | 5 | Hundreds/thousands of procedural heliostats and trackers | Compile only; unbounded scene-generation workload excluded |
| `optimization/receiver` | 1 | CSV input, optimization and interception | Compile only; optimization excluded |

Across the 19 tracked files, the source references `tn.getScene` in 19,
`tn.Clear` and `tn.InsertScene` in 13 each, `new NodeObject` in 13,
`new DataObject` in 10, and `tn.FindInterception` in 8. One additional
`tn.fileSave()` mention in Fresnel is **commented out** and is not an active
execution requirement.

## Implemented

1. A `ScientificScriptLegacyCorpus` syntax smoke compiles **all 19 unmodified
   tracked files** with QJSEngine's `Function` constructor. It explicitly does
   not execute them, so it cannot start a facility ray trace or overwrite CSVs.
2. Two bounded `ScientificScriptLegacyCorpus` tests execute the unmodified
   `examples/devices/CPC/CPC-rotated.tnhpps` and `CPC-rotated-step.tnhpps` via
   the shared ScriptHost, CorePluginRegistry and an owned ScriptSceneDocument.
   They assert the scene root, the CPC hierarchy and the two/eight CPC children.
3. Two new `headless.run_script_cpc_*` CTests invoke **the real headless CLI**
   with the original sample paths. The suite target brings up the FunctionXYZ
   plugin required by the curved sample. No fixture rewrites.
4. `NodeObject.setPart("armature", ...)` now checks that the argument is a
   `TrackerArmature` node from the **same script engine** before assignment.
   Correct historical `t.setPart("armature", hLF)` and `t.setPart("armature", azel)`
   remain supported; wrong-type/cross-host nodes now fail with a JS error rather
   than compromising the tracker tree. One test covers wrong-type rejection and
   valid built-in armature assignment.

## Explicit limitations

- This is a **bounded compatibility baseline**, not proof that 19 scripts all
  execute or produce equivalent scientific results on both CLIs and the GUI.
- `tn.FindInterception` currently works through the legacy GUI/FluxAnalysis
  path. Its **true-headless equivalent is not yet implemented** and the 8
  scripts using it are **not executed** in this patch. The GUI FluxAnalysis
  implementation still depends on widgets/SceneTreeModel; do not silently
  substitute its GUI implementation into the true-headless host.
- Some scripts use large loops, sidecar `.csv`/`.obj`, and C++ tracker render
  paths that can show dialogs; they need independent bounded fixtures before
  automated execution. `ScriptRayTracer::Trace()` is a no-op and must remain
  unexposed until replaced by real session tracing.
- `tn.fileSave` was found only in a **comment** in the tracked Fresnel script.
  Save/reload equivalence remains pending rather than added by this package.
- GUI startup, scene generation and any visual changes must still be checked
  natively; automated headless tests do not prove GUI visual parity.

## Local Windows acceptance

On the existing uncommitted feature branch, apply the patch only after checking
`git status --short`. It deliberately does **not** touch `PROJECT_STATUS.md`
because local changes previously conflicted. Do not reset local changes.

```powershell
cmake --build build --config Release `
  --target tonatiuhpp tonatiuhpp_scientific_equivalence_tests --parallel 8
if ($LASTEXITCODE -ne 0) { throw "Release build failed" }

ctest --test-dir build -C Release `
  -R '^scientific[.]ScientificScript' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "Focused scientific script tests failed" }
# Expected: 14/14 = 10 existing + 4 new.

ctest --test-dir build -C Release `
  -R '^headless[.](run_script_cpc_curved|run_script_cpc_faceted|run_script_scene_document|run_script_cylinder_scene|run_script_rejects_gui_api|run_script_missing_file)$' `
  --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "Selected headless tests failed" }
# Expected: 6/6 = 4 existing selected + 2 new CPC tests.

ctest --test-dir build -C Release -R '^scientific[.]' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "Full scientific tests failed" }
# Expected: 47/47 (43 accepted Step 3 + 4 new).
```

The CPC examples do not call `print()`. For native GUI editor acceptance, run
one or both samples directly from `examples/devices/CPC/` on an empty/saved
scene, check for a successful script completion, visible `Script` > `CPC`
branches and GUI responsiveness. Running them calls `tn.Clear()`; save any
existing GUI work beforehand.

**Do not** run the frozen 500-million-ray benchmark. No new physics, RNG,
propagation, exporter, or interception implementation is added here. Record
Windows native evidence before calling Step 4A locally accepted; update
`PROJECT_STATUS.md` later without overriding the user's existing edits.
