# Tonatiuh++ — GUI/Core Separation and Scientific Equivalence Plan

**Role:** Canonical architectural roadmap for Tonatiuh++ milestones M0–M5.  
**Established objective:** One GUI-independent ray-tracing engine and one scripting implementation, reached through both GUI and CLI.  
**Planning baseline:** `master` at `de33955a` (PR #26 merged), 2026-10-11.  
**Execution tracker:** [`PROJECT_STATUS.md`](../PROJECT_STATUS.md), in its top **Architecture roadmap execution status** section.

> Read this plan for *what to build and how to accept it*. Read `PROJECT_STATUS.md` for *what has actually been implemented, tested, merged, or blocked*. Keep the plan stable; update status in the same PR as architectural work. Do not infer roadmap completion from older benchmark-checkpoint headings that reused “M2”, “M3”, “M4”, or “M5” for unrelated work.

## 1. Outcome and non-negotiable requirements

1. An interactive GUI trace, direct CLI trace, GUI-invoked script trace, and CLI-invoked script trace use the **same simulation preparation, ray propagation, scheduling, recording, export and numerical-result services**. A benchmark is a specialized client of the same execution service, not another physics implementation.
2. GUI and CLI execute `.tnhpps` through **one JavaScript host and one domain API**. Existing scripts that manipulate scenes, generate geometry, set parameters, trace rays, compute interception and read/write data should work from either interface without needing a `MainWindow`. Preserve established `tn` / `tonatiuh`, `NodeObject` and `DataObject` usage through compatible adapters whenever possible.
3. Same effective scene state, inputs and fixed master seed produce equivalent scientific results irrespective of invocation. Define and compare the **effective inputs**, including tracker-update policy, sun grid, air/material/plugin state, export selection and seed—not just the requested ray count.
4. The simulation core does **not** depend on Qt Widgets, SoQt, GUI dialogs, `MainWindow`, `SceneTreeModel` or an existing window. Qt Core, Qt Concurrent, Coin scene representation and QtQml for a separate scripting service may remain when needed.
5. No architectural cleanup silently changes validated optics, RNG streams, 10,000-ray chunk scheduling, frozen references or photon-file format. Differences require a specifically approved scientific change with new evidence.
6. Display-only actions (dialogs, interactive selection, drawing, screenshot) belong in GUI adapters. For scripts, provide a clearly documented capability policy: either a headless implementation for that action or a stable explicit unsupported-capability result. Do not confuse UI capability parity with a difference in **simulation** or **scripting** semantics.

**Not goals of this refactor:** replacing Qt/Coin, rewriting optical physics, changing receiver normalization “as cleanup”, redesigning the file formats, requiring a 500-million-ray run on every PR, or immediately splitting the installed GUI/CLI binaries.

## 2. Baseline code map (2026-10-11)

| Area | Existing code | Gap |
| --- | --- | --- |
| Shared numerical input | `source/application/core/SimulationConfig.h` | Contains rays, seed and sun grid, not a complete run/export request |
| Trace preparation | `source/application/core/TracePreparation.{h,cpp}` | `prepareGuiTrace` and `prepareHeadlessTrace` still perform separate preparation sequences |
| Ray executor | `source/application/core/RayTraceExecutor.{h,cpp}` | Shared 10,000-ray chunks and seeded workers exist, but callers own orchestration/lifecycle |
| Optical propagation | `source/kernel/run/RayTracer.cpp` | Two propagation loops selected by photon recording |
| GUI trace entry | `source/application/main/MainWindow.cpp` | Scene readiness, exporter, start/wait/cancel, diagnostics and results mix with UI code |
| Direct CLI | `source/application/headless/HeadlessCommandRunner.cpp` | Separately orchestrates scene load, exporter, preparation, execution and reporting |
| Benchmark | `source/application/benchmark/BenchmarkRunner.cpp` | Shares executor but independently orchestrates a benchmark run |
| GUI scripts | `source/application/script/ScriptWindow.cpp`, `NodeObject.cpp`, `DataObject.cpp` | Script engine directly exposes `MainWindow`; `NodeObject` keeps global GUI/engine pointers |
| Legacy command-line scripts | `source/application/main/main.cpp` | Non-`--headless` script execution creates `MainWindow` and initializes SoQt |
| True-headless scripts | `source/application/headless/HeadlessScriptHost.cpp` | Different, deliberately limited `tn` API; cannot run typical GUI scripts |
| Legacy script helper | `source/application/script/ScriptRayTracer.cpp` | `Trace()` currently returns 1 without tracing; audit before replacing/removing |
| Photon export | `source/kernel/photons/*`, `source/plugins/photons/PhotonsFile/*` | Buffer is shared; file plugin uses `QMessageBox` for failures and may delete matching output files |
| Build targets | `source/application/CMakeLists.txt`, `source/kernel/CMakeLists.txt` | Shared application services built into GUI executable; `TonatiuhKernel` links SoQt and Qt Widgets |

**Existing evidence:** Architectural M0/A0 acceptance covers designated GUI/headless cylinder, multi-reflection and tracker signatures, photon exports, ordered ray histories and frozen heliostat benchmark evidence. It is not universal cross-platform bitwise equivalence. PR #26 adds GUI Flux Analysis fixed-seed repeatability only; it does not prove full flux-analysis accuracy or universal GUI/CLI parity. Historical results and source-specific limits remain in `PROJECT_STATUS.md` and the related `docs/a0-*.md` documents.

**Existing script compatibility inventory:** 19 tracked `examples/**/*.tnhpps` files; common operations include `tn.getScene` (19), `new NodeObject` (13), `tn.Clear` (13), `tn.InsertScene` (13), `new DataObject` (10) and `tn.FindInterception` (8). These examples are regression inputs, not obsolete samples to rewrite.

## 3. Target dependency direction and contracts

    GUI widgets / editor / views            CLI argument and console adapter
                  \                                   /
                   \                                 /
                   Shared domain API and script host
                           |
                   SimulationSession
                      |        |
             Scene/document   SimulationRequest
                 services           |
                             TracePreparation
                                   |
                             RayTraceExecutor
                                   |
                        RayTracer + optional sinks
                                   |
                        Photon / flux / results

- **SimulationRequest**: validated physical inputs, effective seed and tracker/scene policy, sun grid, photon selection/export policy and optional diagnostics. Adapt command and dialog inputs at the edges; do not make the physics layer parse command-line switches or show dialogs.
- **SimulationSession**: prepares, executes, finishes/cancels and reports the run, with clear resource ownership, status, errors and progress. Provide one path from request to `SimulationResult`.
- **Scene/document service**: manipulate the in-memory scientific scene and resolve node names/paths without `MainWindow` or view model. Expose current scene state to scripts; do not require saving/reloading for ordinary script changes.
- **Scripting service**: a shared `QJSEngine` host, stable `tn` facade and compatible scene/data wrappers. GUI supplies display/log callbacks; CLI supplies terminal callbacks. Both use the same domain methods.
- **Optional photon/diagnostic sinks**: receive events from one optical propagation loop. The physical loop is identical when sinks are absent or enabled.
- **Output adapters**: filesystem/file plugins, flux grids, structured results and GUI visualization consume the same domain output.

**Ownership and safety contract:** A session retains loaded scene, instance tree, plugin objects, callbacks, photon buffers and exporter objects until all workers and finalization are complete. No dangling references to GUI widgets or a destroyed JavaScript engine are allowed. Preserve cancellation/error status and do not count incomplete runs as finished.

## 4. Milestones and ordered work packages

The packages below are ordered. A package can occupy one substantive PR (or a small number when required by review size); do not create a PR per checklist line. Each package must have an implementation, focused regression tests and a documented gate.

### M0 — Scientific safety net (COMPLETE for defined scope)

**Already accepted:** GUI/headless scientific equivalence for the specified fixtures, bounded recording/export comparisons, ordered histories and frozen 500-million-ray benchmark audit. Keep these references immutable and reuse these tests throughout M1–M5.

**M0 preservation gate:** Existing relevant scientific CTests pass; no reference, fixture or benchmark-value edits masquerade as refactoring. New tests are justified by a newly migrated boundary, not added merely for activity.

### M1 — Unified simulation inputs and preparation (PARTIAL; NEXT)

**M1.1 — Specify effective preparation inputs**
1. Map inputs and default policies for GUI Run, GUI Flux Analysis, direct headless trace, benchmark and `tn.traceScene`: seed resolution, ray count, sun grid, tracker updates, air, photon selection and scene ownership.
2. Record where each source comes from and which historical defaults differ. Do not silently change CLI `--update-trackers` opt-in or GUI automatic seed policy.
3. Add/extend small tests that compare the fully prepared scientific state for equivalent scenes, including tracker-enabled fixtures.

**M1.2 — Make one computational preparation implementation**
1. Introduce one canonical prepared-scene input/adapter contract. GUI may borrow an existing scene/instance tree; headless may own a built tree, but both must feed the same validated preparation operations.
2. Move common resolution of sun/air, analytical bounds, aperture sizing/cells, transform, seed, recording callbacks and normalization into shared helpers/a single implementation.
3. Keep scene synchronization/tracker updates explicit **before** common preparation; model the policy as an effective input.
4. Retain proper lifetime handling in `PreparedTraceContext`; do not leak ownership or repeat an expensive instance-tree rebuild needlessly.

**M1.3 — Migrate callers and verify**
1. Adapt `MainWindow`, CLI, benchmark, GUI flux and headless script tracing to the common preparation contract; avoid adding a third competing entry point.
2. Verify identical prepared-scene signatures and existing M0 regressions with fixed seeds; inspect legacy opt-in versus default tracker differences deliberately.
3. Delete obsolete duplicated preparation logic after all callers have migrated.

**M1 exit:** One shared computational preparation path; GUI/CLI/script/benchmark callers provide equivalent effective inputs where intended; all relevant M0 scientific tests pass. No physics, RNG, photon format or default CLI policy change.

### M2 — One ray-propagation algorithm (NOT STARTED)

**M2.1 — Preserve current observable semantics**
1. Compare both existing `RayTracer::operator()` branches for random draws, intersections, air termination, hit/history callbacks, photon path IDs and terminal records.
2. Lock down any distinctions that must survive as **recording output only**, using cylinder, specular, air-loss and tracker fixtures.

**M2.2 — Consolidate the algorithm**
1. Replace the recording/non-recording loops with a single launch/intersect/air/terminate loop.
2. Put photon creation, filtering and buffering behind optional lightweight observation/recording operations, not an alternate physical algorithm.
3. Preserve chunk-seed derivation, worker scheduling, interaction ordering within each ray and established export records. Keep the fast no-recording case efficient.
4. Remove the obsolete second loop rather than retaining dual modes indefinitely.

**M2 exit:** A single propagation algorithm serves recording and non-recording modes. Within-platform fixed-seed exact hit signatures and ordered ray histories match accepted references; photon content/metadata parity and representative performance smoke checks pass.

### M3 — One simulation session and one scripting engine (NOT STARTED)

**M3.1 — Extract a single session lifecycle**
1. Create a GUI-independent `SimulationSession` around the M1 preparation and M2 executor: `prepare`, `start`, `wait/finish`, `cancel`, `result` (API details may vary).
2. Own/pin the prepared scene, loaded plugins, worker futures, callbacks, photon buffers and exporter lifetime through finalization.
3. Define explicit states: not started, running, completed, cancelled, failed; include actual completed work and meaningful errors. Correct GUI cancellation/accounting without changing ray physics.
4. Move execution orchestration from `MainWindow`, `HeadlessCommandRunner`, script tracing and benchmark into this service. GUI progress widgets and CLI printing remain adapters.
5. Test successive runs, concurrent-start rejection, cancellation, worker/export failure, destruction/cleanup, progress and repeatability.

**M3.2 — Unify scripting execution**
1. Inventory public script operations and call signatures using the 19 tracked examples, GUI editor, legacy command-line `.tnhpps` launch and current `--headless run-script` tests.
2. Make one shared `QJSEngine`-based `ScriptHost` responsible for script evaluation, error/line reporting, exposed names, logging and file-path rules. GUI editor, legacy script CLI and true-headless CLI all call it.
3. Replace direct `MainWindow`-as-`tn` exposure with a GUI-independent `ScriptApi` backed by shared scene/document and simulation services.
4. Implement compatible `NodeObject` and `DataObject` adapters without static `MainWindow` / global engine pointers or `QMessageBox` in domain operations. Provide explicit session/engine ownership and failure handling.
5. Migrate script operations in functional groups: (a) scene load/create/clear/save and data IO; (b) node/shape/material/tracker creation and parameter edits; (c) sun/scene changes; (d) Run/trace, interception/flux and results; (e) exporter controls. Preserve existing names while consolidating implementations.
6. Audit `ScriptRayTracer::Trace()` and unused legacy scripting helpers. Remove or redirect stubs only after callers and compatibility tests are established; never expose false “success”.
7. Define presentation-only capabilities (screenshots, prompts, visual selection) separately. If a headless equivalent is absent, fail explicitly and consistently; do not introduce hidden GUI initialization.

**M3.3 — Script parity gates**
1. Run identical fixed-seed scripts in GUI and true-headless CLI for scene generation, save/reload, tracker changes, short repeated tracing, interception and `DataObject` input/output.
2. Compare scientific scene state/results and stable artifacts, not ephemeral console timestamps or thread-dependent export ordering.
3. Test relative paths from script directory (independent of current working directory), syntax/runtime errors with file and line, missing plugins/files, multiple consecutive scripts and cancellation.
4. Retain existing headless `tn.validateScene`, `tn.runBenchmark`, `tn.traceScene` and `tn.writeJson` compatibility during migration; scripts previously limited to one host should move toward the shared documented API.

**M3 exit:** No domain script API depends on `MainWindow` or `SceneTreeModel`; one script host is called from GUI and CLI; the supported existing scripts exercise the same domain functions and produce equivalent results in both modes. Any presentation-only exceptions are explicit and documented.

### M4 — Shared photon recording, export and analysis (PARTIAL FOUNDATION)

**M4.1 — Separate export backend from UI**
1. Separate photon exporter discovery/configuration, lifecycle and write errors from dialogs and `PhotonsWidget`. Remove `QMessageBox` from headless-executable export paths.
2. Move GUI-specific exporter selection to a GUI adapter; have CLI and script clients request the same exporter options through shared services.
3. Preserve file format, power normalization, surface selection, coordinate system, photon IDs, append policy and existing plugin behavior unless a change is expressly approved.
4. Make overwrites/deletions and unwritable-directory failure policies explicit. Validate output paths before destructive operations; report structured errors without requiring a GUI.

**M4.2 — Finish parity and script integration**
1. Extend beyond the limited opt-in CLI export diagnostic and its existing 200,000-ray restriction only after robust export lifetime/error handling.
2. Expose shared recording, selected-surface and export options to the scripting facade; implement equivalent interactive and scripted flux/interception analysis through common nonvisual computation services.
3. Compare canonical physical photon records (surface URL, side, coordinates), power and metadata under GUI, direct CLI and script execution. Treat global file ordering/ID assignment under parallel scheduling according to their documented semantics.
4. Test buffer flushes, multiple output files, append/repeat runs, exporter/plugin failure and interrupted runs.

**M4 exit:** GUI, CLI and scripts use the same recording/export/analysis implementation and offer the agreed nonvisual functionality. Existing file consumers and scientific exporters remain compatible.

### M5 — Build-enforced GUI/core separation and final parity (NOT STARTED)

**M5.1 — Create enforceable build boundaries**
1. Split shared application services into a separately compiled GUI-independent simulation library/target (for example `TonatiuhSimulation`), a shared scripting service and GUI/CLI adapters. Retain `TonatiuhKernel` as the physics foundation.
2. Remove SoQt and Qt Widgets dependencies from core targets, including indirect dependencies through factory `QIcon` metadata, `PhotonsFactory` widget creation, scene utilities and photon plugins. Move widget-facing parts into UI adapters or presentation-only companion interfaces.
3. Design plugin interface/ABI compatibility before changing exported virtual interfaces or plugin discovery. Preserve existing scene/plugin loading across supported platforms.
4. Add a clean, GUI-free headless build/link smoke test and a CMake/CI dependency guard that fails if forbidden GUI libraries or headers re-enter the core.

**M5.2 — End-to-end acceptance**
1. On Windows, Linux and macOS, build core plus a `QCoreApplication`-based headless executable without GUI libraries and run representative scenes and scripts.
2. For identical effective scenes, settings and fixed seeds, compare (a) GUI interactive versus direct CLI, (b) GUI script versus headless script, and (c) scripted versus non-scripted execution.
3. Verify recording, export, flux/interception results, error propagation, cancellation and repeatability; preserve documented within-platform exact comparisons and cross-platform scientific tolerances.
4. Ensure GUI startup still works, all tracked nonvisual sample scripts work in both modes, and release packaging/plugin discovery remain viable.
5. Update architecture/API documentation, `PROJECT_STATUS.md`, tests and release notes as appropriate. Remove dead bypass implementations.

**M5 exit:** One reusable simulation implementation and one scripting implementation are structurally enforced; the GUI is an optional client rather than a runtime requirement; required native scientific and cross-interface tests pass. Only then declare the separation complete.

## 5. Cross-interface acceptance matrix

Every applicable migration must test all relevant rows, not merely a direct CLI smoke case.

| Invocation | Scene source | Expected shared path |
| --- | --- | --- |
| GUI Ray Tracing dialog | In-memory GUI document | Adapter → SimulationSession → shared core |
| GUI Flux Analysis / interception | In-memory GUI document | Adapter → shared analysis/session |
| GUI Script editor (`.tnhpps`) | Current scene or script-created scene | Shared ScriptHost → scene services/session |
| Legacy command-line script without `--headless` | Script/scene | Shared ScriptHost, no distinct domain semantics |
| True-headless CLI `run-script` | Script/scene | Same Shared ScriptHost → scene services/session |
| CLI `trace-scene` | Loaded scene | Adapter → SimulationSession → shared core |
| CLI benchmark | Loaded scene | Benchmark analysis → same SimulationSession/core |

Comparison rules: same **effective** fixed seed, scene geometry/tracker state, physical settings and recording mode; compare exact events/histories within supported toolchain and canonical photon multiset/power; compare known numeric tolerances where exact equality is not a justified contract. GUI-only visualization is not a physics result.

## 6. Verification, efficiency and safety gates

**Per implementation PR:**
1. Start from current protected `master` on a descriptive `refactor/mN-...` or `test/mN-...` branch. Keep scope to one work package and delete the superseded path in the same package when safe.
2. Use the existing build directory and **incremental** Release builds. First run the narrowest relevant unit/scientific/headless/script CTests. Run broader local CTest before review when the change warrants it.
3. Never automatically rerun the 500M benchmark. Use the frozen A0 evidence and fast representative cases; an actual new 500M run requires a motivated change to accepted science and an explicit decision.
4. Compare semantic outputs; include appropriate local Windows evidence and allow GitHub Actions to independently check Ubuntu, macOS and Windows. Do not claim test success before it occurs.
5. Update the top roadmap tracker in `PROJECT_STATUS.md` **in the same PR**: package ID/status, changed files, fixed-seed scientific result, tests actually run, open risks, PR/commit links and next package. A draft or green CI run is **not** a merge; mark merge status only after merge.
6. Merge only after code review and all applicable acceptance criteria. Advance to the next work package when it is accepted; do not open unrelated characterization work by default.

**Security/robustness guardrails:** Scripts execute trusted local JavaScript and plugins execute native code; do not claim sandbox isolation. Avoid raw GUI-object exposure, dangling pointers, global per-process path/engine state and uncontrolled overwrites. Check null/invalid objects, exceptional cleanup, cancellation, filesystem errors and bounded allocations.

## 7. Status maintenance and new-chat handoff

The **single source of truth for the plan** is this document. The **single source of truth for progress** is the top *Architecture roadmap execution status* section of [`PROJECT_STATUS.md`](../PROJECT_STATUS.md). Historical dated sections in that file are evidence, not the current checklist.

At the beginning of any new chat/development session:
1. Read `AGENT.md`, `PROJECT_STATUS.md` **and this plan**; inspect current `master` and any active PR.
2. Identify the first unfinished package from the tracker. Check acceptance evidence before classifying it as complete.
3. Continue that package, using local incremental builds and targeted tests; do not propose an unrelated next task from the chronological status history.
4. When preparing each architectural PR, update `PROJECT_STATUS.md` with accurate *implemented / locally validated / CI validated / merged* distinctions and next step. Update the roadmap document only if the architectural scope or acceptance criteria are intentionally changed.
5. Preserve release-readiness work as a separate status stream. Do not label the historical benchmark “M2–M5” checkpoints as completion of the M2–M5 milestones in this plan.

**Next actionable package (baseline 2026-10-11): M1.1 and M1.2** — inventory effective preparation policies, implement the canonical shared preparation operations, and use existing scientific equivalence tests to guard the change. No M2 propagation or broad scripting rewrite in that first PR.

## 8. Explicit decisions to resolve only when needed

- How to transition the CLI's legacy default tracker policy (currently opt-in `--update-trackers`) without breaking existing computations.
- Whether every display-only legacy scripting operation needs an offscreen headless implementation, or is explicitly GUI-only.
- How to preserve plugin ABI while separating widget-facing factory interfaces.
- Whether changes to destructive/append photon-export behavior require a migration or versioned compatibility mode.

Record each decision in a focused PR and update this plan if it changes the agreed scope. Do not let an unresolved later decision block independent M1 or M2 structural work.
