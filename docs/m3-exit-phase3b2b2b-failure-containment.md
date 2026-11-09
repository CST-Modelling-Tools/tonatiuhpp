# Tonatiuh++ M3 exit — Phase 3B2B2B failure containment acceptance

**Status: ACCEPTED for the implemented and native-Windows-tested Slices A, B, C, D1 and D2. Full M3 architectural exit remains OPEN.**

This record consolidates the uncommitted local work on branch
`refactor/m3-script-api-exit`, pinned Git HEAD
`ab1dc1073d566bf39308567aa0eec082f1794994`. It is an acceptance record,
not a claim that changes have been staged, committed or pushed. Preserve
independent `PROJECT_STATUS.md` edits. Do not begin M4 or the frozen
500-million-ray benchmark.

## Implemented increments and scope

| Slice | Change or test boundary | Native Windows acceptance |
| --- | --- | --- |
| A | `PhotonsBuffer::endExport()` latches failed exporter finalization, including an exporter error reported at finalization; successful finalization remains healthy | 3 new tests; full scientific 84/84, headless 35/35 |
| B | `PhotonsFile` latches metadata parameters open, stream write, flush and close errors | 2 new production-plugin tests; scientific 86/86, headless 35/35 |
| C | Deterministic binary write/flush failure injection in production `PhotonsFile`, restricted to test context; check failure and finalization rejection | 2 new tests; corrected uninitialized default-photon fixture; 5/5 repeated write and 5/5 flush; scientific 88/88, headless 35/35 |
| D1 | Real exporter/buffer successful checkpoint followed by failed append write/flush; preserve data and metadata bytes, retain unwritten photons, reject restart | 2 new tests; 5/5 each; scientific 90/90, headless 35/35 |
| D2 | Real script-host `tn.Run()` append write/flush failure; poisoned host refuses append and reset, including after scene clearing; independent host still works; retained resources survive through teardown | 2 new tests; 3/3 each; scientific 92/92, headless 35/35 |

All counts above come from user-supplied native Windows CTest logs. Each
increment also passed a reported `git diff --check`. The latest D2 full run
recorded **92/92 scientific** in **63.19 s** and **35/35 headless** in
**72.24 s**, with zero failures. Git emitted a non-fatal LF-to-CRLF warning
for `tests/scientific/ScriptLegacyCorpusTests.cpp`; no whitespace errors
were reported. The D2 tests were registered as CTest 139 and 140.

## D2 environment and fixture corrections

The D2 test executable runs from `build/tests/scientific/Release`. Its
production plugin search roots are relative to the executable, so it did not
initially find the existing `build/plugins/photons/Release/PhotonsFile.dll`.
A **build-tree-only directory junction**
`build/tests/scientific/plugins` -> `build/plugins` made the production plugin
visible without altering the application's plugin discovery. Merely setting
`PATH` and `QT_PLUGIN_PATH` was insufficient for the explicit discovery
mechanism. A proposed D2 plugin-path C++ correction was **not applied**.

The first-run diagnostic then exposed an inaccurate shared test assertion:
the actual script error is
`tn.Run: trace was not completed: Photon buffer/export failed during ray tracing.`
The test was corrected to expect the production message; **one shared
assertion** was changed, not two. The test fixture was rebuilt and D2
repeated focused and full regressions passed. These are test/environment
corrections, not a production behavioral change.

The earlier Slice C tests used `std::vector<Photon> photons(1)` while the
`Photon` default constructor is empty; this could leave a surface pointer
uninitialized. The fixture was corrected to use the parameterized `Photon`
constructor with `surface=nullptr`, after intermittent Windows access
violations. No production export serialization was changed for this fix.

## Fail-closed contract established by testing

- Successful completed File output is the last accepted checkpoint. Subsequent
  failed append write/flush cannot silently replace its binary data or
  metadata; the tested prior files remain byte-for-byte unchanged.
- Export and buffer errors latch rather than allowing a false successful
  completion. Unwritten buffer contents are retained until safe teardown.
- A poisoned script host refuses further append and non-append reset, even
  following scene clearing. An independent host remains operational.
- The exporter, buffer, scene instance tree and strong scene reference are
  retained with dependency-aware teardown within the existing backend.
- The format, accepted bounded append settings, GUI `MainWindow::Run`, and
  science algorithms are not changed by the acceptance tests.

## Limits and remaining M3 exit work

These tests do **not** prove recovery from every real OS I/O error, nor
replay/reconstruction of failed output, all File options, concurrent
sessions, or multi-segment rollovers. The existing exporter ownership
retention was tested; a separate architectural ownership **extraction** is
not claimed completed. Historical GUI/headless photon parity was verified at
Phase 3B2B2A; it was not rerun for every 3B2B2B slice. The accepted
3B2B2A restriction remains cumulative at most 200,000 rays with unchanged
scene, selections and exporter settings, fresh initial directory, seed 0 and
FileSize=-1. The full GUI execution lifecycle is still owned by
`MainWindow`, and Phase 3B3 `RunFluxAnalysis` and remaining GUI-bound script
APIs still require work. Full M3 exit remains OPEN.

## Revalidation on the preserved Windows working tree

```powershell
cd C:\OpenSource\tonatiuhpp
ctest --test-dir build -C Release -R '^scientific[.]' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Scientific regression failed' }
ctest --test-dir build -C Release -R '^headless[.]' --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Headless regression failed' }
git diff --check
if ($LASTEXITCODE -ne 0) { throw 'Whitespace check failed' }
```

No commit, push, `PROJECT_STATUS.md` modification, M4 work, or frozen benchmark
is performed by this documentation increment.
