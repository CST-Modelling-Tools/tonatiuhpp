# M3.2 Step 4B1 — bounded true-headless interception

## Scope

This increment makes the tracked `tn.FindInterception(surfaceUrl, rays)` name
available in **true-headless** scripts, without creating a QApplication,
QWidget, SceneTreeModel, screenshot, photon exporter, or alternate ray tracer.
The implementation uses the existing `SimulationSession::prepareHeadless`,
`start`, and `waitForFinished` path and observes front-side `RayTracerHit`
callbacks for exactly one canonical scene-instance URL. Power in watts is
`frontSideHitCount * result.powerPerRay`; the hit count is atomic to avoid
thread-order dependence. Script errors report the actual invocation location.

The GUI Script Editor and legacy `-i` retain the existing MainWindow-backed
`tn.FindInterception` and GUI-only `show=true` behavior. A narrow parity fix
makes both script entry points honor the existing opt-in GUI fixed seed via
`GuiTraceSeed::resolve`, matching ordinary GUI tracing/Flux Analysis seed
selection; the default remains automatic. This does not migrate the GUI to
the headless interception service or establish universal scientific parity.

`FindInterception` is intentionally not capped at a small ray count; the
**verification scripts** are capped to 2,048 rays to avoid heavy runs. The 19
tracked examples are unchanged and the eight large interception examples are
not executed by these tests. The host's ordinary `tn.traceScene`, benchmark,
and no-export behavior is not modified. The frozen 500-million-ray benchmark
must not be rerun without explicit approval.

## API behavior

- Requires a non-empty, unambiguous **TShapeKit** instance URL and a positive
  finite integer ray count in the platform `ulong`/JavaScript-safe range.
- Returns a finite, nonnegative number of watts from **front-side** hits,
  including physically recorded reflected and terminal interactions.
- Unknown/ambiguous targets, invalid arguments, scene preparation failure,
  simulation cancellation, and incomplete execution raise script errors.
- No photon-buffer accumulation, export, or rendered rays are requested.
- Uses `masterSeed=0` in true-headless until explicit fixed-seed scripting
  configuration is agreed; GUI scripts respect the GUI's selected fixed seed
  when enabled, otherwise retain automatic seed selection.
- Scene tracker updates are enabled for the current in-memory scene.
- The third `show` flag remains a GUI-specific capability; this increment
  forwards only the two-argument domain method in true-headless execution.

## Tests and acceptance

- The new scientific tests check error locations, nonexistent surfaces and
  deterministic repeated short interception on a constructed absorbing shape.
- The new CTest executes an included 2,048-ray script in a true-headless
  process and checks its success message.
- Run the same short script with both CLI modes, then in the GUI Script
  Editor after saving work (it calls `tn.Clear()`). Confirm the GUI stays
  responsive and the root is visible.
- For a *matched-input bounded parity check*, select fixed GUI seed `0` in
  the normal Ray Tracing dialog, set its sun grid to `200 x 200`, and keep
  ray count low. Re-run the GUI script and compare its intercepted power
  against true-headless seed-`0` execution of the identical script. Equality
  is expected on this no-tracker, vacuum, planar absorbing scene; this
  assertion has not yet been verified on the maintainer's Windows GUI.
- Other scenes, tracker/export policies and the optional `show` path remain
  separate parity gates. Do not interpret an unmatched automatic-seed run
  as a scientific equivalence failure.
- Re-run the scientific suite after the focused tests to check for accidental
  changes to ray propagation. Step 4B1 is local-acceptance pending until then.
