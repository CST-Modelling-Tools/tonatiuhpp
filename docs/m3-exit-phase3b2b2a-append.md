# Tonatiuh++ M3 exit — Phase 3B2B2A: bounded cumulative File append

**Status: ACCEPTED on native Windows for the bounded 3B2B2A scope; Phase 3B2B2B Slices A–D2 subsequently accepted. Full M3 architectural exit remains OPEN.**

Baseline is the locally accepted, uncommitted Phase 3B2B1 source on
`refactor/m3-script-api-exit`, pinned HEAD
`ab1dc1073d566bf39308567aa0eec082f1794994`. The reconstruction
includes the previously accepted Phase 3B2A test EOF correction and Phase
3B2B1 CMake cylinder fixture-path correction. Do not stage, commit, push,
run the frozen 500-million-ray benchmark, or start M4.

## What this candidate changes

- An independent **per-script-host** `HeadlessScriptRunFileBackend` owns the
  actual existing production File plugin, its settings and `PhotonsBuffer`,
  an owned `SceneInstanceTree`, and a strong Coin scene reference. A script
  may now execute two or more synchronous `tn.Run()` calls with
  `tn.SetPhotonBufferAppend(true)` and an unchanged exporter configuration.
- Headless `TracePreparation` borrows this persistent instance tree **only
  when explicitly requested** by the script adapter. Existing headless CLI,
  A0 diagnostic, benchmark and no-export preparation still build their own
  tree exactly as before. Persistent instance pointers prevent the File
  exporter from holding dangling surface IDs across appended runs.
- On an accepted completed append, the metadata photon power is normalized
  using the **cumulative sum of completed rays**, matching the existing GUI
  `m_raysTracedTotal` rule. Each call is bounded to 1–200,000 rays and a
  cumulative append series to 200,000 rays. Fixed seed 0, FileSize=-1, an
  absolute pre-existing initially empty directory, the production File plugin,
  and the same photon byte format remain required for bounded parity.
- After every successful File append, the adapter hashes the production data
  and parameters files. A later append rejects a changed file, modified
  exporter/selection/buffer setting, replaced scene, already-failed exporter,
  and a zero-photon first-run result. All such preflight rejections occur
  **before tracing, exporter restart or mutation of existing output files**.
- A successful non-append subsequent run uses a new exporter/buffer and
  requires a fresh empty output directory. Reset is blocked if the earlier
  export failed, so any unwritten photons still buffered in the script host
  cannot be discarded by a script-level retry. On worker cancellation,
  incomplete tracing, `endExport` failure or exporter restart failure,
  further runs latch fail-closed and do not treat partial output as accepted.
  **There is no automatic replay or repair of an interrupted File export.**
- `PhotonsFile` retains its GUI modal diagnostics when an actual QApplication
  exists; in true headless QCoreApplication processes its five warning sites
  log `qWarning` without trying to create a modal QWidget. All normal File
  serialization code and formats are otherwise unchanged.
- GUI `MainWindow::Run`, GUI progress/cancellation and historical GUI append
  logic remain unchanged. The existing shared `tn.Run` callback still routes
  GUI scripts to that historical adapter, while the headless callback now
  captures one persistent backend per script host.

## Source and fixture footprint

Seven guarded source/build edits:

1. `source/application/headless/HeadlessScriptRunFileBackend.h`
2. `source/application/headless/HeadlessScriptHost.cpp`
3. `source/application/core/TracePreparation.h`
4. `source/application/core/TracePreparation.cpp`
5. `source/plugins/photons/PhotonsFile/PhotonsFile.cpp`
6. `tests/scientific/ScriptLegacyCorpusTests.cpp`
7. `tests/CMakeLists.txt`

Four new repository files:

- `tests/scripts/m3_exit_phase3b2b2_append.tnhpps.in`
- `tests/cmake/run_m3_exit_phase3b2b2a_append.cmake`
- `tests/scientific/verify_m3_exit_phase3b2b2_append.py`
- `docs/m3-exit-phase3b2b2a-append.md`

The delivery-only `compare_m3_exit_phase3b2b2a.ps1` compares the native GUI
and native headless canonical physical photon records using the *existing*
A0 comparator. The Python append verifier uses that same A0 parser; no new
export format or numerical science model is introduced.

## Guarded installation on the accepted local Windows tree

```powershell
cd C:\OpenSource\tonatiuhpp
$phase3b2b2a = "$HOME\Downloads\Tonatiuhpp_M3_Exit_Append_Lifecycle_Phase3B2B2A"
Expand-Archive "$HOME\Downloads\Tonatiuhpp_M3_Exit_Append_Lifecycle_Phase3B2B2A.zip" -DestinationPath $phase3b2b2a -Force
python "$phase3b2b2a\apply_m3_exit_phase3b2b2a.py" --repo 'C:\OpenSource\tonatiuhpp' --check
if ($LASTEXITCODE -ne 0) { throw 'Phase 3B2B2A preflight blocked' }
# Only if the preflight is clean:
python "$phase3b2b2a\apply_m3_exit_phase3b2b2a.py" --repo 'C:\OpenSource\tonatiuhpp' --apply
if ($LASTEXITCODE -ne 0) { throw 'Phase 3B2B2A apply blocked' }
git diff --check
if ($LASTEXITCODE -ne 0) { throw 'Phase 3B2B2A whitespace check failed' }
```

Expected preflight: `7 exact source/build anchors and 4 new files`, correct
branch and HEAD, no writes. The installer checks **all reconstructed prior
accepted source and fixture files**, not just these seven, except explicitly
allowed user-edited `PROJECT_STATUS.md` and documented accepted Phase 3A doc
variants. It backs up all overwritten files in `build/tests` and rolls back
partially written edits. No force option or index mutations.

## Native Windows scientific and runtime validation

Maintain the already corrected Qt/third-party test runtime PATH in the
PowerShell process (see the prior 0xc0000135 diagnostic). Only the normal
small regression traces are run.

```powershell
cmake --build build --config Release --target tonatiuhpp PhotonsFile tonatiuhpp_scientific_equivalence_tests --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Phase 3B2B2A Release build failed' }
ctest --test-dir build -C Release -R '^scientific[.](ScientificScriptAppendDispatch|ScientificScriptRunDispatch|ScientificGuiScriptFacade)[.]' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Phase 3B2B2A focused scientific tests failed' }
# Expected 8/8: two new append dispatch, four prior Run, two facade tests.
ctest --test-dir build -C Release -R '^scientific[.]' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Full scientific regression failed' }
# Expected: 81/81 (79 accepted + 2).
ctest --test-dir build -C Release -R '^headless[.](m3_exit_phase3b2b2a_append|m3_exit_phase3b2b1_run_file|m3_exit_phase3b2a_export_config|m3_exit_phase3b1_run_inputs|m3_exit_phase3a_field_grid|m3_exit_phase2_sun_position|m3_script_parity_baseline|m3_script_tracker_parity|run_script_cpc_curved|run_script_cpc_faceted|run_script_interception_bounded|m3_script_cancel_recover|photon_export_scientific_equivalence)$' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Selected headless scientific/File regression failed' }
# Expected: 13/13, assuming existing A0 File test is configured.
```

The new native CTest launches independent File exporter processes:

- 2048-ray single run, fixed seed 0; 2048+2048-ray append run in one host;
  exact multiset of physical photon records must double; stable surface URL
  and surface-ID cardinality must be preserved; serialized photon power must
  halve subject to its existing decimal precision.
- Attempt to change the buffer size before append: reject at `tn.Run`,
  preserving the first complete production photon file byte-for-byte.
- Deliberately tamper with the parameters file **only inside the disposable
  test directory**: reject the second `tn.Run` before reopening the plugin,
  keeping the binary data file byte-for-byte unchanged.
- Generate the native GUI script as a separate fixture.

## Native GUI/headless append parity acceptance

After **all CTests complete**, open rebuilt GUI with the **fixed master seed
0**, and run the generated script in a fresh GUI Script Editor:

`C:\OpenSource\tonatiuhpp\build\tests\m3_exit_phase3b2b2a_append\script\m3_exit_phase3b2b2a_gui.tnhpps`

Expected output:

`M3 Phase 3B2B2A cumulative File append passed.`

The GUI will run two ordinary `MainWindow::Run` sessions (normal modal
progress each time). It must produce `gui/PhotonMap.dat` and
`gui/PhotonMap_parameters.txt`, and these must compare physically with the
headless `append/` files:

```powershell
& "$phase3b2b2a\compare_m3_exit_phase3b2b2a.ps1" -RepositoryRoot 'C:\OpenSource\tonatiuhpp'
if ($LASTEXITCODE -ne 0) { throw 'GUI/headless append photon parity failed' }
git diff --check
if ($LASTEXITCODE -ne 0) { throw 'Git whitespace validation failed' }
```

Required comparator message: `Photon export canonical records match (...)`
followed by `M3 EXIT PHASE 3B2B2A: NATIVE GUI/HEADLESS APPEND PHOTON PARITY
PASSED`. Comparison ignores nondeterministic raw record ordering and global
photon IDs, but preserves exact physical-position bits, side, stable URL,
count and serialized power. **Do not rerun the append CTest after the GUI
run:** its fixture cleanup intentionally resets the GUI output directory.

## Accepted native Windows verification (historical result, not a command to rerun)

The original Phase 3B2B2A acceptance completed with the corrected five-site
headless diagnostic guard, a native Release build, M3 exit smoke **6/6**,
focused scientific **8/8**, full scientific **81/81**, selected headless
**13/13**, and `git diff --check` success. The generated GUI script printed
`M3 Phase 3B2B2A cumulative File append passed.`; the existing A0 comparator
reported canonical GUI/headless parity for **2,272 photons**, photon power
**0.0359832**, and surface cardinality **1/1**, followed by
`M3 EXIT PHASE 3B2B2A: NATIVE GUI/HEADLESS APPEND PHOTON PARITY PASSED`.
These are the accepted historical results, not a claim that GUI parity was
rerun after each later 3B2B2B slice. Subsequent full scientific/headless
regressions include the registered 3B2B2A append test.

## Limits, omissions and next milestones

- True-headless append is accepted **only** for unchanged in-memory scene
  topology, unchanged File exporter settings, FileSize=-1, fresh File output,
  200k total rays or fewer, and the tested selected-surface/global-coordinate
  record configuration. Other File options, per-run scene modifications,
  concurrent sessions and multi-file segment rollover are not accepted.
- Failure after File I/O begins is **not** repaired in this slice. Buffers
  stay alive only until the script host exits; a failed host cannot be
  assumed to rescue unwritten photons. No destructive reset is allowed on a
  poisoned instance. Subsequent Phase 3B2B2B Slices A–D2 validated failure
  latching, deterministic File write/flush failures, append containment,
  and host poisoning/retention; see `m3-exit-phase3b2b2b-failure-containment.md`.
  This does not constitute automatic recovery or a complete ownership extraction.
- The GUI `Run` state machine is still owned by `MainWindow`, not extracted
  into a fully shared nonvisual execution layer. Historical export types and
  complete GUI/headless lifecycle interchangeability remain unfinished.
- Phase 3B3 `RunFluxAnalysis` and the other GUI-bound script APIs remain
  pending. Full **M3 architectural exit is OPEN**; M4 stays separate.

## Eventual commit title (NOT performed)

`refactor(script): pin photon export resources for bounded append parity`
