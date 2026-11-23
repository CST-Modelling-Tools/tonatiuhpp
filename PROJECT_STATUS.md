# Project status

## Current continuation checkpoint — PR #60 merged (2026-11-23)

- **Authoritative baseline:** `master` contains merged PR #60, `test(naming): replace milestone identifiers with behavioral names` (https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/60). The verified merge commit is `b8934c0dc604ed7e4a27246ba61b94cf064af1d5`; GitHub records the merge at **2026-11-23 07:50:07 UTC**. The G0 + G1 + G1.1 + G2 + G2.1 migration is **complete and merged**, not local/uncommitted.
- **Hosted CI:** GitHub Actions CI run #287 (https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/45129823592), run ID `45129823592`, **completed successfully**. Its Windows (`windows-latest`), Linux (`ubuntu-24.04`) and macOS (`macos-latest`) build jobs each concluded `success`. This confirms the checks in that run; it is not a new release qualification.
- **Maintainer's final local Windows Release verification:** **207/207 CTests passed**, zero failed, **226.68 seconds** elapsed, including **4 native GUI tests**. Separate Python tooling: **44 run, 41 passed, 3 expected Windows directory-symlink privilege skips**, zero failures; **4/4 behavior-based naming safeguards passed**.
- **Scope and scientific invariants:** Test paths, fixtures, produced artifacts, scene IDs, CMake-private interfaces, test-only environment keys and test evidence schemas now use behavioral names. Scientific algorithms, RNG/photon behavior and immutable 500-million-ray reference data were not changed. **Do not rerun/regenerate the frozen 500-million-ray benchmark.** The historic full-run `m5_full_<10 hex>` directory name remains accepted by its unchanged auditor.
- **Next engineering checkpoint:** First perform a **read-only audit of remaining production C++ lambda expressions** under `source/`, as directed by `AGENT.md`. Prioritize one small, independently validated and behavior-preserving milestone; do not launch another blanket naming refactor. New modifications require a separate branch, complete testing and maintainer authorization before commit/push/PR/merge.
- **Reading this handoff:** Earlier G0/G1/G2 and candidate-status checkpoints below are chronological **pre-merge history**. Their statements that CI, commits or merge were pending are superseded by this checkpoint; do not resume those completed steps.

---

## Historical checkpoint — completed naming migration: Windows accepted (2026-11-23)

- **Branch and base:** `chore/test-name-cleanup` at the exact PR #59 merge
  `81b21b30c2f8a51639cef686155c0b21f845d93a`. The combined G0 + G1
  + G1.1 + G2 + G2.1 changes remain a **local, uncommitted migration**.
- **Final maintainer-reported Windows Release acceptance (2026-11-23):**
  **207/207 CTests passed, zero failed, 226.68 seconds elapsed**, including
  **four native GUI tests** (the `scientific` label includes 169 tests).
  The separate Python tooling suite ran **44 tests: 41 passed, three expected
  Windows directory-symlink privilege skips**, and zero failures. All **four
  behavioral test-naming safeguards passed**.
- **Completed naming scope:** G0 behavioral test-file/CTest names; G1 scene,
  generated result and test artifact names; G1.1 output-directory naming
  guards; G2 active GUI test, diagnostic, photon-file fault-injection,
  CMake-private, evidence-schema and synthetic helper identifiers; G2.1
  exact full-benchmark naming-contract allowance. The old architecture
  verifier is renamed to `verify_runtime_binaries.py`.
- **Scientific preservation:** No ray-tracing algorithm or frozen reference
  changes are part of this migration. The original 500-million-ray auditor
  continues to accept the historical `m5_full_<10 hex>` full-run directory
  format; routine heliostat output directories use descriptive names. The
  500-million-ray benchmark was **not rerun or regenerated**.
- **Remaining gates:** Review and stage the combined change, then obtain
  explicit authorization before committing, pushing, or opening a PR.
  Require hosted **Windows, Linux and macOS CI** before merge. This local
  validation is **not** evidence that hosted CI, PR review, or merge passed.
  Keep this checkpoint and the accompanying testing/migration documentation
  accurate after each subsequent action.

---

## Historical checkpoint - G2.1 frozen benchmark naming-guard fix

- **Branch/baseline:** Unstaged `chore/test-name-cleanup` on PR #59 merge
  `81b21b30c2f8a51639cef686155c0b21f845d93a`; G0 + G1 + G1.1 + G2
  application completed, with no commit, push, PR or merge claimed.
- **Latest maintainer-reported G2 Python check (2026-11-23):** 44 tests ran,
  **40 passed, one failed and three skipped** for unavailable Windows
  directory-symlink privileges. All three other naming guards passed.
  The sole failure was a false positive in the test-owned content-name guard:
  `tests/cmake/run_heliostat_benchmark.cmake` intentionally generates
  `m5_full_<10 hex>` for MODE=full, as required by the unchanged frozen
  500-million-ray auditor. The G1.1 allowlist covered the auditor and its
  synthetic test but overlooked the corresponding active producer.
- **G2.1 correction:** Limit this additional allowance to the exact CMake
  runner, while checking that its MODE=full branch still generates the
  historic audit name, its routine branch uses descriptive `heliostat_*`
  names, and no additional `m5_full_` uses appear in the runner. The CMake
  runner, original auditor, synthetic test, frozen benchmark and scientific
  code are NOT changed by G2.1. No 500-million-ray trace is run.
- **Acceptance pending:** Rerun all 44 Python tooling checks (expected 41
  passing, three expected privilege skips), then Windows Release configure,
  build and full 207 opt-in CTests including four GUI tests. The earlier
  207/207 native Windows pass preceded G2 and does not certify these changes.
  Require Windows/Linux/macOS hosted CI before merge. No commit, push or
  merge is authorized or claimed.

---

## Historical checkpoint — G2 active testing interface cleanup (validation pending)

- **Verified maintainer evidence:** After G1.1, the separate Windows Python
  tooling suite passed: **43 executed, 40 passed, 3 skipped** due to missing
  Windows directory-symlink privilege. All three test naming guards passed.
  The latest full native Windows Release run reported **207/207 CTests**
  (227.31 seconds, including four real GUI CTests) before G1.1's two scratch
  directory renames. That result cannot certify the new G2 changes.
- **G2 candidate:** Coordinate behavior-based names for native GUI test hooks,
  CMake-private dependency boundary identifiers, test-only photon File failure
  injection, native trace/ray-history capture environment variables, and
  versioned *test-generated* acceptance evidence schemas. Retain the frozen
  500-million-ray benchmark auditor and its historical `m5_full_<10 hex>`
  directory format unchanged; do not run/regenerate that scientific reference.
  Non-full heliostat test output folders use descriptive names, while the
  full-run directory pattern stays byte-for-byte identical.
- **Scope and validation:** This is a mechanical testing-infrastructure
  interface rename, not a physics/refactor/release change. Documented former
  hook keys/schemas are intentionally replaced rather than silently aliased;
  update external diagnostic scripts that invoke these opt-in test hooks.
  Rerun configure + Windows Release build, **207/207** native CTests with
  four display-backed GUI tests, and **44** tooling tests (expect 41 passed,
  3 known symlink skips). Then review full staged Git renames and require
  hosted Windows/Linux/macOS CI before merge. No commit, push, PR or merge
  is authorized or claimed by this updater.

---

## Historical checkpoint — G1.1 naming-guard correction (revalidation pending)

- **Branch/baseline:** Uncommitted `chore/test-name-cleanup` at PR #59 merge
  `81b21b30c2f8a51639cef686155c0b21f845d93a`, carrying G0 + G1.
- **Maintainer-reported G0+G1 Windows Release run (2026-11-23):**
  **207/207 CTests passed**, 0 failures, **228.64 seconds**, including
  4/4 opt-in real native GUI tests. Separate Python tooling ran 43 tests:
  **39 passed, 1 failed, 3 skipped** (Windows directory-symlink privileges).
  The only failure was the new G1 test-owned naming guard; it found two
  unrenamed CTest output leaves in their callers and cleanup guards,
  plus the established `m5_full_*` frozen-benchmark auditor directory
  contract in the auditor and its synthetic unit test. This run does NOT
  establish G1 passing Python acceptance.
- **G1.1 correction:** Rename the two CTest-owned disposable output leaves
  together with their exact cleanup-path safety checks; preserve the
  `m5_full_<10 hex>` audit contract and restrict the Python naming guard's
  allowance to the precise two audit files and original accepted strings.
  The auditor, synthetic benchmark test and frozen references are unchanged.
  No production code, scene geometry, ray/RNG algorithm, benchmark or
  scientific reference is edited or executed by the updater.
- **Next gate:** After G1.1 application, rerun Windows Release configuration,
  build, 207 full opt-in CTests, and all 43 tooling tests (expect 40 passed,
  3 expected skips). Review rename-aware staged diff; require hosted Windows,
  Ubuntu and macOS CI before merge. No commit, push, PR, merge or release
  has been performed by this script. The historical sections below are
  superseded; do not treat G0/G1 candidate text as current acceptance.

---

## Historical checkpoint — G1 in-content test naming (NOT YET VALIDATED)

- **Baseline:** G0 is Windows Release validated by the maintainer on
  2026-11-22: 207/207 CTests (212.58 seconds), Python tooling 39 passed /
  3 expected Windows symlink-privilege skips out of 42. G0 is still
  uncommitted and unmerged on `chore/test-name-cleanup` at PR #59 HEAD
  `81b21b30c2f8a51639cef686155c0b21f845d93a`.
- **G1 scope:** Rename test-owned result CSV/TSV filenames, scene-node
  identifiers, and diagnostic labels together with all in-repository
  producers/consumers. Add a Python regression guard against restoring
  milestone-named result files and scene identifiers. Registered CTest names,
  scene geometry, RNG, photon data, scientific algorithms, reference bytes,
  and non-testing production C++ stay unchanged.
- **Deliberate exceptions:** Versioned scientific test-evidence schemas
  (`m3.3-script-parity-v1`, etc.) and test-only process environment keys
  (`TONATIUHPP_M5_GUI_NATIVE_*`, `TONATIUHPP_M3_TEST_FILE_DATA_FAULT`, etc.)
  are interface identifiers with older producers/consumers. They are not
  renamed by a test-fixture cleanup without an explicit compatibility plan.
  Historical records and frozen 500M reference artifacts are immutable.
- **Status:** G1 application is a candidate and has NOT been built or tested
  on Windows/Linux/macOS, committed, pushed, or merged. After applying G1,
  rerun the full Windows Release suite (expect 207 opt-in CTests) and Python
  tooling (expect 43 tests: 40 passes / 3 expected skips), then obtain hosted
  three-OS CI before any merge. Record the actual results here.

---

## Historical checkpoint — Test-name cleanup G0 (WINDOWS VALIDATED; CI PENDING)

- **Verified baseline:** PR #59, `fix(gui): avoid invalid transform downcast in SetValue`,
  merged to `master` on 2026-11-22 at `81b21b30c2f8a51639cef686155c0b21f845d93a`.
  Earlier architecture PRs #56–#58 are also merged. The old Batch F pending
  section below is historical; do not follow old status as an active instruction.
- **Why:** Registered CTests already use behavior-based names, but 27 tracked
  test source/driver/template filenames still encode obsolete m3/m4/m5
  milestones. See `docs/test-naming-migration.md` for every rename and scope.
- **G0 scope:** Test paths, callers and test markers, local helper names,
  two Python naming-guard tests, and these handoff/testing docs. Absolutely no
  production C++/physics/RNG/photon/reference updates. Preserve JSON/CSV schemas,
  environment variables, scientific fixture values and output safety guards.
- **State:** Local Windows Release **VALIDATED by maintainer** on 2026-11-22:
  207/207 CTests passed (0 failures, 212.58 seconds), including the four
  opt-in native GUI CTests. Separately, 42 Python tooling tests ran:
  **39 passed and 3 skipped** due to unavailable Windows directory-symlink
  privileges. Both new naming-guard tests passed. These are local Windows
  results, not hosted three-platform CI, a PR, a merge or release acceptance.
  The frozen 500-million-ray reference MUST NOT run.
- **Next steps:** G0's filename/registered-test migration is Windows
  validated, but **not a complete in-content test-name normalization**.
  Review residual test-owned scene names, result filenames, schema IDs,
  test-only environment variables and diagnostic labels; change these
  only with all producer/consumer assertions and compatibility needs
  explicitly audited. Do not rewrite immutable references or historical
  records. Keep changes on `chore/test-name-cleanup`, review the staged
  diff and run hosted Windows/Linux/macOS CI before any merge. No C++
  lambda expressions or frozen-reference execution.

---

Last updated: 2026-11-22. PR #59 merged; G0 naming migration Windows validated; CI/PR/merge pending.

## Historical checkpoint — Batch F Windows validated (merged as PR #59)

**Repository and instructions:** `CST-Modelling-Tools/tonatiuhpp` at
<https://github.com/CST-Modelling-Tools/tonatiuhpp>; local Windows checkout
`C:\OpenSource\tonatiuhpp`. Read `AGENT.md` before changing source;
`docs/testing.md` defines test inventory and validation. This top section is
current; all dated handoffs below the historical divider are superseded.
Source, `git status`, and CI evidence take precedence over chat summaries.

**Verified upstream baseline:** PR #56 (architecture A+B+C) merged at
`9f65fe736bfcd2fc4aaddcdc8f9651e850a45232`; PR #57 (scene ownership
and Undo/Redo stabilization D+D2) merged at
`987e9e4ebd59c80750ba96e411b0827b8006d81c`; PR #58 (automated real
Fresnel GUI Cut/Copy/Paste/Translate/Undo/Redo regression E) merged at
`830fe2c6d4b61daa37022936bf91a2dea2361a33` on 2026-11-22.
**Do not resume candidate A-E instructions in historical sections.**

**Most recent maintainer-reported Windows Release evidence (Batch E):**
207/207 CTests passed with `TONATIUHPP_ENABLE_NATIVE_GUI_ACCEPTANCE=ON`,
including 4/4 real native GUI modes and `gui.native_scene_undo_redo`.
Separate Python tooling: 40 tests run, 37 passed, 3 skipped because this
Windows runner cannot create directory symlinks. These are local results
reported before the PR #58 merge; do not extrapolate to new work or claim
new three-platform CI without checking actual CI runs.

**Batch F maintainer-reported Windows Release acceptance (2026-11-22):**
The Batch F change was built in Windows Release with `BUILD_TESTING=ON`
and `TONATIUHPP_ENABLE_NATIVE_GUI_ACCEPTANCE=ON`. The complete CTest
suite passed: **207/207**, 0 failures, **209.95 seconds**; this includes
**4/4** real native GUI CTests and `gui.native_scene_undo_redo`.
The separate Python tooling run completed **40 tests**, with **37 passes**
and **3 skips** caused by unavailable Windows directory-symlink privileges.
The maintainer's validation script reported `Batch F Windows validation PASSED`.
These results establish local Windows acceptance, **not** Linux/macOS CI,
installation acceptance, merge or release. The validation commands did not
execute the frozen 500-million-ray benchmark.

**Batch F change (WINDOWS VALIDATED; COMMIT/PR/MERGE PENDING):**
`MainWindow::SetValue` in `source/application/main/MainWindow.cpp` casts
`TSeparatorKit::getPart("transform", true)` to `SoTransform*` even though
`TSeparatorKit` declares its transform catalog part as `TTransform`
(derived from `SoTransformation`, not `SoTransform`). It later converts
back to `SoNode*`. Remove this unrelated type downcast, retain the actual
`SoNode*` returned by `getPart`, and return with an `Abort` diagnostic if
no transform part is available. Preserve existing node/field resolution,
Qt Undo command creation and the ability to edit GUI manipulator parts.
Do not modify the scientific kernel, physics, RNG, reference data or exports.

**Existing focused automated regression:**
`tests/integration/gui/NativeSceneUndoRegression.cpp` already calls
`MainWindow::SetValue("//Node/Node_01_1", "translation", "20 0 0")` on a
real `TSeparatorKit` created from `examples/facilities/Fresnel/Fresnel.tnhpps`.
It reads the actual `TTransform`, checks translation/scene-tree/Coin3D
identity, and runs three complete seven-command Undo/Redo cycles. Leave
those assertions unchanged. Do not add another test solely for this cast.

**Batch F reproduction and validation commands:** From clean PR #58 `master`, create
`fix/gui-transform-field-cast`. Download
`tonatiuhpp_gui_transform_field_F_apply.py`, execute `python <script> --root . --check`
and then `python <script> --root . --apply`. Review `git diff --check`, the
source diff and documentation. Then run:

```powershell
cmake -S source -B build -DBUILD_TESTING=ON -DTONATIUHPP_ENABLE_NATIVE_GUI_ACCEPTANCE=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release -R '^gui[.]native_scene_undo_redo$' --output-on-failure -V
ctest --test-dir build -C Release --output-on-failure
python -m unittest discover -s tests/tooling -p "test_*.py" -v
```

For that Windows option set, expect 207 CTests; the default non-GUI
Windows suite is 203. Linux/macOS default 202 and opt-in 206 registrations
are expected from CMake, but must be confirmed on the actual CI runners.
Check all native Windows/Linux/macOS CI jobs before any merge. Never run,
regenerate or substitute the frozen 500-million-ray scientific reference.
Do not claim speedups without controlled measurements. No production C++
`lambda` expressions. No commit, push, CI launch, merge or release without
maintainer signoff and completed validation.

**Known deferred, separately reviewable issue:** Other legacy `SoTransform*`
casts occur in GUI manipulation/menu code (for example `ShowMenu`). Their
semantics are not audited by Batch F; avoid broad blind substitutions.
Review them as a separate, independently tested follow-up if appropriate.

**Next handoff update:** After maintainer-authorized commit and PR, verify
all hosted Windows/Linux/macOS CI jobs and record the PR, merge SHA,
platform-specific CTest and native GUI evidence. Do not claim Batch F
merged until verified. Then consider the separate manipulator-cast audit
(Batch G) with focused tests and no broad blind replacements.
A future chat should first read the top of `PROJECT_STATUS.md`, `AGENT.md`
and `docs/testing.md`, then inspect actual `git status -sb`, `git log -1`
and the working diff. Do not assume a candidate script was applied.

---

## Historical records (superseded; kept for provenance)

## Historical handoff — Batch D manual Undo crash; D2 repair candidate NOT validated

**2026-11-21 Windows evidence:** Batch D built successfully and passed all **206/206** CTests (203 standard and 3 opt-in native GUI modes) plus Python tooling **34 passed / 3 skipped** (Windows directory-symlink privileges). However, a real GUI scene-editing check on `examples/facilities/Fresnel/Fresnel.tnhpps` **crashed on Undo after Cut, Insert/Rename, Paste, Copy/Paste, Translate and multiple Undo steps**. **Batch D is NOT accepted, committed or merged.** The existing GUI acceptance tests do not exercise this undo chain.

**D2 local corrective candidate:** `tonatiuhpp_scene_tree_undo_repair_D2_apply.py` applies only on the exact uncommitted D branch. Pin copied Coin3D node identity for stable Undo/Redo, retain parent Coin node instead of a scene-tree `InstanceNode*` that is recreated on parent Undo/Redo, explicitly protect field-edit/rename undo references, prevent wrong-row deletion, and guard stale GUI selections. No physical calculations, photon/RNG logic, scene serialization or plugin ABI are changed. This is a source-level hypothesis for the crash; **the exact crashing stack frame has not been captured**. D2 is uncompiled and requires the full Windows Release + 206 CTests + Python tooling, **and especially the complete manual failing Undo sequence followed by Redo and repetition**, before any commit. Stop and collect a debugger call stack if it still crashes. Do not run the frozen 500M-ray benchmark.

---

## Historical handoff — scene-tree ownership batch (original D candidate; NOT accepted)

**Baseline:** `master` at merge commit `9f65fe736bfcd2fc4aaddcdc8f9651e850a45232`, PR #56. Batches A+B+C are merged. The maintainer reported 203/203 Windows CTests, 34 passed/3 skipped Python tooling tests, 3/3 native GUI acceptance tests and a successful manual GUI progress/cancellation check for that merged batch; the feature PR CI workflow succeeded. This is not a new cross-platform or production-install validation of the ownership changes below.

**New local patch:** `tonatiuhpp_scene_tree_ownership_D_apply.py` runs only on a clean `refactor/scene-tree-ownership` branch at the exact merge commit. It encapsulates `InstanceNode` child ownership, provides read-only child access and explicit detach, uses temporary RAII during recursive deletion and headless tree construction, removes leaked `InstanceNode` objects on Cut, fixes CmdPaste undo's post-Cut invalid child access, and guards failed Cut preconditions before Qt removal notifications. Borrowed Coin3D nodes, external scene pointers, trace scheduling, random seeds, photon data, scene serialization and plugin-factory interfaces are unchanged. **Compatibility gate:** the exported `InstanceNode` class no longer exposes mutable `children` or `setParent` as public source APIs. Existing third-party source users need adaptation; verify external binary ABI before any merge rather than assuming it is stable.

**Status:** prepared but NOT applied, compiled, Windows-validated, cross-platform-validated or merged as written. Before acceptance, run the Windows Release build, full standard 203 CTests (unless test inventory deliberately changes), Python tooling tests, opt-in three real native GUI scientific tests, and manually exercise repeated Cut/Delete/Paste and Undo/Redo on both ordinary and shared scene nodes. Verify model row/index consistency, no dangling pointers or leaked detached nodes, empty-scene Flux Analysis, and scene loading in GUI and CLI. Do not run or regenerate the frozen 500-million-ray benchmark. No commit, push, merge or hosted CI without explicit maintainer approval and local validation. Update this status once evidence is available.

---

## Historical handoff — architecture implementation (A+B validated; C candidate superseded)

**Repository:** `CST-Modelling-Tools/tonatiuhpp`. On 2026-11-21 the reviewed `master`
HEAD was `7184367d5035546d8fa8972b1415f9d56680516e` (merged
[PR #55](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/55), test-suite
housekeeping). E1 scientific correctness ([PR #53](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/53)
and [PR #54](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/54)) and
housekeeping are complete, merged and cross-platform CI validated. **Do not
resume old pending-E1 or pending-test-housekeeping instructions below.**

**Maintainer authorization:** Improve architecture and implementation, quickly
but safely, with **NO C++ lambda expressions**. Keep scientific correctness,
throughput and maintainability as first principles. Do not modify `master`,
commit, push, start hosted CI, merge or release without maintainer signoff and
successful local Windows Release validation. The frozen 500-million-ray
scientific reference must **never be rerun or regenerated** as part of this work.
Past M9 performance comparisons were inconclusive; do not claim speedups or
regressions without controlled measurements.

### Local work prepared for batch A

A standalone, local-only application script is provided separately in the
conversation as `tonatiuhpp_architecture_hardening_A_apply.py`. It requires a
**clean `chore/architecture-hardening` branch at exactly** the master commit
above. Its `--check` mode validates expected Git blobs (including Windows
CRLF/LF conversion) and source edit anchors before touching any files; its
`--apply` mode makes nine uncommitted source/documentation edits. It does not
build, commit, push, trigger CI, make backups or run scientific references.

Batch A edits (only after the script is applied):

1. `InstanceNode.h`: explicitly delete copy construction and copy assignment
   because the node deletes its raw-pointer children.
2. `RayTracer.cpp`: guard photon-buffer mutex with `QMutexLocker` so an exporter
   exception cannot leave it locked; preserve existing mutual exclusion.
3. `source/plugins/CMakeLists.txt`: fail configuration on **any platform** if an
   official plugin target listed in `TONATIUHPP_PLUGIN_TARGETS` is absent.
4. `FluxAnalysis.cpp`: cap initial vector reservation to `min(rays, 1,000,000)`
   instead of ~33.55 million photons, without reducing permitted capacity or
   changing accumulation or RNG semantics.
5. `PhotonsFile` CMake/source: compile the environment-based write/flush
   injection seam only with `BUILD_TESTING=ON`; preserve test fault injection.
6. Root `source/CMakeLists.txt`: declare CMake 3.28, already the effective
   minimum of component build files.
7. `MainWindow.cpp`: reuse the existing `TonatiuhCore::setProjectSearchPaths`
   implementation and remove its duplicate helper without changing path order.
8. This status/handoff is updated.

**Batch A verified local 2026-11-21:** Maintainer applied all nine edits
on `chore/architecture-hardening` and reported a successful Windows Release
run: **203/203 standard CTests passed** (202.38 s) and Python tooling
**37 tests: 34 passed, 3 skipped** because directory symlinks are unavailable
on this Windows runner. No test failures. This verifies the tested build
but is **not** yet a cross-platform CI or installed-production acceptance.
Additional interactive GUI/flux and BUILD_TESTING=OFF runtime checks remain
for the relevant changes. No source modifications have been committed, pushed
or merged as part of these architecture batches. Check live Git status.


### Current checkpoint: B locally tested; bounded C candidate not yet validated

The maintainer reported separate Windows Release automated validation
after Batch B: 203/203 ordinary CTests passed (205.14 seconds) and
Python tooling ran 37 tests (34 passed, 3 skipped due to unavailable
directory-symlink privileges). These results do not establish actual
GUI/CLI plugin inventory parity or installed production acceptance.

A new local-only Batch C script targets the exact uncommitted A+B tree.
It adds an executor destructor that cancels/joins abandoned workers,
joins any already-submitted workers before a partial launch failure
may release executor resources, and restores visible Qt Flux Analysis
progress and cancellation while preserving silent run behavior. It
does not alter chunk scheduling, RNG, photon numbers, scientific output
formats, or the 500-million-ray reference. The patch is NOT built,
tested, committed, pushed or cross-platform accepted. Before accepting
it, run Windows Release build, all 203 CTests, Python tooling, visible
GUI progress/cancel and scripted/silent flux acceptance. Explicitly
verify clean cancellation, worker failure cleanup, append rollback,
and no GUI scene-edit use-after-free. Real GUI and installed-runtime
acceptance and eventually three-platform CI remain separate gates.

InstanceNode child ownership and Coin3D reference conversions are
deferred to a dedicated audited gate, not bundled into this GUI fix.

### Immediate validation gate for batch A

From the clean, synchronized master commit, make `chore/architecture-hardening`.
Apply script `--check`, then `--apply` and inspect `git diff --check` and the
source diff. Build all official plugin targets and the application in Windows
Release; expect **203** standard CTests, run the complete suite and separately
run Python tooling tests. Check test-enabled `PhotonsFile` fault injection and
`BUILD_TESTING=OFF` production installed-runtime behavior, especially the
required-plugin manifest. Test GUI flux analysis with modest ray counts, verify
photon/flux outputs and that startup memory no longer reserves ~1.5 GiB.
Check frozen-reference *integrity* only if desired, never run the 500M scene.
Record exact outputs, failures and any accepted compromises here before commit.
Only then consider a single consolidated commit and a single three-platform CI
PR **with maintainer approval**. Do not run a full performance comparison in an
uncontrolled environment just to validate this architecture patch.

### Additional approved improvements - NOT implemented in batch A

**Batch B, factory/plugin architecture (WINDOWS AUTOMATED TESTS PASSED):** Unify
GUI `PluginManager` and headless `CorePluginRegistry` scientific type catalogs
and plugin search roots. `MainWindow.cpp` currently loads only the first search
root, while headless examines all roots. Give GUI built-in factories explicit
ownership; preserve loaded `QPluginLoader` lifetimes and every official
plugin. Preserve GUI ordering, plugin names/ABI, scene deserialization, UI-only
`ComponentFactory` and `PhotonsFactory`. Later consider moving icon/presentation
metadata out of scientific `TFactory`/`QIcon` only with an intentional ABI plan.
Add GUI/CLI plugin-inventory/scene-loading/installed-artifact comparisons.

**Batch B implementation:** The separate local script
`tonatiuhpp_architecture_factory_B_apply.py` applies on the verified
uncommitted Batch A working tree, with strict file, branch and baseline
checks. It adds `ScientificFactoryCatalog` once to TonatiuhSimulation,
replaces both GUI and CLI duplicated built-in factory lists with it,
uses a deterministic shared plugin file scanner for all configured roots,
retains GUI plugin loaders and owns GUI built-in factory objects, and
defers plugin-manager destruction until after GUI scene teardown.
The script makes NO commit, hosted CI run or benchmark. Before accepting
it, validate Windows Release build, 203 CTests, Python tooling and
actual GUI+CLI plugin discovery/scene loading; obtain three-platform CI
after maintainer approval. GUI-only component/exporter registration,
factory names, scene file format and plugin ABI are deliberately retained.

**Batch C, ownership/execution safety:** Encapsulate `InstanceNode::children`
and migrate owning pointers toward `std::unique_ptr` only after auditing
`SceneTreeModel`, `SceneInstanceBuilder`, append/export-surface pointers and
Coin3D refs. Keep external non-owning scene pointers explicit. Tighten the
`RayTraceExecutor` start/join API and replace type-erased `shared_ptr<void>`
resource pins where useful. Verify cancellation, worker failures and cleanup.

**Batch D, GUI and build boundaries:** Repair actual progress/cancellation for
`FluxAnalysis::run` (it currently builds a `QFutureWatcher` but synchronously
waits, with display commented out); preserve silent/script behavior, seeds,
output and append semantics. Narrow `MainWindow` incrementally by responsibility
and migrate touched old Qt `SIGNAL`/`SLOT` connections to typed connects, with
no lambdas. Improve explicit per-document/script path context without breaking
`project:` aliases. Separate GUI presentation metadata from scientific
factories only after the registry ABI is settled.

**Batch E, post-profile internals:** Separate `BenchmarkRunner` parsing,
numerical accumulation and serialization without changing binary/reference
formats. Consider cached `PhotonsFile` surface-ID lookup only after profiling
and exact serialized-output comparison. Extract oversized CMake staging and
runtime packaging into named modules selectively; preserve all three platforms.
Do not claim performance gains or change RNG, roulette, ray scheduling,
geometry, photon numbering, flux normalization or atmospheric DNI convention.

**Scope strategy:** Coherent implementation/validation batches, not microcommits
or speculative global rewrites. For each batch preserve scientific and GUI/CLI
behavior, 203 Windows / 202 Linux/macOS test inventories unless deliberately
reviewed, and opt-in native GUI plus installed-runtime acceptance as relevant.
The eventual cross-platform PR is a gate after local validation, not permission
to merge automatically. Maintain this active handoff as each batch advances.

## Historical handoff - E1 and test-suite housekeeping (superseded)

- **Merged baseline:** E1 Stage 1 ([PR #53](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/53)) and E1 Stage 2 ([PR #54](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/54)) are on `master` at `b9065855ca856c6cfc7dd75745ff584d6ad3d4a1`. The latter passed Windows, Ubuntu and macOS PR CI; Windows local Release validated 203/203 standard CTests and frozen-reference integrity (the 500M-ray trace was **not** rerun).
- **Approved maintenance scope:** Prepare one `chore/test-suite-housekeeping` change on a dedicated branch. Rename milestone-encoded CTest and GoogleTest identifiers to behavior-oriented names; distinguish repeatability from equivalence; remove the unused stand-alone SoQt cube probe; eliminate only the obsolete callback-order assertion while preserving all ray-history, energy and export regressions. See `docs/testing.md`.
- **Validation gate:** The maintenance change is uncommitted and **not accepted** until a local Windows Release build and full standard CTest suite pass (203 expected), followed by a single cross-platform CI cycle if the maintainer authorizes push/PR. No production physics, reference bytes, fixture schemas, installer or release changes. No commit, push, merge or release without the maintainer's explicit approval and validated local results.
- **Performance caution:** Prior M9 timing results remain inconclusive; no new benchmark or performance conclusion is implied by this housekeeping task. Do not rerun or regenerate the frozen 500M-ray reference.

## Historical handoff — project snapshot of 2026-11-16

## Active handoff — next milestone decision (verified 2026-11-16)

**Repository and Git checkpoint:** `CST-Modelling-Tools/tonatiuhpp`, default branch `master`. The verified remote `master` HEAD after [PR #51](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/51) is `d7d47280a300a60cf1f8c138f20eafe5f5e22e4d` (merged 2026-11-16). The M9 closeout was authored on `docs/m9-performance-closeout` as local commit `9751c7e12a94b19b41199869b2cf552e8d291dc7` and is now merged; the local Windows checkout may still be on that branch and **has not been independently checked after the merge**. Verify local Git state before further changes, and do not assume `master` is synchronized.

**Completed workstreams:** Installed-runtime and packaging acceptance Phase E was merged as [PR #46](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/46), `ccf3261a03411eab4decf61d76a8e467a02c1545`; the production C++ lambda-elimination milestones M1–M8 were merged as [PR #50](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/50), `0e5bde8614b6b547990b8561b53b5f81872606b0`. M9 is **complete as a documented investigation**, not a passing performance-acceptance gate. Older Phase D/E pending-work instructions below are dated history, **not active tasks**.

**M9 decision:** Exact scientific equality **PASS** for the tested Windows Release 5,000,000-ray fixed-seed heliostat workload (identical scene bytes, matched Release toolchain, 16 workers). Paired candidate time changes in two independent 6-block ABBA experiments were **+0.418%** and **+1.129%**, but both failed stability limits; **performance equivalence within +/-2% is NOT established, and no reproducible regression above 2% was demonstrated**. The Windows counter collector missed samples under full CPU load (34/34 monitored runs sparse; reduced-counter probe 4/4 sparse), while idle sampling was regular. The cause of timing drift is unproven. **Do not repeat benchmarks on the same uncontrolled setup, relax limits, discard unstable blocks, edit the scientific kernel, or rerun/regenerate the frozen 500-million-ray reference.** Full results, hashes, provenance, artifact paths and limitations are in [docs/performance-m9-acceptance.md](docs/performance-m9-acceptance.md).

**Current next action:** No new implementation milestone or product release has been approved. First conduct a **scoped project-priority and release-readiness review** based on the latest `master` and current project requirements. In that review, separate accepted relocated GUI/CLI and copied Qt IFW *payload* tests from **unproven actual Qt IFW installer/update/uninstall transactions, signing/notarization, or end-user installation**. Choose a bounded, useful next milestone with maintainer approval before editing production code. Do not reopen M1–M8 or superseded Phase E plans merely because older text remains below.

### Instructions for the next chat / maintainer handoff

1. Read the top of this file and `AGENT.md`; consult `docs/testing.md`, `docs/scientific-validation.md`, `docs/performance-m9-acceptance.md` and relevant CI/release docs for the chosen scope. Treat the subsequent 2026-11-12 handoff as historical context.
2. On the Windows workstation (`C:\OpenSource\tonatiuhpp`), **before any local Git operation or edit**, inspect `git status -sb`, `git branch --show-current`, `git rev-parse HEAD`, and remote refs. If the worktree is clean and no other changes need preservation, synchronize `master` using `git switch master` then `git pull --ff-only origin master`; otherwise investigate without resetting, cleaning, stashing or overwriting anything. Remote `master` may have advanced since the SHA recorded above.
3. For any new work, define evidence and acceptance boundaries up front, create a dedicated branch, and keep scientific algorithms, seed policy, frozen reference artifacts and test scope intact. Never treat an inconclusive M9 performance comparison as a pass or a verified regression.
4. Do not commit, push, merge, tag, release, or modify `master` directly without the maintainer's explicit authorization. Preserve original M9 run data under `%USERPROFILE%\TonatiuhPP-Performance-M9\` outside the repository; no new benchmark is needed for the handoff.

## Closed workstream — M9 lambda-elimination post-merge validation (2026-11-16)

- **Merged source baseline:** production C++ lambda cleanup M1–M8 is merged in [PR #50](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/50), `master` commit `0e5bde8614b6b547990b8561b53b5f81872606b0`. Pre-cleanup comparison parent: `d4889f7083d1f5d4c134c2dce2d11a60a57fc759`.
- **M9 scientific check: PASS** for exact reduced-ray output equality across the executed Windows Release baseline/candidate comparisons (5 million rays, fixed seed, identical scene bytes). The frozen 500-million-ray reference was not rerun or regenerated.
- **M9 performance acceptance: INCONCLUSIVE / UNSTABLE ENVIRONMENT.** Two independent full comparisons returned paired candidate time changes of +0.418% and +1.129%, but both failed ABBA timing-stability criteria. Neither a reproducible regression over 2% nor equivalence within ±2% has been established.
- **Monitoring diagnostic:** Windows counter collection was sparse under full CPU load (34/34 windows in the monitored experiment; 4/4 in a reduced-counter probe), but regular at idle. Cause of timing drift remains unproven. Further benchmarks are deferred pending better-controlled measurement conditions; no source or performance-harness changes are warranted.
- **M9 closeout and provenance:** see [docs/performance-m9-acceptance.md](docs/performance-m9-acceptance.md). Local uncommitted evidence remains on the maintainer's Windows workstation. This is not release authorization.

## Historical project handoff — recorded 2026-11-12 (superseded)

The Phase E and testing-status narrative below is preserved as a dated historical snapshot. Its former "active work", "current handoff", and pending-merge statements do not describe the post-PR-#50 `master` checkpoint above. Do not resume superseded work based solely on that older snapshot.

**Active work:** Phase E — all three relocated test-enabled scientific GUI, `BUILD_TESTING=OFF` production runtime, and copied Qt IFW package-data acceptance paths now pass as **required** checks on Windows, Ubuntu and macOS in CI run `43310255444`. Final scoped review and user-authorized merge remain pending; actual IFW installer/update/uninstall transactions, signing and notarization are outside this evidence. Phase D is ACCEPTED and merged as PR #45 (merge commit `4c4121f5e52ac354dbe4d67bcb5472946d74be05`), with required Windows WGL, Ubuntu X11/GLX and macOS Cocoa native GUI CTests all passing. A+B+C are also accepted. This remains maintenance work, NOT an authorized product release.

**This document is the current handoff.** Older 2026-11-10 P0/P1/P2 candidate/pending diaries and runtime-refactor plans have been superseded by merged code and remain in Git history. Do not resume those plans. Read `AGENT.md` and `docs/testing.md` before changing code.

## Verified code and validation baseline

- Repository: `CST-Modelling-Tools/tonatiuhpp`; default branch `master`. **Accepted starting point for Phase D:** PR #43 merge commit `0c0b3ee3a9a16c733340a999ff3372efba9dcd46` (2026-11-12). The five-phase plan originally began at PR #41 merge `01400118f50557ae82883b523bb42905a2970c11` (2026-11-11).
- Earlier accepted production work: PR #39 (M0–M5 shared scientific engine), PR #40 (permanent GUI/CLI entrypoints, shared runtime and large File-export limit correction; merge `edf301a354cdec36c61f9d5c880c016fe7784528`), and PR #41 (scientific test rationalization; merge `01400118f50557ae82883b523bb42905a2970c11`).
- **Phase A+B:** PR #42 merged at `0e2edff29483ee0dd944ee585687b4a54d4390a0` (2026-11-11). It established `docs/testing.md`, the reproducible CTest inventory, modular test registration and behavior-oriented test paths. PR #42 CI run `43102812883` passed Windows, Ubuntu and macOS.
- **Phase C:** PR #43, `test(reliability): harden CTest execution and CI evidence`, merged on 2026-11-12 at `0c0b3ee3a9a16c733340a999ff3372efba9dcd46`. Its CI run `43160351782` completed successfully across `windows-latest`, `ubuntu-24.04` and `macos-latest`: **180/180**, **179/179** and **179/179** CTests respectively. Reported full-suite elapsed times: Windows 60.70 s, Ubuntu 42.60 s and macOS 39.49 s. Evidence: https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/43 and https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/43160351782 .
- **Windows maintainer Phase C evidence:** Release configure/build, preserved 180-test inventory, Python tooling/negative cleanup-guard checks, complete sequential and bounded parallel CTest runs (180/180), corrected `fast` tier (15/15; 13 units plus 2 CLI checks), and an interactive native GUI launch with tracing/flux. The GUI exercise is manual/opt-in validation, **not yet** required automated GUI CTest evidence.
- Production executables are `tonatiuhpp-gui` (Qt Widgets/SoQt application) and `tonatiuhpp-cli` (QCoreApplication CLI). Both share one scientific kernel and the production simulation/scripting services. The CLI does **not** accept `--headless`; the GUI rejects it. Retain the two-executable architecture; a dynamically loaded GUI module was previously evaluated and rejected.
- Important compiled targets: `TonatiuhppKernel`, `TonatiuhppLibraries`, `TonatiuhSimulation`, `TonatiuhScripting`, `TonatiuhScriptRuntime` and `TonatiuhppGuiPresentation`. The headless/kernel dependency boundary excludes Qt Widgets, Qt PrintSupport and SoQt; some shared code still intentionally uses QtGui/Qml.
- The accepted frozen 500-million-ray Windows heliostat benchmark (M5/PR #39) and its scientific reference remain valid only for their documented scope. **Never rerun the 500-million-ray benchmark or regenerate its frozen artifacts** during routine development/CI, and do not change physics, RNG/seed behavior, tracker policy, photon binary format, or flux/power normalization as part of testing work. See `docs/scientific-validation.md` for provenance and hashes. Passing build-tree CI is **not** installed-runtime, native GUI automation or release acceptance.

## Current testing landscape (accepted through PR #43)

- `source/CMakeLists.txt` includes CTest; `tests/CMakeLists.txt` coordinates modular registrations under `tests/unit/`, `tests/scientific/`, `tests/integration/cli/`, `tests/integration/scientific/` and `tests/architecture/`. Google Test uses `gtest_discover_tests()`. The historical Windows reference has **143 GoogleTests** (130 scientific, 13 unit) and 37 other CTests; the supported hosted baselines are **180 Windows**, **179 Ubuntu/macOS**. The sole intended Windows-only registration is `scientific.ScientificPluginPaths.WindowsBuildTreeIncludesMaterialPluginRoot`.
- CTest is the authoritative native discovery/execution/report interface. The full Release regression remains `ctest --test-dir build -C Release --output-on-failure`; a measured quick tier is `ctest --test-dir build -C Release -L fast --output-on-failure` with **15 tests** (13 unit and 2 CLI argument/startup tests). Historical `smoke` labels are broader and **must not** be mistaken for `fast`; overlapping label totals are not additive.
- Phase C made required Python and scientific fixtures configuration requirements, rather than silently losing registered scenarios. `tests/cmake/PrepareTestOutput.cmake` guards disposable build-tree output cleanup against traversal, symlink and redirected destinations; reviewed nested CMake process launches have explicit timeouts. `tests/tooling/test_test_output_guard.py` covers negative-path behavior. CI checks platform-aware inventories and test sentinels, and uploads `build/ctest-inventory.json` and `build/ctest-results.xml` (JUnit).
- Separate Python `unittest` checks validate CI classification, scientific auditors/comparators and CTest tooling; a passing native CTest total does **not** automatically include separately invoked Python tests or optional diagnostics. The standalone `tests/soqt_smoketest/` is also outside ordinary CTest registration.
- `tests/integration/cli/cli_process_repeatability.py` establishes scientific repeatability across **two independent real CLI processes**; it does **not** prove GUI/CLI equivalence. Its `--mode installed` offers a starting point for Phase E but does not mean installed-runtime acceptance has passed.
- The real native GUI acceptance helper lives in `tests/integration/gui/NativeGuiAcceptance.cpp` and `.h`, compiled into `tonatiuhpp-gui` only with `BUILD_TESTING=ON`. It is opt-in through `TONATIUHPP_M5_GUI_NATIVE_REPORT` and `TONATIUHPP_M5_GUI_NATIVE_MODE` (plus mode-specific variables), supports `interactive-file`, `seeded-interception` and `interactive-flux`, and exercises real Qt dialogs/Script Editor/flux paths. It is **not yet registered as routine automated GUI CTest**. Merely obtaining output, a positive flux or matching CLI runs does not establish independent physical accuracy.
- `tests/scientific/GuiSunSizingDiagnostic.cpp` is opt-in, not unattended acceptance. `installer/prepare_ifw_payload.py`, `.github/workflows/release.yml` and the installed mode of `cli_process_repeatability.py` provide Phase E foundations. No uniform unattended, three-OS GUI scientific acceptance or installed-artifact acceptance is claimed.
- `.github/workflows/ci.yml` performs three-OS Release build-tree validation for relevant changes and separately validates Python tooling. Markdown-only changes may use its tested change classifier and skip the native build; a documentation-only CI result must not be described as a fresh three-OS scientific regression.

## Approved FIVE-PHASE implementation plan

**Working principle:** Five coherent engineering phases, **not** five mandatory commits or five micro-PRs. Preserve all meaningful scientific coverage. Finish a phase as a useful whole before polishing peripheral details. No changes to physics or benchmark reference data. Maintain this status at meaningful checkpoints with exact evidence and state (local / branch / PR / merged).

### Phase 1 / A — Testing inventory, policy and classification

**Goal:** Make the testing contract and coverage discoverable before reorganizing files.

**Deliverables:** Author `docs/testing.md` as the current testing policy; produce or document a reproducible CTest inventory (prefer `ctest --show-only=json-v1`, with a small helper only if it materially helps). Include test names, owners/domain, level, runtime tier, execution interface, fixture/reference, platform prerequisites, deterministic/approximate assertions, and whether execution is build-tree, GUI or installed. Explicitly inventory separately executed Python `unittest` and standalone developer diagnostics.

Define a consistent label taxonomy: level (`unit`/`integration`/`system`), domain (for example `scientific`/`scripting`/`export`/`gui`/`architecture`), and tier (`fast`/`regression`/`extended`). Distinguish same-process component checks, CLI repeatability, numerical reference validation and real GUI/CLI comparison. Establish an unambiguous small quick-run command and the authoritative full regression command. Ensure required coverage is not confused with optional diagnostics.

**Gate:** Current 180-test baseline is accounted for on the reference Windows build; additional non-CTest checks are identified, labels are checked against actual discovery, and no test scenario or scientific evidence is dropped. **State: ACCEPTED; PR #42 merged with Windows + Ubuntu + macOS CI passing (2026-11-11).**

### Phase 2 / B — Test layout and CMake modularization

**Goal:** Organize tests by enduring behavior instead of M3/M4/M5 milestone labels.

**Deliverables:** Split the large `tests/CMakeLists.txt` into coherent, function-oriented subdirectories (e.g. `unit`, `scientific`, `integration/cli`, `integration/scripting`, `integration/export`, `integration/gui`, `architecture`, `support`, `fixtures`). Relocate the three `tests/m5/` survivors into their correct GUI/CLI areas. Review remaining historical driver/fixture names and only rename where doing so improves stable comprehension and all references can be updated safely. Retain established scientific fixture contents, report schemas, script APIs, generated artifact formats, baseline paths and output semantics.

**Gate:** Fresh configure/build succeeds; 180 existing scenarios remain discoverable and pass with no silent skips (a changed count needs explicit scenario-level justification); all affected file paths, CMake generator expressions, Windows DLL staging, Python references, docs and CI agree. Validate on Windows/Ubuntu/macOS before merging the structural change. **State: ACCEPTED; PR #42 merged and all three CI platforms passed.**

### Phase 3 / C — Test infrastructure reliability and reporting

**Goal:** Make test execution safe, deterministic, diagnosable and parallel-safe.

**Deliverables:** Consolidate duplicate subprocess/exit-code/output-management logic **only where maintenance gains justify it**; keep simple CMake drivers simple and retain Python for substantial structured comparisons. Use disposable test-root directories under the build tree; validate cleanup targets (including symlinks/path traversal), avoid accidental shared-output races, enforce process-level and CTest-level timeouts, and preserve stdout/stderr plus useful artifacts for failures. Remove ambiguous conditional registration for mandatory scenes/dependencies. Rationalize `smoke` vs `fast` semantics, and generate CI-readable CTest JUnit/summary artifacts where supported. Test the harness itself with deliberately failing cases.

**Gate:** Full suites succeed both sequentially and with bounded parallelism, failure injection yields correct nonzero results, cleanup refuses unsafe paths, no mandatory test silently disappears, and all three CI platforms pass. **State: ACCEPTED; PR #43 merged on 2026-11-12 after Windows local validation and successful Windows, Ubuntu and macOS hosted CI (run 43160351782).**

### Phase 4 / D — Genuine GUI scientific acceptance

**Goal:** Exercise real `tonatiuhpp-gui` flows as distinct from CLI reproducibility.

**Deliverables:** Wrap the existing native GUI helper in bounded process-level CTest drivers with isolated temporary output, reliable reports and explicit shutdown. Initially cover its three real GUI modes: interactive File export, seeded Script Editor interception, and interactive Flux Analysis. Validate generated numerical artifacts against independently trusted scientific expectations/comparators where applicable, not only against duplicate GUI/CLI executions or "positive output". Guard test-only behavior so production `BUILD_TESTING=OFF` binaries remain free of acceptance machinery.

**Gate:** A real display/Qt/SoQt/OpenGL environment is confirmed for each platform before making GUI checks required. Bounded timeouts, real widget interaction, independent evidence and no frozen 500M run. Add platform-specific CI gating only after reliable execution is proven; do not claim all three GUI environments are functional without actual evidence. **State: not started.**

### Phase 5 / E — Installed-runtime and packaging acceptance

**Goal:** Verify that what users install works independently of the source/build tree.

**Deliverables:** Integrate disposable CMake-install / IFW-payload checks with direct installed `tonatiuhpp-cli` and graphical `tonatiuhpp-gui` acceptance. Reuse the existing `tests/integration/cli/cli_process_repeatability.py --mode installed` CLI acceptance logic after checking its assumptions. Ensure scientific plugins, Qt platform plugins, SoQt/Coin runtime dependencies, macOS `.app`/Frameworks layouts, Linux RPATH and Windows DLL discovery function without borrowing the development SDK or build-directory paths. Distinguish installed-runtime smoke checks from comprehensive installer/upgrade testing; only claim what has actually been executed.

**Gate:** External-prefix/packaged installations pass bounded CLI and available real GUI scenarios, are relocated or launched outside source/build contexts, and pass platform-appropriate Windows/Linux/macOS checks. No modifications to user installation locations; no release authorization implied. **State: not started.**

## Execution and Git checkpoint policy (owner decision 2026-11-11)

- **Work in milestone mode, not micro-step mode.** Make coherent, reviewable changes and avoid cosmetic refactoring, fragmented micro-commits, and premature release claims.
- Phases A+B were accepted together in PR #42; Phase C was accepted in PR #43. Phase D (native GUI acceptance) and Phase E (installed-runtime acceptance) should have distinct validation and review checkpoints because they exercise different contracts. The five phases do not require five commits or a rigid PR count.
- Commit/push/open PRs for meaningful changes, CI validation or durable handoffs, not every tiny edit. Use a dedicated work branch. **Do not write directly to `master`, merge, release, delete user files or disturb a dirty local index without express authorization.**
- Before modifying the Windows checkout, confirm `git branch --show-current`, `git rev-parse HEAD` and `git status --short`. The maintainer reported synchronizing the local checkout after the Phase C merge (2026-11-12), but GitHub cannot verify uncommitted Windows-local state.
- For acceptance, record exact platform, commands, results, artifact/reference provenance, branch, commit/PR and scope. Passing one interface contract must not be relabeled as another. Supply a Conventional Commits message and modified-file summary as required by `AGENT.md`.

## Next-chat resumption instructions

1. Read `PROJECT_STATUS.md`, `AGENT.md`, `docs/testing.md` and `docs/scientific-validation.md` from the active branch, and compare with `master`. The accepted Phase D code baseline is `0c0b3ee3a9a16c733340a999ff3372efba9dcd46`; Phases A, B and C must **not** be restarted.
2. Verify the actual local Windows branch/HEAD/working tree before modifying local files. Keep a separate Phase D work branch; no direct `master` edits.
3. Begin Phase D by auditing `tests/integration/gui/NativeGuiAcceptance.cpp` and its integration in `tonatiuhpp-gui`. Preserve the real widget interactions and existing three modes; distinguish the helper's current opt-in behavior from CTest registration and confirmed platform support.
4. Implement bounded, isolated, process-level native GUI acceptance with reliable reports, explicit shutdown and **independent numerical references/comparators** where applicable. Never substitute two-CLI repeatability for GUI acceptance or claim correctness from nonempty/positive output alone.
5. Verify a real display/Qt/SoQt/OpenGL environment before enabling GUI acceptance as required CI on an OS. Collect exact Windows, Ubuntu and macOS evidence separately; do not enable unsupported display configurations or claim all three platforms were tested prematurely.
6. Maintain the immutable 500-million-ray benchmark, scientific implementation and `BUILD_TESTING=OFF` production separation. After Phase D acceptance, pursue Phase E in a separate installed-runtime/packaging checkpoint; no release is implied.

## Accepted A+B+C and Phase D starting checkpoint (2026-11-12)

- **A and B — ACCEPTED:** PR #42 merged 2026-11-11 at `0e2edff29483ee0dd944ee585687b4a54d4390a0`; hosted CI run `43102812883` passed three OSes. Windows maintainer evidence: 180/180 CTests, preserved identities, inventory/tooling validation and real GUI launch. Permanent behavior-oriented test layout is in place.
- **C — ACCEPTED:** PR #43 merged 2026-11-12 at `0c0b3ee3a9a16c733340a999ff3372efba9dcd46`. Its GitHub Actions PR run `43160351782` passed all three build jobs: Windows 180/180 (60.70 s), Ubuntu 24.04 179/179 (42.60 s), macOS 179/179 (39.49 s). Windows local maintainer validation included sequential and parallel 180/180, negative output-cleanup/tooling checks, 15/15 corrected fast-tier tests (13 unit) and interactive GUI tracing/flux. These are build-tree and separately reported manual GUI results, **not automated Phase D acceptance**.
- **D — NEXT / NOT STARTED:** Register and scientifically validate actual `tonatiuhpp-gui` interactions in bounded CTest drivers; establish reliable display-backed CI evidence per OS before making requirements mandatory. Do not change scientific references or trigger the frozen benchmark.
- **E — NOT STARTED:** Independently validate installed GUI/CLI runtime paths, dependencies, scientific plugins and selected small numerical outputs from disposable external install/payload roots.
- **Current scope:** All A–C acceptance evidence precedes this status-only update. Updating this Markdown document does **not** rerun any tests, authorize a release, or imply acceptance of D or E.

## Phase D implementation candidate checkpoint (2026-11-12)

- **Status:** Implementation candidate, **not accepted or merged**. Development branch:
  `maintenance/native-gui-scientific-acceptance`, based on Phase D baseline
  `a7473a2d805529c3efb1cfb00c043978255d15be` (PR #44 merged).
- Proposed optional registration: `TONATIUHPP_ENABLE_NATIVE_GUI_ACCEPTANCE=ON`
  adds three real GUI CTests, with optional Linux `xvfb-run` launch through
  `TONATIUHPP_GUI_ACCEPTANCE_USE_XVFB=ON`. Default existing Windows 180 /
  Linux and macOS 179 CTest inventory stays unchanged.
- The test-only Python runner executes actual GUI Qt workflows with bounded
  process lifetimes, isolated logs and reports, an independent cylinder GUI/CLI
  photon-export comparator, and 8x8 m planar 1,000 W/m² interception/flux
  analytical checks using the preexisting small scientific scripts.
- Python synthetic numerical-oracle positive/negative tests passed locally
  (13 tests). **Real Qt native acceptance has not yet been run or demonstrated
  on any supported OS in this checkpoint; CI acceptance is pending.** This
  statement is not an authorization to merge Phase D, modify physics or
  rerun the 500-million-ray frozen benchmark.
- Next: run on the maintainer's native Windows Release build in a genuine
  visible SoQt/OpenGL session, review retained per-mode scientific evidence,
  then establish reliable Ubuntu/macOS display-backed CI separately; record
  exact results before enabling required platform GUI gates.

### Phase D native GUI acceptance — three-platform evidence (2026-11-12)

- Native Windows Release maintainer evidence: **183/183** CTests, including
  the three real graphical workflows, plus independently reviewed numerical
  `acceptance.json` output. Earlier isolated native GUI run: 3/3.
- Ubuntu 24.04 hosted X11/GLX (Mesa llvmpipe) in CI run
  [43185720709](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/43185720709):
  **3/3** native GUI CTests, true widget/SoQt session and numerical reports,
  alongside successful ordinary 179/179 build-tree CTests.
- macOS Cocoa in that same CI run, after correcting regenerated obsolete
  AGL linker flags: **3/3** native GUI CTests, real Cocoa widget session and
  numerical reports, alongside successful ordinary 179/179 build-tree CTests.
- The real File-export GUI and CLI produce identical canonical photon
  multisets **within each platform**: 3184 records on Windows/Linux and 3197
  on macOS (per-photon power 0.0719665 W). This is not a claim of exact
  cross-platform RNG equality. Script interception is 63857.145538085264 W
  Windows/Linux and 63889.13809597008 W macOS; native Qt flux integrals are
  63857.145538085264 W Windows/Linux and 64225.0599537606 W macOS, against
  the independently expected 64000 W ideal planar absorber reference.
- **Phase D tested-scope cross-platform evidence is now verified.** The
  three graphical tests are being promoted to required Ubuntu/macOS CI
  checks; hosted Windows graphical feasibility is being probed independently
  while the real Windows desktop evidence remains accepted. Final PR review,
  the new required-gate CI run and user-authorized merge are still pending.
- Acceptance scope remains limited to the three small reference fixtures
  and documented 20% Monte Carlo/analytic criterion. No change to scientific
  kernel, sampling, photon serialization or immutable 500-million-ray
  heliostat benchmark. Phase E installed-runtime acceptance is separate.

### Phase D Windows hosted crash remediation (2026-11-12)

- Windows WER/CDB [43212203419](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/43212203419):
  null OpenGL dispatch in `Qt6Gui!QOpenGLFunctions::glViewport` during
  native expose/flush; `0xC0000005` underlying exception, outer
  `0xC000041D` callback termination. A 1024x768 `WinSta0\\Default`
  desktop existed; graphics adapter was `Microsoft Hyper-V Video`.
  No scientific GUI test action or numerical assertion had begun.
- Pinned Mesa3D 25.1.9 MSVC software WGL remediation:
  [43220096714](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/43220096714)
  achieved **3/3 actual hosted Windows native GUI tests** and
  **180/180 ordinary Windows CTests**. Canonical GUI/CLI photons
  3184/3184, interception and Qt flux 63857.145538085264 W,
  and script flux 63857.114 W. Native Windows desktop evidence
  183/183 remains independently valid.
- This milestone removes temporary WER diagnostic code/checkpoint
  instrumentation and makes hosted Windows graphical acceptance **required**
  just as Linux/macOS graphical gates already are. Mesa dependency is
  SHA256 pinned and staged only inside CI's disposable build/bin/Release
  directory, then removed before upload; not a production dependency.
- **Final required-gate CI run pending** at this checkpoint. Do not merge
  until all three mandatory OS graphical checks pass and the maintainer
  explicitly approves merging. Immutable 500-million-ray benchmark
  and production scientific kernel remain unchanged.

## Phase E initial implementation checkpoint (2026-11-12)

- **Base:** accepted master `4c4121f5e52ac354dbe4d67bcb5472946d74be05`
  (Phase D PR #45 merged). Branch:
  `maintenance/installed-runtime-packaging-acceptance`. No direct
  master changes or merge are authorized.
- **Initial Phase E scope:** new disposable relocated CMake-install pilot
  on the three-OS CI matrix. Reuse the preexisting scientific installed
  CLI repeatability and actual Qt GUI acceptance scripts; require
  installed GUI/CLI executables and installed cylinder/sphere example
  scenes. Launch scientific subprocesses from unrelated working
  directories with SDK/library-loader environment overrides removed.
  On Windows, continue to use the verified SHA256-pinned Mesa software
  WGL only as temporary hosted CI infrastructure.
- **New evidence:** installer relocation guard self-tests explicitly
  reject unexpected paths, source-tree installs and symlinked install
  roots. The GUI reports remain actual Qt/SoQt real-dialog and scientific
  analytical comparisons, not offscreen simulations.
- **Pilot status:** exploratory and `continue-on-error` until each
  platform's relocation, native dependency loading, plugin lookup and
  numerical results are examined. A green overall CI build does not
  mean this pilot succeeded.
- **Remaining acceptance:** repair any actual installed-runtime
  failures, audit macOS Frameworks/plugin RPATH, verify production
  `BUILD_TESTING=OFF` release-like payloads, and distinguish CMake
  installs from Qt IFW installer/update behavior. Frozen 500M artifact
  remains immutable; no production physics change. PR stays draft
  until scientific evidence supports a final acceptance decision.

### Phase E first CI evidence — genuine installed-runtime failures

- First pilot [43269447246](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/43269447246)
  ran Linux/macOS/Windows CMake installation into disposable runner temporary
  roots. The installed CLI passed on Linux and Windows; all three relocated
  GUI/runtime paths were **not yet accepted**. The step was
  `continue-on-error`, hence green overall jobs must not be conflated
  with pilot success.
- Linux: missing `libQt6OpenGL.so.6` at GUI launch (exit 127) despite the
  library being present in installed `lib`. Patch installed bundled shared
  library `RUNPATH` for transitive dependencies.
- macOS: missing private `@rpath/libTonatiuhppKernel.dylib` from raw
  CMake-installed CLI; macdeployqt does not bundle all private runtime
  modules. Reuse release app consolidation in the pilot.
- Windows: missing runtime DLL at installed GUI startup
  (`0xC0000135`), while installed CLI passed. Add explicit Qt6OpenGL
  DLL deployment for SoQt's indirect dependency, then require actual
  rerun evidence before concluding this was the missing DLL.
- No benchmark regeneration or production scientific changes. Next:
  inspect second pilot outcomes and harden remaining installed dependencies;
  explicit new production `BUILD_TESTING=OFF` payload validation remains
  required for final Phase E acceptance.

### Phase E consolidated fix and production-smoke pilot (2026-11-12)

- [Second CI run 43272756323](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/43272756323)
  completed with default CI and Phase D native GUI gates green.
  **Linux/macOS relocated CMake installs passed** installed CLI plus
  all three actual GUI scientific modes. Windows installed CLI passed
  but installed GUI still exited `0xC0000135` before scientific output.
  The non-gating pilot step was marked as failed despite parent CI success.
- Windows PE import audit of the hosted CI artifact:
  `TonatiuhppGuiPresentation.dll` requires `Qt6PrintSupport.dll`,
  absent from the captured build runtime; `SoQt1.dll` requires
  `Qt6OpenGL.dll`, present after the earlier targeted fix. Candidate
  install correction explicitly bundles Release Qt6PrintSupport,
  Qt6OpenGL and Qt6OpenGLWidgets. A new fail-closed installed-GUI
  dependency preflight and positive/negative tool tests expose the
  exact missing DLL, independently of opaque process exit status.
- Start a **separate BUILD_TESTING=OFF** exploratory pilot after the
  test-enabled installed acceptance: reconfigure and rebuild the real
  production GUI/CLI, relocate its install, run actual production CLI
  scientific acceptance and real native GUI liveness from a foreign CWD.
  The production GUI does NOT have test-only Qt dialog hooks, and a
  seven-second startup smoke cannot establish full GUI numerical
  acceptance. No IFW installer/updater test is implied.
- Three-platform CI for these candidate changes remains to be
  executed and reviewed. Only then consider promoting completed
  installed-runtime acceptance to a required gate; IFW packaging,
  release environment independence and maintenance updates remain
  separate evidence requirements. **PR #46 remains draft/unmerged.**

### Phase E preliminary Qt IFW production-payload pilot

- The next exploratory CI checkpoint will stage the existing
  `BUILD_TESTING=OFF` CMake install through
  `installer/prepare_ifw_payload.py`, make a separate copy of the
  `com.tonatiuhpp.app/data` payload in runner temporary storage,
  relocate it, and execute production CLI scientific acceptance plus
  actual native GUI startup. This is separate from the already-tested
  `BUILD_TESTING=ON` dialog acceptance and plain production CMake
  install smoke. Do not infer full GUI numerical acceptance or actual
  IFW installer/update/uninstall execution from these checks.
- Pilot remains `continue-on-error` until individual Linux, macOS and
  Windows results, loader paths and numerical evidence are reviewed.
  All prior required Phase D tests remain unchanged; PR #46 stays draft.

### Phase E three-platform installed and package-data pilot success (2026-11-12)

- [CI 43304366649](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/43304366649):
  all three ordinary CI platform jobs passed. Individual **relocated
  BUILD_TESTING=ON** installed CLI and real GUI file/interception/flux
  scientific acceptance: PASS on Windows, Ubuntu and macOS.
  Actual numerical JSON records were inspected: 3184/3184 matching
  GUI/CLI photons on Windows/Linux; 3197/3197 on macOS; intercepted
  power around 63.86 kW; Qt grid power around 63.86–64.23 kW versus
  independently defined 64 kW planar reference.
- **BUILD_TESTING=OFF** production CMake-installed CLI scientific
  acceptance and real native GUI startup: PASS on all three OSes.
  **Copied Qt IFW package-data** CLI scientific acceptance and real
  GUI startup: PASS on all three OSes. IFW payload manifests report
  467 Linux, 254 macOS and 221 Windows files in their respective
  tested payloads. These are structural counts, not a claim of exact
  matching package contents across platforms.
- The Linux indirect `RUNPATH`, macOS private dylib bundle and Windows
  GUI `Qt6PrintSupport.dll` dependency problems are corrected and
  verified under relocated installed workloads. Qt IFW **installer,
  updater and uninstaller transactions, signing and notarization**
  have not been exercised; the test contract explicitly excludes them.
- Promote the three passing Phase E pilots to required checks and add
  a stricter Mach-O external-development-path audit to macOS bundle
  consolidation. A final CI run must pass **all required checks and
  new audit** before Phase E may be considered accepted. PR #46
  remains draft and unmerged; frozen 500M benchmark and production
  scientific logic remain unchanged.

### Phase E release-workflow parity checkpoint

- The macOS Release workflow now calls the same bundled library
  consolidation/RPATH audit helper exercised by Phase E instead of
  duplicating a divergent inline implementation. The shared helper
  is limited to CI's disposable temporary paths and the release
  job's exact `GITHUB_WORKSPACE/dist` output directory.
- This aligns release *implementation* with the tested CI pilot;
  production Release tag, signed distribution, Qt IFW binarycreator
  installation and updater transactions are **not claimed as tested**.
  PR remains draft pending final required three-OS CI audit results.

### Phase E final required three-platform validation checkpoint (2026-11-12)

- [Required CI run 43310255444](https://github.com/CST-Modelling-Tools/tonatiuhpp/actions/runs/43310255444), PR #46 head `785ffb2989f2afa0e976c85164498c04d2b5c9b6`: **all three platform build jobs succeeded** with mandatory (not `continue-on-error`) Phase E steps. Ordinary CTest: Windows **180/180**, Ubuntu **179/179**, macOS **179/179**. Separately required native Qt GUI scientific CTests: **3/3 on each OS**.
- Relocated `BUILD_TESTING=ON` CMake-installed CLI scientific process checks and all three real GUI native acceptance modes (`interactive-file`, `seeded-interception`, `interactive-flux`) explicitly emitted `INSTALLED_ACCEPTANCE_RESULT=PASS` for Windows, Ubuntu and macOS. The accepted comparison oracles are the existing Phase D test scripts, not a new reference or frozen 500M execution.
- Relocated production `BUILD_TESTING=OFF` CLI scientific checks and bounded native GUI startup also each explicitly emitted **PASS** on all three OSes. Production GUI liveness is intentionally **not** dialog-level numerical acceptance because production binaries omit test-only hooks.
- A copied, relocated Qt IFW **package-data payload** passed the same production CLI scientific and GUI startup checks on all three OSes. Manifest sizes: **Windows 221**, **Ubuntu 467**, **macOS 254** files (platform-dependent counts, not an assertion of parity).
- The CI jobs contain real installed-runtime checks with hard failure semantics; this checkpoint is supported by individual job logs, not inferred solely from green aggregate jobs. The PR remains **draft and unmerged**. Actual IFW binarycreator installation/upgrades/uninstallation, signatures/notarization, user-machine deployment and hardware-specific OpenGL behavior are **not** proven by this run. No release or merge authorization is implied; preserve production science and immutable 500-million-ray reference.
