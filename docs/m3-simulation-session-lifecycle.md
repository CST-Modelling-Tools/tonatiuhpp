# M3.1 — Shared simulation session lifecycle

**State (2026-10-20):** Windows/MSVC Release build and targeted automated/nominal GUI acceptance **PASSED**; manual GUI Cancel-button smoke and hosted Windows/Ubuntu/macOS CI, review and merge are **PENDING**. M1 and M2 are merged into `master`.

## Scope and interface

`source/application/core/SimulationSession.{h,cpp}` provides the shared, Qt Widgets-free lifecycle for GUI Run, Flux Analysis, CLI `trace-scene`, benchmark, and true-headless `tn.traceScene`. The session delegates analytical scene preparation to the accepted `TracePreparation` adapters and delegates ray physics, chunk seeds and worker scheduling to the existing `RayTraceExecutor`/`RayTracer`. It owns the execution handle, executor and prepared context; optionally retains shared owned scene, plugin, exporter and buffer dependencies until its destruction. The GUI still supplies borrowed scene/tree/buffer references, which must remain valid until the session has joined its workers and the adapter has finalized export.

States: `Idle` → `Prepared` → `Running` → exactly one terminal state (`Completed`, `Cancelled`, or `Failed`). A completed/failed session may prepare a subsequent run; running sessions reject concurrent `start()` or `prepare*()`. `cancel()` requests cancellation through the same QFuture mechanism already exposed to Qt progress observers. The destructor requests cancellation and synchronously joins an active execution before releasing pinned resources. `waitForFinished()` is the sole accepted transition to a completed result.

**Progress accounting:** the executor reports counts of fully finished 10,000-ray chunks/rays. A chunk interrupted by an exporter failure is not counted; under concurrent failures these counts may conservatively undercount, never overclaim a partially processed chunk. An accepted `Completed` result requires all chunks and rays to have finished and no cancellation, worker exception or export failure. On `Cancelled` or `Failed`, `result().raysTraced` represents only fully completed chunks and is **not** a scientifically accepted total; consumers must check `state()`. Full scientific and benchmark outputs are emitted only for completed runs.

**GUI correction:** cancellation/failure no longer adds the full requested ray count to GUI or flux-analysis cumulative power denominators. GUI Run still closes the exporter and informs the user; incomplete recording receives zero photon power rather than a falsely completed normalization. Flux Analysis discards incomplete buffered analysis data instead of appending it to a valid flux map. The legacy exporter may already have flushed partial data before failure/cancellation; this package does not make exported files transactional. Detailed exporter failure/append policy belongs to M4.

## Compatibility deliberately retained

- Identical `SimulationConfig`, effective seeds, default GUI automatic/fixed seed and Flux Analysis increment-per-run seed policy.
- Headless tracker updating remains opt-in; benchmark and true-headless scripting still use their existing defaults.
- Shared prepared-scene math, 10,000-ray chunk seed derivation, worker pool, single ray-propagation loop and photon metadata are unchanged.
- File exporter choices, buffer sizes, output formats, receiver grid/signature schema and native diagnostic callbacks stay with the existing adapters. The scripting host itself remains separate pending M3.2.
- No frozen benchmark reference edits or 500-million-ray rerun.

## Regression tests and acceptance steps

Five `ScientificSessionLifecycle.*` tests characterize repeat runs and active-state rejection, cancellation, injected worker exceptions, injected exporter failures, and destruction joining active workers while retaining resource pins. Existing direct `RayTraceExecutor` tests remain in place as an independent scientific reference.

On Windows, validate a clean incremental Release build and the focused session tests before running the full scientific and headless equivalence suites:

```powershell
cmake --build build --config Release --target tonatiuhpp tonatiuhpp_scientific_equivalence_tests --parallel 8
ctest --test-dir build -C Release -R '^scientific[.]ScientificSessionLifecycle[.]' --output-on-failure
ctest --test-dir build -C Release -R '^scientific[.]' --output-on-failure
ctest --test-dir build -C Release -R '^headless[.](ray_history_(cylinder|specular|tracker)_equivalence|photon_export_(scientific|tracker)_equivalence|benchmark_repeatability|benchmark_heliostat_10k|run_script_cylinder_scene)$' --output-on-failure
```

After automated tests, execute a native GUI Run and Flux Analysis with `examples/benchmarks/cylinder.tnhpp` and fixed seed `123456789`, confirming a nonzero flux map. Exercise the GUI cancellation path separately on a sufficiently long run and confirm it does not claim the requested full ray count. Do not generate CI or a PR before these local results are reviewed.

**Observed Windows/MSVC Release results (2026-10-20):** Release application and scientific-test targets built, including `SimulationSession.cpp`. **33/33 scientific CTests passed** (five new `ScientificSessionLifecycle` cases); **12/12 selected headless CTests passed** across CLI, script, tracker, independent ordered ray history, photon export and bounded benchmarks. Native cylinder GUI Run used fixed seed `123456789` and completed in **58 s**; GUI Flux Analysis used the same seed and completed in **19 s**, with the flux map visually confirmed **nonzero**. These establish nominal local functional acceptance, not native GUI Cancel-button acceptance. No 500-million-ray rerun or throughput parity claim was made.

**Open limitations:** a borrowed GUI scene remains controlled by the existing synchronous GUI adapter rather than by `shared_ptr` ownership. Completely preventing nested-event-loop GUI document replacement while tracing needs a separate UI guard. The photon exporter retains its historical nontransactional flush semantics; scripted/GUI API unification remains M3.2/M3.3. The large frozen benchmark remains unnecessary for this bounded lifecycle change.
