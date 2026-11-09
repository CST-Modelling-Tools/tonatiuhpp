# M4.1 — One File-export service for direct CLI and scripts

**Base:** merged M3 `master` at `9476eefbb1e9b74533dc7e46872713a94729db72`. **Status:** source candidate; not yet compiled or scientifically accepted on the maintainer's Windows installation. **Milestone:** M3 is CLOSED for its bounded documented scope (PR #36 and post-merge master CI #42540600069 passed Windows/Ubuntu/macOS); M4 is OPEN.

## Implemented boundary

A single `source/application/core/PhotonExportFileService.h` provides:

1. File exporter discovery through the production `tonatiuh.PhotonsFactory` plugin ABI, without constructing widgets; lifetime ownership keeps the real plugin loader alive until after exporter destruction.
2. A strict File-export output preflight for GUI/headless script hosts and direct CLI: existing, absolute, non-symlink, **empty** output directory; unique temporary write/flush probe. No deletion or overwrite of existing data by these callers. If the preflight fails, the real exporter never starts.
3. The existing direct `--headless trace-scene ... --photon-export-dir DIR` command no longer requires `TONATIUHPP_A0_TRACE_SIGNATURE_FILE`. When the env var **is** present, legacy A0 signature instrumentation remains unchanged. Existing `--no-export` defaults and opt-in `--update-trackers` remain unchanged.
4. A repeatable direct CLI `--photon-surface URL` option for selected-surface recording. Both selection and File settings reach the same `HeadlessTracePreparationInput` and `PhotonsSettings` contracts used by scripting.
5. One native CTest comparing **two separate 2,048-ray direct CLI File exports** with **one separate 2,048-ray headless script** using the real plugin, same scene/seed/grid/tracker setting/surface. The established comparator checks canonical coordinate/side/surface-URL record multisets and photon power, not incidental record order or global photon IDs. Negative checks verify no overwrite, unrelated-file preservation, and missing-directory noncreation.

There is **no change** to optics, RNG, chunk scheduling, photon serialization, frozen fixtures, GUI interactive Run, legacy File export binary format, or the 500-million-ray reference evidence. The File output remains bounded to 200,000 requested rays for the direct CLI and script paths; testing larger exports is a later acceptance gate.

## Expected native CTest

- `headless.m4_cli_script_file_export_parity` — one new headless/scientific/equivalence test. It runs no 500M benchmark.
- The previously accepted scientific/headless tests, including real File-export failure/append and tracker regressions, must remain green.
- Expected total is **approximately 156 unique tests** versus the previously validated 155. CMake environment and optional Python detection can change the count; actual CTest result is authoritative.

## Compatibility notes

- The direct `--photon-export-dir` option is now a real, fail-closed File export command rather than an environment-dependent A0 diagnostic. Users must pre-create an empty absolute destination directory. Relative destinations, missing destinations, symbolic-link destinations and nonempty destinations are rejected instead of creating or clearing them.
- `--photon-surface URL` may be repeated with distinct URLs, but is invalid with `--no-export`. Existing calls with no selected surfaces remain supported.
- GUI script `tn.Run()` still uses its existing bounded File/No-export semantics; it now shares File plugin discovery and preflight with direct CLI. GUI defaults and GUI interactive export policy remain unchanged.
- The nonvisual helper loads the existing plugin through the **unchanged** factory interface. It does not remove `QMessageBox` or GUI plugin dependencies from the *legacy* File plugin; separating that UI responsibility remains an M4/M5 obligation, not an accomplished result here.
- Output-directory refusal is not a promise of atomic cross-process reservations; the checks protect existing contents at preflight time, but concurrent unrelated writers are outside the documented contract.

## Not closed by this change

M4 still requires broader exporter types/configuration, safely lifting bounded export limits, plugin write-error presentation separation, flush/append/interrupted-run acceptance for newly migrated modes, and direct interactive GUI/CLI/script equality under all agreed export policies. M5 still requires a library whose dependency graph **cannot** pull in Qt Widgets or SoQt, clean three-OS build-enforced headless linking, plugin packaging and installable release acceptance.

## Validation record

The actual native Windows build/CTest output, review SHA, and hosted CI result will be appended after acceptance. No native compilation or end-to-end photon comparison has been run in this authoring environment.
