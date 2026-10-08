# M2 — Cross-platform headless validation

**Status:** development branch; Windows/MSVC and CI verification pending. Do not merge without scientific review.

## Scope

Previously, the Windows CMake configuration did not register headless smoke tests unless `TONATIUHPP_TEST_EXECUTABLE` pointed to a separately installed executable. The default is now the build-tree `$<TARGET_FILE:tonatiuhpp>` on all three platforms, with `$<TARGET_FILE_DIR:tonatiuhpp>` as the working directory. This supports MSVC multi-config Release output while preserving the explicit installed-runtime override.

The new `headless.benchmark_repeatability` case starts two independent processes using the cylinder scene, benchmark v1 configuration, 20,001 rays, seed 123456789, and an 8 × 8 receiver flux grid. It compares exactly the reported total power, minimum/average/maximum flux, and `flux_grid_sha256`. Time and worker metadata are excluded. This exercises three fixed 10,000-ray chunks, the last containing one ray, without modifying existing reference files.

CI now checks headless smoke-test discovery and requires Windows job success. The 11 existing scientific regression tests are unchanged. No production ray physics, RNG derivation, sun sizing, photon export, energy normalization, or benchmark formulas are modified.

**Limit:** This test proves repeatability on each platform separately; it is not proof of identical cross-platform hashes, nor of full GUI/export equivalence or historical 500-million-ray benchmark-v2 agreement.

## Windows/MSVC Release validation

Open PowerShell in `C:\OpenSource\tonatiuhpp`:

```powershell
git fetch origin
git switch --track origin/test/cross-platform-headless-validation-m2-20261009
cmake --build "C:\OpenSource\tonatiuhpp\build" --config Release --target tonatiuhpp tonatiuhpp_scientific_equivalence_tests -j 16
ctest --test-dir "C:\OpenSource\tonatiuhpp\build" -C Release -R '^(scientific[.]|headless[.])' --output-on-failure
```

Expected test discovery: **11 scientific + 10 headless = 21 tests**. Do not merge on test failure; collect the complete output, especially for missing Windows DLLs, and investigate.

## Review gates

Require passing Windows/MSVC Release tests, three-platform CI (Windows, Ubuntu, macOS) and author scientific review before merging. Full reference-v2 comparison, GUI photon-file/flux export comparisons, packaging, and updater checks remain release tasks.
