# M2 - Ray-propagation semantics and single-loop consolidation

**Status (2026-10-20):** **M2.1 characterization and M2.2 single-loop consolidation both PASSED local Windows/MSVC Release acceptance.** Both application and scientific-test targets built, 28/28 scientific CTests, 5/5 native ray-history/photon-export equivalence CTests, 2/2 bounded benchmark CTests, and native fixed-seed GUI Run/Flux Analysis with a confirmed nonzero flux map passed. **M2 hosted Windows/Ubuntu/macOS CI, review and merge remain pending.** Architectural M1 was previously completed in merged PRs #28 and #30.

## Legacy dual-loop behavior characterized before M2.2

- Before consolidation, `RayTracer::operator()` dispatched on `m_photonBuffer && m_mutexPhotonsBuffer` (whether photon buffering was active), not on a physics policy. Both paths generated a primitive ray, intersect the instance tree, draw air-transmission RNG after at least one reflection, issue reflected-hit callbacks, and report final termination. Both consumed the same RNG stream for equivalent scenes and seeds (existing baseline tests).
- **Terminal hit callback order differs:** when a terminal surface is hit, the no-recording path issues its terminal `HitCallback` **before** `HistoryCallback`, whereas recording mode issues `HistoryCallback` **before** the terminal `HitCallback`. This matters to callbacks with external side effects; it is not a change in ray physics. Reflected-hit callbacks occur before termination in both branches.
- **Selected-surface filter leaks into terminal hit callbacks:** in recording mode, a nonempty `m_exportSurfaceList` can suppress a terminal `HitCallback` if the terminal surface is not selected. Reflected-hit callbacks are not filtered; no-recording mode ignores this selection. M2.2 explicitly preserves this legacy distinction as a compatibility rule rather than silently changing it.
- **Air-loss handling:** after at least one reflection, an air-transmission loss clears the intersected surface and resets `ray.tMax` to `gcf::infinity`. The history hook preserves the sampled terminal distance separately (`airDistance`) before that reset. Recording mode increments `rayLength` on this branch; the no-recording path does not, but this currently only affects recording-related path bookkeeping. A path that misses after reflection can have an infinite sampled segment, so do not assume every air termination has a finite terminal distance.
- **Photon-only semantics:** recording may add the sun photon at path ID 0, reflected/intermediate photons at their existing nonzero path indices, and terminal points subject to the export selection. Filtering must **not renumber** these path-local IDs. File format and exporter buffering must remain unchanged.
- The history callback's terminal record is created before export-only `ray.tMax` normalization, so it is suitable for comparing the scientific propagation path under both modes.

## Focused M2.1 regression additions

`tests/scientific/TraceEquivalenceTests.cpp` adds three `ScientificPropagationSemantics.*` GoogleTests using **existing fixtures** and the **existing GUI borrowed-tree preparation** in `traceOnce`:

1. `TerminalCallbackOrderDiffersByRecordingMode`: one-chunk cylinder trace, exact histories, event ordering, and recorded sun point (ID 0).
2. `FilteredExportSuppressesRecordedTerminalHits`: one-chunk two-reflection specular fixture selecting only `//Node/First/Shape`. Verify identical full histories, preserved callbacks at the selected surface, the historically suppressed receiver terminal callbacks in recording mode, reflected-hit visibility, and filtered photon surface/path IDs.
3. `AirLossTerminalRecordsMatchWithRecording`: one-chunk non-vacuum Fresnel fixture. Compare all exact per-ray history fields and terminal state, confirm actual atmospheric-transmission calls and observed air terminations, and check callback order in both modes.

The existing exact-hit, photon-equivalence, tracker, selected-surface-independent, native ordered-history and benchmark tests remain essential. The M2.1 characterization patch did **not** change production tracing. The separate M2.2 patch changes `RayTracer.cpp` only, retaining the same RNG draws, hit/history observations, photon-ID/export rules and worker scheduling. No optics, seed derivation, scheduler, air model, photon file format or immutable reference edits are proposed. The previously accepted M0/M1 evidence remains valid.

## Local validation gate and observed results

From the current Windows build directory, with the runtime DLL directories available on `PATH`:

```powershell
cmake --build build --config Release --target tonatiuhpp_scientific_equivalence_tests --parallel 8
ctest --test-dir build -C Release -R '^scientific[.]ScientificPropagationSemantics[.]' --output-on-failure
ctest --test-dir build -C Release -R '^scientific[.]' --output-on-failure
```

**Observed on Windows/MSVC Release, 2026-10-20:** both production application and scientific-test builds passed; all **28/28 scientific tests** passed; and all **5/5** native headless ray-history/photon-export parity tests passed. Existing nonfatal `FluxAnalysis.cpp` warnings C4834 and C4805 were observed. These results established the **pre-consolidation M2.1 baseline**. The post-consolidation M2.2 acceptance results are recorded below. The frozen 500-million-ray benchmark was not rerun for this bounded refactor.

## M2.2 implementation policy: preserve measured behavior

**Compatibility choice for this consolidation:** preserve the existing recording-only export-filter suppression of terminal `HitCallback` and its terminal callback ordering **as explicitly tested by M2.1**. This is not an endorsement of the coupling as a long-term scientific API; standardizing unfiltered hit observers or callback ordering would be a separate, reviewed policy change with explicit parity evidence. Keep all three M2.1 assertions as long as this compatibility contract remains.

The M2.2 patch uses one outer launch loop and one inner intersection/reflection/air-attenuation loop for both modes. Its record-only observations append the initial sun photon, intermediate bounce photons and terminal photon with identical local IDs, selection rules and `tMax` conversion. A mode-specific terminal-observer block retains the current hit-before-history ordering without recording and history-before-filtered-hit ordering with recording. The final photon-buffer write still occurs once per traced chunk under the existing mutex, with the same export-failure signaling. No new RNG calls, scheduler changes or extra instance-tree preparation are introduced.

**M2.2 local Windows acceptance PASSED (2026-10-20).** The maintainer applied the consolidation to the existing M2.1 worktree and validated the rebuilt application and scientific-test executable. The final MSVC Release build completed successfully without recompiling the binaries that had passed the tests. The Windows runtime initially returned `0xC0000135` for the three new CTests because required DLL directories were missing from `PATH`; after restoring the runtime paths, all three passed. This was an environment loader error, not a failed scientific assertion.

- **28/28 scientific CTests:** including all three `ScientificPropagationSemantics` characterization tests, GUI/headless preparation, exact scientific hit events, exponential air, specular and tracker cases.
- **5/5 native headless equivalence CTests:** three independently launched ordered ray-history comparisons (cylinder, specular, tracker) and two photon-export parity cases (scientific, tracker).
- **2/2 bounded benchmark CTests:** `headless.benchmark_repeatability` (20,001-ray cylinder cross-process scientific repeatability) and `headless.benchmark_heliostat_10k` (10,000-ray heliostat smoke). These demonstrate successful execution and repeatability, **not** a measured before/after throughput comparison.
- **Native Windows GUI execution:** cylinder fixture, fixed master seed `123456789`; Run completed in **71 s**, Flux Analysis in **23 s**, and the maintainer confirmed the flux map was nonzero.
- **Integrity:** `git diff --check` reported no errors. No frozen scientific references, RNG policies, default tracker options or photon file formats were changed. No 500-million-ray rerun was requested or performed.

**M2 local exit gate is satisfied for the exercised Windows scope.** Hosted Windows/Ubuntu/macOS CI, code review and merge of the single combined M2.1+M2.2 PR remain outstanding. Representative benchmark smokes passed; **no quantitative no-recording throughput non-regression claim** is made without a controlled before/after timing comparison.
