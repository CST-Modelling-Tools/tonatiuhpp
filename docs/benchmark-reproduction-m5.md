# M5 — Authentic heliostat benchmark reproduction

**Status:** M5 feature branch; CI and scientific review pending. This milestone validates the executable with real inputs, without changing production simulation code or reference values.

M4 preserved the authentic frozen heliostat scene and reference. M5 exercises that scene through the actual headless executable on the supported CI platforms.

## Automated scientific checks

| CTest | Platforms | Scope |
| --- | --- | --- |
| `headless.validate_heliostat_scene` | Windows, Ubuntu, macOS | Load authentic 1,971-heliostat scene in the real headless executable |
| `headless.benchmark_heliostat_10k` | Windows, Ubuntu, macOS | Execute 10,000 rays with seed 123456789 and original 100x100 receiver; inspect JSON, no-export logs, 80,000-byte binary grid and SHA-256 |
| `headless.benchmark_heliostat_1m_repeatability` | Windows, Ubuntu, macOS | Run **two separate 1,000,000-ray processes**, compare exact within-platform total/minimum/average/maximum power/flux metrics and flux-grid SHA-256 |

The shared `tests/cmake/run_heliostat_benchmark_m5.cmake` derives configurations from `examples/benchmarks/benchmark_config_v2.example.json`. It keeps the published receiver side, bounds, dimensions, seed and frozen scene unchanged. For **10K and 1M** runs it removes `reference_file`, because the approved **500M reference must not be compared to different ray counts**. All outputs use a fresh directory inside the CMake build tree, never the committed examples directory.

This checks **within-platform** reproducibility on all three operating systems. Different OS/compiler floating-point results may differ legitimately; CI does not demand cross-platform bit-for-bit identity.

On macOS, the uninstalled CMake app bundle runs from `build/application/TonatiuhPP.app/Contents/MacOS`. Plugin discovery also checks the matching CMake build-tree `build/plugins` directory, which contains the `MaterialSpecular` plugin required by the authentic scene.

## Local CTest commands (Windows, Ubuntu, macOS)

With a configured Release build and runtime dependencies available (as in CI):

```console
ctest --test-dir build -C Release -R "^headless[.](validate_heliostat_scene|benchmark_heliostat_10k|benchmark_heliostat_1m_repeatability)$" --output-on-failure
```

## Explicit 500-million-ray reference gate (NOT in CI)

To reproduce the official validated reference on a well-provisioned Windows machine, use an executable whose Qt and Tonatiuh++ DLL dependencies are discoverable:

```powershell
cd C:\OpenSource\tonatiuhpp
$exe = (Resolve-Path "build\application\Release\tonatiuhpp.exe").Path
$root = (Get-Location).Path
cmake "-DTEST_EXECUTABLE=$exe" `
      "-DTEST_WORKING_DIRECTORY=$(Split-Path $exe -Parent)" `
      "-DSOURCE_CONFIG=$root\examples\benchmarks\benchmark_config_v2.example.json" `
      "-DSCENE_FILE=$root\examples\benchmarks\benchmark_heliostat_field_v1.tnhpp" `
      "-DOUTPUT_DIR=$root\build\m5-full-reference" `
      "-DMODE=full" `
      -P tests\cmake\run_heliostat_benchmark_m5.cmake
```

`MODE=full` uses the *unmodified* `benchmark_reference_500M_v2.json`, runs 500,000,000 rays and requires `benchmark_pass: true` plus an exact reference flux-grid SHA-256 match. It may take many minutes and is not suited to normal CI. Results will be in the fresh `build/m5-full-reference/m5_full_...` directory. If the comparison fails, investigate platform/toolchain and physical-configuration differences; **never change the published reference just to obtain a pass**.

The build-tree executable may require the project dependency directories (for example `build/libraries`, `build/kernel`, `build/SunPath` and `third_party/_install/bin`) on `PATH`. An installed Release executable with correctly packaged dependencies is preferable for release validation.

**Not covered:** native GUI scene load or Flux Analysis, installer acceptance, true 500M reproduction until the explicit command is executed, or strict bitwise comparison between different operating systems.
