# Architectural A0 — frozen 500-million-ray acceptance and completion review (PR #22)

**Status (2026-10-11):** PR #22 validation framework prepared; a **new full 500,000,000-ray run on the merged PR #21 baseline has not yet been demonstrated**. A0 scientific closeout is **PENDING**, not accepted by passing lightweight CI or by a synthetic auditor test.

## Scientific baseline and scope

- **Merged base:** PR #21 merge commit `44b32a8a`. Existing native Windows cylinder/one-axis GUI/headless tests passed, including ordered per-ray histories (20,001 rays, seed 123456789), with 10,914 / 36,650 matching interactions respectively. Existing selected Windows scientific regressions passed 28/28; CI passed Windows/Ubuntu/macOS on PR #21.
- **Frozen authentic scene:** `examples/benchmarks/benchmark_heliostat_field_v1.tnhpp`, 9,413,273 bytes, Zenodo MD5 `e42985c98b965f084a958226851ed626`. Do not edit it, normalize line endings or regenerate it.
- **Frozen v2 reference:** `examples/benchmarks/benchmark_reference_500M_v2.json`, 500,000,000 rays, seed 123456789, 50,000 stable chunks of 10,000 rays, target **front side 1**, bounds **[-2,2] x [-2,2]**, **100 x 100** grid. Reference binary-grid SHA-256: `36c7dc17cee8e7fdef820f08d4482e7c3dcdee3b5eed4748ea3949a6cf4af472`; total power **42.217052166502086 MW** and maximum flux **19.52519778288914 MW/m2**.
- **Benchmark name:** executable config remains `benchmark_v1`; reference label `benchmark_v2` identifies the post-RNG-refactor scientific reference, **not** another tracing algorithm. The reference does not override the established **0.1% total-power** and **5% peak-flux** benchmark tolerances. Exact frozen grid SHA-256 agreement remains a **separate mandatory condition**.
- `PROJECT_STATUS.md` records an earlier independent native Windows 500M reproduction of v2 before the latest A0 PRs. This is historical evidence, **not** proof of a new full run from the PR #21 merged master or PR #22 head.

## What PR #22 adds

The existing `tests/scientific/verify_reference_v2.py` already checks frozen reference JSON, the SHA-256 and byte length of its binary grid, exact CSV-to-binary values, min/mean/max metrics and authentic scene size/MD5. The existing `tests/cmake/run_heliostat_benchmark_m5.cmake` already offers explicit `MODE=full`: it launches the **real** headless executable for 500M rays and checks `benchmark_pass: true`, exact grid hash and emitted grid files. Neither action belongs to regular CI.

PR #22 makes this manual gate independently auditable:

1. For **MODE=full only**, save the native executable's captured `stdout_1.log` and `stderr_1.log` alongside the generated config, result JSON, binary grid and CSV. Do not change execution arguments or random streams.
2. Add `tests/scientific/audit_500m_benchmark.py`, a **post-run auditor** which first pins the original frozen reference/scene integrity, then requires actual fresh `m5_full_<10 hex>`-style output artifacts outside `examples/benchmarks`. It independently verifies the generated config and native log (including 500M traced rays and reference comparison), scene/reference paths, fixed identity/chunking, no photon export, structured pass flags, exact frozen binary hash and result hash, exact generated CSV/binary round-trip, grid min/mean/max/total-power consistency, and unchanged approved tolerances. It refuses missing/inconsistent files; it never launches ray tracing.
3. Add lightweight standard-library synthetic **auditor unit tests**, including corrupted/mismatched negative cases, to three-platform CI. **Their pass proves the auditor's behavior, not 500M physical execution**.
4. Preserve every frozen benchmark file byte and all production optics, RNG, scheduler, ray-history flags, photon exporter formats and automatic CI ray counts.

## Routine preflight (not the 500M run)

From a configured Windows Release checkout of the PR #22 branch:

```powershell
cd C:\OpenSource\tonatiuhpp
python tests/scientific/verify_reference_v2.py
python -m unittest discover -s tests/scientific -p test_audit_500m_benchmark.py -v
cmake --build build --config Release --parallel 16
ctest --test-dir build -C Release -R '^scientific[.]|^headless[.](validate_heliostat_scene|benchmark_heliostat_10k|benchmark_heliostat_1m_repeatability)$' --output-on-failure
```

These guard scene loading, unchanged scientific tests, medium within-platform reproducibility and reference-file integrity. **No 500M run has been performed just because this command succeeds.** The existing medium tests intentionally remove the inapplicable 500M reference comparison.

## Explicit full 500M Windows acceptance (manual; not CI)

Run only on a host equipped for the full native benchmark; **do not execute it accidentally in regular CTest/CI**. Use a new evidence folder. Keep the console transcript, exact git HEAD, clean working-tree status, executable SHA-256 and the newly generated output artifacts. Disable A0 per-ray history/hit diagnostics: they are bounded to 200,000 rays and are **not** designed for this benchmark.

```powershell
cd C:\OpenSource\tonatiuhpp
$exe = (Resolve-Path 'build/application/Release/tonatiuhpp.exe').Path
$root = (Get-Location).Path
$evidence = Join-Path $root ("build\a0-frozen-500m-" + (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $evidence -Force | Out-Null
Remove-Item Env:TONATIUHPP_A0_TRACE_SIGNATURE_FILE -ErrorAction SilentlyContinue
Remove-Item Env:TONATIUHPP_A0_RAY_HISTORY_FILE -ErrorAction SilentlyContinue

Start-Transcript -Path (Join-Path $evidence 'manual-console.log')
try {
    git rev-parse HEAD
    git status --short
    Get-FileHash $exe -Algorithm SHA256

    python tests/scientific/verify_reference_v2.py
    if ($LASTEXITCODE -ne 0) { throw 'Frozen reference integrity failed.' }

    $cmakeArgs = @(
        "-DTEST_EXECUTABLE=$exe",
        "-DTEST_WORKING_DIRECTORY=$(Split-Path $exe -Parent)",
        "-DSOURCE_CONFIG=$root\examples\benchmarks\benchmark_config_v2.example.json",
        "-DSCENE_FILE=$root\examples\benchmarks\benchmark_heliostat_field_v1.tnhpp",
        "-DOUTPUT_DIR=$evidence",
        "-DMODE=full",
        "-P", "tests/cmake/run_heliostat_benchmark_m5.cmake"
    )
    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) { throw 'Actual 500M benchmark failed.' }

    $run = Get-ChildItem -Path $evidence -Directory -Filter 'm5_full_*' |
        Select-Object -First 1
    if (-not $run) { throw 'No new 500M artifacts were generated.' }

    python tests/scientific/audit_500m_benchmark.py $run.FullName
    if ($LASTEXITCODE -ne 0) { throw 'Independent 500M artifact audit failed.' }

    Get-FileHash (Join-Path $run.FullName 'grid_1.bin') -Algorithm SHA256
    Write-Host "Accepted artifact location: $($run.FullName)"
}
finally {
    Stop-Transcript
}
```

The full runner writes `config_1.json`, `result_1.json`, `grid_1.bin`, `grid_1.csv`, `stdout_1.log` and `stderr_1.log` within a new `m5_full_*` directory. It must exit zero, report `500000000` traced rays, `benchmark_pass: true` and `flux_grid_hash_matches: true`. The independent Python audit must also **exit zero**. Preserve the complete logs and commit/executable provenance when reporting this acceptance to PR #22.

**Important limit:** A matching JSON/CSV/bin artifact alone cannot cryptographically prove it came from a fresh run of the claimed executable. Store the native log, commit identifier, executable SHA-256 and full command alongside the artifact audit. Do not report complete 500M acceptance if the native process failed, did not execute, or produced only copied reference files.

## A0 scientific closeout decision

| Gate | Evidence required | State at draft |
| --- | --- | --- |
| Original scene + frozen reference integrity | Pinned scene MD5/size, binary SHA-256 and CSV/JSON consistency | Existing CI check; rerun on PR #22 |
| Representative three-platform scientific checks | Prior PR #21 Windows 28/28 and Windows/Ubuntu/macOS CI; renewed PR #22 CI | Prior evidence passed; PR #22 CI pending |
| Native GUI/headless scientific equivalence | Cylinder/tracker hits, File exports, complete ordered histories | Passed for the stated fixtures on Windows in PRs #16–#21 |
| New full 500M actual execution | Independent run on PR #22 code/baseline, original scene and reference, fixed seed/chunks | **OPEN** |
| Post-run independent audit + provenance | `audit_500m_benchmark.py` success, native stdout/stderr, commit and executable hash | **OPEN** |
| Scientific A0 review | Explicitly review above evidence, scope limits and numerical-reference ownership | **OPEN** |

**Decision: BLOCKED / NOT YET CLOSED** until a fresh full 500M run and independent acceptance evidence are reviewed. Do not revise reference values, weaken tolerances or change optics to make a failing run pass. If a mismatch occurs, preserve outputs/logs and classify platform/toolchain or scientific causes. A0 closure, when justified, applies to the **defined fixtures and frozen benchmark contract**, not universal cross-platform bitwise equivalence, all tracker types, all media/materials, GUI Flux Analysis, installers or release qualification. Those remain separate scope and release gates.
