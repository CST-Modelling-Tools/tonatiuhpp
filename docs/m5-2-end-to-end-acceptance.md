# M5.2 — consolidated end-to-end acceptance (Windows native accepted; three-platform CI passed; pre-PR)

**Roadmap:** `docs/gui-core-separation-scientific-equivalence-plan.md`, M5.1–M5.2.  
**Branch:** `refactor/m5-compiled-service-boundaries` from `626ef4d66e019d0c9528b5b6cb7a52e11565de51`.  
**Merge strategy:** Preserve the published M5 branch and open ONE final M5 PR only after the final correction review and CI rerun. The integration commit is already published. Do not rerun the frozen 500-million-ray benchmark.

## Accepted foundation and native acceptance

M5.1 was accepted on 2026-11-09 with **166/166 Windows Release CTests**. The GUI, the GUI binary's QCoreApplication path and the standalone QCoreApplication executable link the same `TonatiuhKernel`, `TonatiuhLibraries`, `TonatiuhSimulation`, `TonatiuhScripting` and `TonatiuhScriptRuntime`. Official scientific scene and File-export plugins share that kernel. Sampled Windows nonvisual imports excluded Widgets, SoQt and PrintSupport; QtGui is intentionally retained for scientific/factory compatibility.

M5.2 subsequently passed **167/167 native Windows Release CTests**, the temporary staged-install adapter acceptance, and the real Windows Qt GUI presenter acceptance. The presenter selected File export in the actual GUI dialog and traced rays, producing **3,184 canonical photons** with exported power **0.0719665** and identical GUI/standalone native hit and ordered-history signatures. The original M3.3 script produced **identical 17-digit interception power** in the GUI Script Editor and standalone at seed zero. The real Qt Flux Analysis dialog plotted a positive **8x8** map, saved PNG evidence, and differed from the fixed-seed script by a maximum absolute value **0.00464174** (a bounded comparison, not equality).

The maintainer then committed the M5 changes as [`1ee7def`](https://github.com/CST-Modelling-Tools/tonatiuhpp/commit/1ee7def2181ed907e48db5381e32308b4a9a8821), and reports that the manually dispatched GitHub CI passed on **Windows, Ubuntu and macOS**. The three-platform claim applies to the configured build-tree tests, including `headless.m5_2_cross_adapter_end_to_end`, and not to unperformed interactive Linux/macOS GUI or installed-runtime tests. CI run URLs were not supplied for independent log review.

The maintainer confirmed **there are no legacy third-party plugins**. Compatibility with hypothetical old binary-only third-party plugins is therefore not an M5 acceptance requirement. Existing first-party plugins remain in scope and were exercised.

## M5.2 implementation in this package

1. Install the independent `tonatiuhpp_m5_core_headless` executable alongside the existing GUI, exactly ONE physics kernel and the original official scientific plugin binaries. Mac app bundle plugin discovery also checks the installed `<prefix>/bin` location without changing the GUI plugin ABI or scientific code.
2. Register `headless.m5_2_cross_adapter_end_to_end` with CTest (Python3 required when testing). It runs two **distinct** executables: the original GUI binary in its preexisting `--headless` `QCoreApplication` branch and the independent standalone `QCoreApplication` binary. This is **not** an automated interactive MainWindow trace or GUI Script Editor test.
3. Run exact 2,048-ray, seed `123456789` paired scene traces with A0 hit and *ordered per-ray history* digests on cylinder, two-reflection specular, one-axis tracked scene, and the real external MaterialSpecular plugin sphere. Required effective input fields, nonzero signal and scientific SHA-256 histories are compared within the same OS.
4. Run the identical **unmodified M3** bounded JavaScript fixtures through both processes: scene/field edits, `NodeObject`, interception and `DataObject` CSV; nontrivial tracker/sun response; front/back 8x8 Flux Analysis. Compare script-relative native artifacts with exact normalized-newline bytes. Exercise four cancellation/recovery and fail-closed resource/error cases in each process.
5. Run the existing bounded **M4** 2,048-ray File script and direct trace through each executable (four runs); compare canonical photon multisets, power and format using the existing `tests/scientific/compare_photon_exports.py` implementation, not unstable parallel ordering. Record SHA-256 for every File artifact; reject overwrite attempts without touching an existing sentinel.
6. Reuse the **full scientific/headless CTest suite exactly once** after incremental compile (target total 167 unique tests if the new M5.2 test is registered). Save the bounded acceptance result in a unique `build/tests/m5_2_evidence/` run folder. The M5.2 test is part of the normal three-platform CTest graph and is covered by the maintainer-reported passed manual GitHub CI.
7. On Windows, stage `cmake --install` to a unique temporary root by temporarily reconfiguring `CMAKE_INSTALL_PREFIX` (the existing install scripts capture this value during configuration); always restore the original cache prefix. Run a reduced but meaningful repeat of both binary paths in the installed tree with a restricted PATH excluding build/Qt/third-party development DLLs. Preserve staged evidence for inspection; do not touch an actual installation.

**Source and fixture protection:** no scientific algorithm, RNG, optical-material code, scene file, benchmark reference, existing M3/M4 test, or production photon format is modified. The test uses new isolated directories and does not remove preexisting user data. A failing acceptance retains its evidence and does not commit, push, reset, or reapply code.

## Accepted scope and remaining limitations

| Gate | Outcome |
| --- | --- |
| Single nonvisual kernel, services and first-party plugins | Windows native checks passed; the architecture is built on all three CI operating systems. |
| Seeded GUI-binary QCore versus standalone QCore traces, scripts, Flux, File export and cancellation | M5.2 bounded Windows acceptance passed; the same registered test belongs to the three-platform CI graph, reported passed. |
| Windows temporary installed runtime | Passed for GUI/headless scientific adapter paths. |
| Windows real GUI File dialog, Run, GUI Script Editor and Qt Flux plot | Passed in the final native presenter harness with retained JSON, photon and PNG evidence. |
| Three-platform hosted build and CTests | Windows, Ubuntu and macOS reported passed by the maintainer after manual dispatch on the published branch. |
| Hypothetical legacy third-party binary compatibility | Not applicable: the maintainer confirms there are no such plugins. |
| Nine previously uncovered legacy sample scripts | Not claimed covered; retain as a documented sample-corpus gap unless classified/tested separately. |
| Linux/macOS interactive GUI and installed-runtime deployment | Not established by the ordinary build-tree CI; release packaging remains a separate validation gate. |
| Single final M5 PR | Not opened as of this review. |

### Production build isolation

The native Qt presenter driver is compiled only while `BUILD_TESTING=ON` and is inert without the explicit test environment variables. The repository's Release workflow previously left CMake's testing default enabled. The final-review correction makes its Windows, Linux and macOS Release configurations specify `BUILD_TESTING=OFF` so the presenter driver and other test-only targets are not compiled into distributed builds. Ordinary CI remains testing-enabled and continues to execute CTests.

### Scientific and compatibility boundaries

The fixed-seed comparisons establish parity within each tested operating system; no claim is made that every floating-point digest must be bit-identical across operating systems. File comparisons use canonical physical photon records rather than nondeterministic photon IDs. QtGui remains intentionally used by nonvisual components for factory and geometry types, while Widgets, SoQt and PrintSupport are excluded from the documented nonvisual linkage. No frozen 500-million-ray benchmark was rerun, and M5 acceptance alone is not proof of release packaging or updater readiness.

## Scientific and compatibility constraints

- Preserve within-platform exact ray-hit and ordered-history checks. Across different operating systems compare the accepted scientific quantities within documented tolerances; do **not** demand cross-OS byte identity for floating-point hit signatures.
- File exporter canonical comparison deliberately ignores nondeterministic global photon IDs / write order but preserves exact position coordinates, front/back, stable surface URL and photon power.
- Qt GUI presentation remains outside the kernel link graph. Retain `QIcon`/`QtGui` compatibility where historically exposed; do not reintroduce Qt Widgets or SoQt in kernel, standalone or first-party nonvisual plugin binaries.
- Headless plugin metadata `TonatiuhppHeadlessABI=1` is a compatibility declaration, not a security boundary. Do not load untrusted native plugins or claim old binary ABI stability.
- No frozen benchmark run, no benchmark reference update and no release-readiness claim.

## Published integration and final review

- Published branch: `refactor/m5-compiled-service-boundaries`.
- M5 integration commit: [`1ee7def2181ed907e48db5381e32308b4a9a8821`](https://github.com/CST-Modelling-Tools/tonatiuhpp/commit/1ee7def2181ed907e48db5381e32308b4a9a8821).
- Final-review follow-up: correct Release-only testing isolation, derive standalone version metadata from `APP_VERSION`, update governing documentation, then manually rerun Windows/Ubuntu/macOS CI before opening the one final M5 PR.
- No automatic commit, push, PR, merge, reset or frozen benchmark execution is part of this correction package.
