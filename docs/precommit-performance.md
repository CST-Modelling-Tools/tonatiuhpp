# Tonatiuh++ pre-commit performance gate

This is a **developer-invoked, no-commit** benchmark harness. It does not modify frozen scientific benchmark reference files. Its scientific-equality check compares the baseline build to the candidate build at a **reduced ray count**. That check is useful, but does not replace the authoritative 500-million-ray scientific reference.

## Updating an installed v1 harness (Windows)

Use the `TonatiuhPP-Precommit-Performance-v2.zip` release directly from Downloads; do **not** manually extract or move its contents. A PowerShell bootstrap command can extract it temporarily and invoke the included guarded `Update-TonatiuhPP-Performance.ps1` installer. The updater:

- Only changes `scripts/performance/Compare-TonatiuhPP-Performance.ps1`, `scripts/performance/compare_working_tree.py` and `docs/precommit-performance.md`.
- Verifies that existing files match the published v1 SHA-256 values, so locally modified scripts are not silently overwritten.
- Backs up the three originals outside the repository before replacing them; retains the baseline helper untouched.
- Does not change builds, scientific source, benchmark references, Git index or commits.

Python 3.10+ and Windows PowerShell 5.1+ are supported. No pip packages are required. The PowerShell wrapper prefers the checkout's `.venv` Python.

## Prepare a baseline without committing anything

From `C:\OpenSource\tonatiuhpp`, create an isolated clean worktree:

```powershell
.\scripts\performance\New-PerformanceBaseline.ps1 -Repo 'C:\OpenSource\tonatiuhpp'
```

It creates `C:\Users\<you>\TonatiuhPP-Performance-Baselines\<HEAD-short-sha>`; it refuses to overwrite an existing directory. It does **not** delete, reset, stash, stage, or commit your changes. If submodules are used by your build, initialize them in the baseline worktree using the project's documented process.

Build the **clean HEAD baseline** and the **uncommitted candidate** in separate Release build folders with exactly the same compiler, Qt, third-party libraries, CMake configuration, optimization flags, architecture and environment. Follow the repository's normal build instructions. Supply the paths of the two resulting **standalone CLI executables**. They must have runtime DLLs and plugins accessible from their respective build/runtime folders.

Changing the harness requires no C++ rebuild. It can be tested separately with mock benchmark executables.

## Run the cross-over comparison

The baseline and candidate scenes must have identical SHA-256. The canonical benchmark scene may be used as a **read-only input**, but the harness creates independent reduced-ray configurations without `reference_file`, and stores outputs under fresh directories outside the installations.

```powershell
.\scripts\performance\Compare-TonatiuhPP-Performance.ps1 `
  -Repo 'C:\OpenSource\tonatiuhpp' `
  -BaselineExe 'C:\PATH\TO\BASELINE\bin\tonatiuhpp-cli.exe' `
  -CandidateExe 'C:\PATH\TO\CANDIDATE\bin\tonatiuhpp-cli.exe' `
  -BaselineScene 'C:\PATH\TO\BASELINE\examples\benchmarks\benchmark_heliostat_field_v1.tnhpp' `
  -CandidateScene 'C:\PATH\TO\CANDIDATE\examples\benchmarks\benchmark_heliostat_field_v1.tnhpp' `
  -Rays 5000000 -AbbaBlocks 2 -WarmupPairs 2 `
  -StabilityLimitPercent 3.0 -AgreementLimitPercentagePoints 2.0
```

**v2 defaults:** two baseline/candidate warm-up pairs (four warm-up runs excluded from performance statistics), then two measured ABBA blocks (eight measured runs). Use `-WarmupPairs 0` for a rapid functional smoke test only; no performance conclusion should be drawn without warming up. The legacy `-Warmup` option is retained but unnecessary with v2 defaults. Adjust `-WarmupPairs`, `-StabilityLimitPercent` and `-AgreementLimitPercentagePoints` as needed after calibration.

Warm-up executions are still subject to scientific validation, and will appear in logs and `runs.csv`, but are excluded from statistical summaries. Warm-up does **not** guarantee thermal stability; drift checks remain mandatory for a meaningful verdict. The harness cannot measure temperature, CPU effective clocks or package power automatically; collect such telemetry separately when available.

For older `tonatiuhpp.exe` builds (such as v0.1.8.26), add `-BaselineLegacy` to invoke `tonatiuhpp.exe --headless benchmark`. Both builds' binary paths and scene paths must be supplied explicitly.

## Reporting and verdicts

Each unique output directory contains `metadata.json`, `report.json`, `report.md`, `runs.csv`, and a folder for each run containing `config.json`, `result.json`, `console.log`.

- Scientific fields must match **exactly**, including the `flux_grid_sha256`, for this same-machine same-toolchain baseline/candidate test. Exact same-machine comparison is deliberately stricter than general cross-platform tolerances.
- Each execution must have exit code 0, expected rays and seed, `rays_traced == rays`, and 10,000-ray chunks.
- All measured executions must use the same worker count.
- **Primary decision metric:** median of the per-ABBA-block time changes. A block's effect is `(mean(candidate_pair) / mean(baseline_endpoints) - 1) * 100`.
- The previous overall baseline/candidate median comparison is retained as an auxiliary metric to facilitate comparison with v1 reports. With strong drift it can disagree sharply with the paired metric.
- **Stability checks:** absolute time change between baseline endpoints of each ABBA block; absolute change between the two candidate timings inside each block; variation in baseline and candidate block means across blocks; and the spread in block effects. Default drift/spread limit is 3%, and maximum difference between block effects is 2 percentage points.
- If any stability check fails, return `INCONCLUSIVE / UNSTABLE ENVIRONMENT`, regardless of the measured speed difference. A single ABBA block is also inconclusive for performance acceptance.
- If checks pass, effects outside the tentative 2% performance threshold yield **potential** regression/improvement verdicts requiring confirmation. A result within the threshold is **not proof of equivalence**.
- A potential regression returns exit code `3`; scientific or execution failure returns `2`; an inconclusive result returns `0` but must not be interpreted as acceptance. Inspect the report verdict, never just the process exit code.
- `report.md` and `report.json` do not replace reviewing all individual measurements and independent hardware telemetry.

**Limitations**: the benchmark's `elapsed_seconds` includes preparation and the ray loop, but not subsequent CSV/binary output or scientific reference checking. This is ideal for comparing that metric consistently, but a microbenchmark or separate trace-only timer is needed to isolate per-ray history-hook costs. Do not automatically reject a scientific change because its exact reduced-ray flux-grid hash differs; investigate whether the intended physics changed, and separately apply the project's numerical tolerances. These heuristic checks are not formal confidence intervals, statistical hypothesis tests or proof of software causation. Confirm any meaningful result with independent runs and hardware telemetry.

## Safety

- No Git commit, stage, reset, clean, or ref update is performed.
- No existing run directory is overwritten; every output root must be new.
- No frozen 500M reference is read as an expected value for smaller tests, let alone rewritten.
- Baseline and candidate scene input bytes must match.
- Historical release installer comparisons are less controlled than two locally built versions using the same compiler and dependencies.
- Use a separate hardware monitor to inspect CPU effective clocks, temperature, power limits and throttling; the harness does not claim to infer these from timing.
