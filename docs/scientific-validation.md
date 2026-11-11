# Scientific validation of the current codebase

## What must remain stable

Scientific refactoring must preserve the accepted scene interpretation, sun and tracker preparation, ray propagation, random-number streams, photon records, cancellation behavior, and flux/power normalization unless a separately approved physics change explicitly changes the reference.

The frozen heliostat scene, the 500-million-ray v2 reference JSON, and its binary/CSV grids live under `examples/benchmarks/`. **Do not edit or regenerate them during a code or documentation cleanup.** Keep uncommitted benchmark outputs and console transcripts in the ignored local build tree.

## Routine checks

With a configured build and the necessary native dependencies available:

```powershell
python tests/scientific/verify_reference_v2.py
python -m unittest discover -s tests/scientific -p test_audit_500m_benchmark.py -v
ctest --test-dir build -C Release --output-on-failure
```

These checks validate the frozen reference integrity, the audit tool's error handling, and the configured CTest suite (including bounded scientific/headless comparisons). They **do not** perform a new 500-million-ray simulation. On Linux/macOS single-config builds, omit `-C Release` if unnecessary.

The source contains more targeted tests for canonical photon exports, fixed-seed scene hits, ordered per-ray histories, script data, flux analysis, tracker behavior, and independent CLI-process repeatability and GUI-style scientific checks. The active test definitions are in `tests/CMakeLists.txt` and `tests/scientific/CMakeLists.txt`.

## Accepted full-scale flux reference

The current M5 merge candidate was tested with an explicit Windows Release run of **500,000,000 rays**, seed **123456789**, **50,000 chunks of 10,000 rays**, and a **100 x 100** front-side receiver flux grid bounded by `[-2, 2] x [-2, 2]`.

| Measure | Expected result |
| --- | --- |
| Total collected power | `42.217052166502086 MW` |
| Maximum flux | `19.52519778288914 MW/m2` |
| Exact binary grid SHA-256 | `36c7dc17cee8e7fdef820f08d4482e7c3dcdee3b5eed4748ea3949a6cf4af472` |

On 2026-11-10, the Windows M5 Release executable with SHA-256 `8482dad2cfea7623850c6a269a81a1539be14e8548273c72c2fe9f9dee16e278` produced the **exact frozen grid**, accepted power and peak flux, and passed `tests/scientific/audit_500m_benchmark.py`. The M5 code was merged into `master` in [PR #39](https://github.com/CST-Modelling-Tools/tonatiuhpp/pull/39), merge commit `8c6fa58f8038e9b44e5592cf4ab7658a721251d7`. This accepted evidence is sufficient for the documented M5 scientific scope; it is not a claim of release/installer readiness or of universal physical correctness.

## Optional full 500-million-ray manual verification

**Historical opt-in procedure only: do not execute it as part of this refactoring. Use only by a separate deliberate scientific acceptance decision, never as a routine CI step.** The test is computationally expensive. From the Windows repository root, first build the Release executable, ensure a clean source revision, and check that the frozen reference is intact.

```powershell
$root = (Get-Location).Path
$exe = (Resolve-Path 'build/bin/Release/tonatiuhpp-cli.exe').Path
$evidence = Join-Path $root ('build/a0-full-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $evidence -Force | Out-Null

Start-Transcript -Path (Join-Path $evidence 'console.log')
try {
    python tests/scientific/verify_reference_v2.py
    if ($LASTEXITCODE -ne 0) { throw 'Frozen reference integrity failed.' }

    Remove-Item Env:TONATIUHPP_A0_TRACE_SIGNATURE_FILE -ErrorAction SilentlyContinue
    Remove-Item Env:TONATIUHPP_A0_RAY_HISTORY_FILE -ErrorAction SilentlyContinue

    $cmakeArgs = @(
        "-DTEST_EXECUTABLE=$exe",
        "-DTEST_WORKING_DIRECTORY=$(Split-Path $exe -Parent)",
        "-DSOURCE_CONFIG=$root/examples/benchmarks/benchmark_config_v2.example.json",
        "-DSCENE_FILE=$root/examples/benchmarks/benchmark_heliostat_field_v1.tnhpp",
        "-DOUTPUT_DIR=$evidence",
        '-DMODE=full', '-P', 'tests/cmake/run_heliostat_benchmark_m5.cmake'
    )
    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) { throw 'Full benchmark failed.' }

    $run = @(Get-ChildItem $evidence -Directory -Filter 'm5_full_*')
    if ($run.Count -ne 1) { throw 'Expected exactly one benchmark run.' }
    python tests/scientific/audit_500m_benchmark.py $run[0].FullName
    if ($LASTEXITCODE -ne 0) { throw 'Independent audit failed.' }

    Get-FileHash (Join-Path $run[0].FullName 'grid_1.bin') -Algorithm SHA256
}
finally {
    Stop-Transcript
}
```

Preserve the result JSON, binary/CSV grid, native stdout/stderr logs, benchmark configuration, exact commit identifier, executable SHA-256, and console transcript. The post-run auditor verifies the numerical files but cannot, by itself, prove that a particular binary actually executed. Do not substitute copied reference artifacts for a real run.

## Boundaries

The accepted Windows full-scale run and hosted Windows/Linux/macOS CI establish behavior for the tested scientific fixtures and platform builds. They do not replace interactive GUI tests, installed-runtime deployment checks on every platform, or future feature-specific regression coverage. A documentation-only change does not justify rerunning the full benchmark.
