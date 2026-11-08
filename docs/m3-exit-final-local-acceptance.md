# Tonatiuh++ — M3 integrated local acceptance and formal exit review

**Record date:** 2026-11-08. **Branch:** `refactor/m3-script-api-exit`. **Pre-checkpoint HEAD:** `2ec6d5a355cbb4b2ec6ab504a7573322a15bfccc`. **Status:** M3 remains OPEN until a reviewed checkpoint passes exact-commit Windows, Ubuntu and macOS hosted CI and a formal exit decision is recorded. This record concerns the local acceptance of the accumulated M3 changes; it is not a universal scientific equivalence or production-release claim.

## 1. Maintainer-observed Windows evidence

- Native Windows Release combined scientific/headless CTest after the final shared scripted `tn.Run()` integration: **155/155 distinct tests passed**, no failures. Labels overlap: **133 scientific**, **37 headless**, **14 equivalence**, and **37 smoke**.
- Real GUI versus true-headless **fixed-seed Flux Analysis**: exact saved 8x8 TSV grid match on both selected surface sides. Front file SHA-256 `c4c17c59003619c439ec448944ef063aae8a985b6571c5625d839c5bef483893`; back file SHA-256 `e475917410f3f229a76f4a7596e45ba11f3b2d8d6122d45a2040d3d77ca18386`.
- Native GUI seven-argument `tn.RunFluxAnalysis()` success: checked actual 8x8 export with **64/64 positive cells**, peak **1439.67 W/m2**. Invalid missing-directory output raised a controlled JS exception at its user script line rather than reporting success.
- Real native GUI scripted `tn.Run(0)` File-export append (two calls of 2,048 rays each) completed successfully. The canonical output matched the independent true-headless baseline: **2,272 physical photon records**, identical surface URL/side/coordinate multiset and exact stored power **0.0359832**, 1/1 surface mapping.
- In a fresh GUI Script Editor host, a scripted File export to a nonexistent directory failed at the user call site: `tn.Run: File export requires an existing absolute output directory.` The target path remained nonexistent (`Test-Path` returned `False`). Attempting an append after scene replacement in a previously used editor host was also correctly rejected; this is an independent, expected safety guard.
- Legacy script runtime audit: **8/8 additional standalone scripts passed**, plus the **2 CPC scripts** already covered by CTest, yielding **10/19** tracked examples covered by runtime execution. The remaining **9 scripts are deferred** because of preloaded-scene/data or larger-run requirements; compiling them or identifying their inputs does not count as runtime acceptance.
- `git diff --check` returned success in the integrated native workflow. Nonfatal LF/CRLF normalization warnings do not change accepted scientific results. The frozen **500-million-ray** simulation was not rerun.

These reports were supplied from the maintainer's Windows checkout; full native build logs and GUI session recordings are not part of this Git checkpoint record. The CTest and exact output checks are bounded to their specified fixtures, seeds, exporters and accepted comparison semantics.

## 2. Integrated architecture now under review

- GUI editor and true-headless CLI use the same `ScriptHost` and shared `ScriptSceneServices` domain API, with presentation-only actions remaining explicitly UI-specific.
- Both script hosts route domain `tn.Run()` through widget-free `ScriptRunExecutionService`, based on the shared `SimulationSession` plus `PhotonsBuffer` and File/no-export lifecycle. The former headless backend name remains a compatibility alias for existing failure-containment tests. Interactive GUI Run remains a GUI presentation/client workflow.
- Scripted `tn.RunFluxAnalysis()` uses a shared nonvisual calculation/export backend; the legacy seven-argument and explicit-seed eight-argument forms share the scientific path. Existing GUI seed/default policies are mapped at the adapter boundary.
- Shared output guards reject unsafe append after scene replacement, incompatible append settings and invalid paths; they preserve prior File export content across append/export failures according to the accepted tests.
- No optics, RNG algorithm, chunk scheduling, frozen benchmark values or established photon binary serialization were deliberately changed in this M3 integration.

## 3. Known scope limits and subsequent milestones

- **Scripted `tn.Run()`** currently implements bounded **File/No export** and a maximum of **200,000** rays for these script paths; other exporter plugins, larger runs and multi-file exporter configurations are M4 work. GUI scripted `tn.Run()` no longer opens a modal progress widget or draws rays as a side effect; the interactive GUI Run presentation remains unchanged.
- GUI-specific visual/presentation actions are not a headless scientific capability. Their unsupported-capability policy and the inactive, non-invokable legacy `ScriptRayTracer::Trace()` helper require clear documentation and final source review, but neither is accepted evidence of a second active domain ray tracer.
- Nine of nineteen legacy examples remain **unexecuted** in this runtime audit. Do not claim unrestricted legacy-script compatibility or unconditional GUI/headless parity.
- A separately compiled **QtWidgets/SoQt-free simulation/kernel target**, full plugin/exporter capability parity and final installed binaries remain **M4/M5** work. The fact that the current nonvisual services avoid QWidget APIs does not establish M5's enforced build boundary.
- The checkpoint must be reviewed for ownership, worker joins/cancellation, plugin lifetimes and compatibility policies. This document is a local acceptance record, **not** an architectural closure verdict or a cross-platform CI certificate.

## 4. Formal M3 exit decision gate

1. Verify exact accumulated source against the pinned M3 patch lineage and review the entire staged diff; commit only the explicitly enumerated source/tests and this record plus `PROJECT_STATUS.md` on `refactor/m3-script-api-exit`.
2. Open a **draft PR** against `master`, without merging, so the repository's existing three-platform CI (`build (ubuntu-24.04)`, `build (macos-latest)`, `build (windows-latest)`) runs against the **new** checkpoint commit. Review logs for unexpected skipped builds or regressions; the current workflow only does document-only CI for all-Markdown changes, whereas this PR includes C++/CMake/tests.
3. Review the resulting PR diff, runtime capability limitations and script-host/scene/worker/exporter lifetimes. Resolve failures or substantive review findings without altering frozen scientific references or running the 500-million-ray benchmark.
4. Only after all required checks and reviews pass, update `PROJECT_STATUS.md` to make an explicit **M3 CLOSED** decision for the documented scope. Do not imply M4, M5 or installable release completion; merge remains a separate authorized step.

## 5. Conventional commit message

`refactor(m3): consolidate GUI and headless script execution`

The body is retained in the accompanying checkpoint package's `COMMIT_MESSAGE.txt`, recording the scientific acceptance, compatibility limits, and outstanding three-platform CI gate.
