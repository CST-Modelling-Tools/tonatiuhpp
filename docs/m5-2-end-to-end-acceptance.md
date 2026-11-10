# M5.2 — consolidated end-to-end acceptance (source candidate; native validation pending)

**Roadmap:** `docs/gui-core-separation-scientific-equivalence-plan.md`, M5.1–M5.2.  
**Branch:** `refactor/m5-compiled-service-boundaries` from `626ef4d66e019d0c9528b5b6cb7a52e11565de51`.  
**Merge strategy:** ONE final M5 pull request only after complete milestone acceptance. Do not commit/push/open a PR during local integration. Do not rerun the frozen 500-million-ray benchmark.

## Accepted foundation (do not misstate as M5.2 success)

2026-11-09 Windows MSVC Release **M5.1 ONE-kernel 166/166 CTests passed**. The two QCore entrypoints and the GUI link the SAME `TonatiuhKernel`, `TonatiuhLibraries`, `TonatiuhSimulation`, `TonatiuhScripting`, and `TonatiuhScriptRuntime`. The File exporter and official scene plugins have ONE scientific implementation. The existing four M5 build/link/plugin smoke tests, seeded non-export and exact photon-file tests, and Windows sampled nonvisual PE-import exclusions passed. Legacy prebuilt third-party widget-plugin ABI compatibility was NOT certified.

## M5.2 implementation in this package

1. Install the independent `tonatiuhpp_m5_core_headless` executable alongside the existing GUI, exactly ONE physics kernel and the original official scientific plugin binaries. Mac app bundle plugin discovery also checks the installed `<prefix>/bin` location without changing the GUI plugin ABI or scientific code.
2. Register `headless.m5_2_cross_adapter_end_to_end` with CTest (Python3 required when testing). It runs two **distinct** executables: the original GUI binary in its preexisting `--headless` `QCoreApplication` branch and the independent standalone `QCoreApplication` binary. This is **not** an automated interactive MainWindow trace or GUI Script Editor test.
3. Run exact 2,048-ray, seed `123456789` paired scene traces with A0 hit and *ordered per-ray history* digests on cylinder, two-reflection specular, one-axis tracked scene, and the real external MaterialSpecular plugin sphere. Required effective input fields, nonzero signal and scientific SHA-256 histories are compared within the same OS.
4. Run the identical **unmodified M3** bounded JavaScript fixtures through both processes: scene/field edits, `NodeObject`, interception and `DataObject` CSV; nontrivial tracker/sun response; front/back 8x8 Flux Analysis. Compare script-relative native artifacts with exact normalized-newline bytes. Exercise four cancellation/recovery and fail-closed resource/error cases in each process.
5. Run the existing bounded **M4** 2,048-ray File script and direct trace through each executable (four runs); compare canonical photon multisets, power and format using the existing `tests/scientific/compare_photon_exports.py` implementation, not unstable parallel ordering. Record SHA-256 for every File artifact; reject overwrite attempts without touching an existing sentinel.
6. Reuse the **full scientific/headless CTest suite exactly once** after incremental compile (target total 167 unique tests if the new M5.2 test is registered). Save the bounded acceptance result in a unique `build/tests/m5_2_evidence/` run folder. The M5.2 test is part of the normal three-platform CTest graph whenever native CI is finally run.
7. On Windows, stage `cmake --install` to a unique temporary root by temporarily reconfiguring `CMAKE_INSTALL_PREFIX` (the existing install scripts capture this value during configuration); always restore the original cache prefix. Run a reduced but meaningful repeat of both binary paths in the installed tree with a restricted PATH excluding build/Qt/third-party development DLLs. Preserve staged evidence for inspection; do not touch an actual installation.

**Source and fixture protection:** no scientific algorithm, RNG, optical-material code, scene file, benchmark reference, existing M3/M4 test, or production photon format is modified. The test uses new isolated directories and does not remove preexisting user data. A failing acceptance retains its evidence and does not commit, push, reset, or reapply code.

## Acceptance matrix and remaining external gates

| Gate | Evidence / expectation | Status before native run |
| --- | --- | --- |
| One nonvisual kernel and first-party plugins | M5.1 build/link smoke and guard | Windows M5.1 passed 166/166 |
| GUI-binary QCore ↔ independent QCore native signatures | `headless.m5_2_cross_adapter_end_to_end`, full A0 per-ray digests | Pending M5.2 local |
| Matched bounded domain JavaScript, tracker, Flux and cancellation | Same two process outputs, exact artifact data | Pending M5.2 local |
| Direct CLI ↔ script File exporter and overwrite refusal | Four-path physical photon/power comparator | Pending M5.2 local |
| Windows temporary staged install | Installed both executables; no development-library PATH fallback | Pending |
| Windows interactive GUI, Script Editor and actual GUI Run/Flux Analysis | Manually run the same fixed-seed GUI fixtures; compare their artifacts to existing accepted headless baselines | **Not tested by this Python harness** |
| All 19 tracked examples and declared presentation-only API exceptions | M3 previously covered 10; remaining nine require bounded review and/or capability documentation | Open |
| Windows, Ubuntu, macOS clean build/CTests/installed runtime | Native tests on actual OS; no hosted CI until the final M5 PR | Open |
| Legacy binary-only third-party plugins | Owner decision: explicitly require migration/rebuild or provide compatibility adapter and test actual old binaries | Open |
| One final M5 PR, review and CI | Only after all agreed local acceptance is complete | Not started |

### GUI-interactive acceptance (later, not replaced by CLI tests)

Use a disposable project and the same fixed seeds/inputs established by M0–M4. Verify ordinary GUI startup and shutdown; open a plugin-based scene (`examples/benchmarks/sphere.tnhpp`, using MaterialSpecular); run a short interactive ray trace with File export and Flux Analysis; run `tests/scripts/m3_flux_shared_parity.tnhpps` and `tests/scripts/m3_3_gui_headless_parity.tnhpps` in the actual GUI Script Editor and compare their exported numeric artifacts with headless evidence, keeping their output directories distinct. These scripts **clear the active scene**; save work first. Verify cancellation, plugin menu / File widget presence and GUI presenter behavior. This is an explicit separate native acceptance check.

### Multi-platform and packaging follow-up

The added CTest test is platform-independent Python standard library plus existing native executables. After the M5 scope is technically complete, run it (and the entire relevant CTest suite) on Windows, Ubuntu and macOS; verify installed executable resolution, Qt/Coin runtime deployment, shared plugin discovery and GUI startup. The current source package author cannot run any native Qt/Coin/SoQt binary in the container: **no cross-platform or installed-runtime success is claimed before the user reports it.**

## Scientific and compatibility constraints

- Preserve within-platform exact ray-hit and ordered-history checks. Across different operating systems compare the accepted scientific quantities within documented tolerances; do **not** demand cross-OS byte identity for floating-point hit signatures.
- File exporter canonical comparison deliberately ignores nondeterministic global photon IDs / write order but preserves exact position coordinates, front/back, stable surface URL and photon power.
- Qt GUI presentation remains outside the kernel link graph. Retain `QIcon`/`QtGui` compatibility where historically exposed; do not reintroduce Qt Widgets or SoQt in kernel, standalone or first-party nonvisual plugin binaries.
- Headless plugin metadata `TonatiuhppHeadlessABI=1` is a compatibility declaration, not a security boundary. Do not load untrusted native plugins or claim old binary ABI stability.
- No frozen benchmark run, no benchmark reference update and no release-readiness claim.

## Proposed eventual conventional commit (DO NOT EXECUTE yet)

`refactor(core): complete single-kernel M5 separation and cross-interface acceptance`

Consolidate GUI-independent simulation and scripting into one shared scientific kernel and one set of plugin implementations. Install and validate a standalone QCoreApplication client alongside GUI operation. Enforce forbidden GUI dependency closure and exercise bounded fixed-seed hit/history, Flux Analysis, interception, File-export, failure, cancellation, and plugin parity across supported platforms. Record staged installation and legacy plugin migration evidence without modifying scientific algorithms, immutable benchmark fixtures, or ray-propagation semantics.
