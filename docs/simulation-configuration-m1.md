# M1 — Centralized simulation configuration

**Status:** first development slice, not yet validated or merged.

## Configuration contract

`source/application/core/SimulationConfig.h` holds the scientific run inputs common to GUI tracing, GUI flux analysis, headless CLI tracing, headless scripts and headless benchmarks:

| Member | Default | Notes |
| --- | --- | --- |
| `rays` | 0 (invalid until set) | Existing positive-ray requirement |
| `masterSeed` | 0 | Existing deterministic per-chunk seed; 0 is valid |
| `sunWidthDivisions` | 200 | Existing default sun aperture sampling width |
| `sunHeightDivisions` | 200 | Existing default sun aperture sampling height |

`SimulationConfig::validate()` enforces positive ray count and positive sun-grid dimensions using historical preparation error strings. Production preparation now validates one shared value rather than repeating validation logic.

The refactor only changes the scientific preparation **input representation**. Existing workflow-specific defaults, user interfaces, CLI arguments, script argument rules, benchmark JSON parsing, photon export and buffering, flux receiver grid, ray physics, RNG streams and fixed chunk scheduler remain unchanged. In particular GUI still initializes with 10,000 rays, 200 × 200 grid, and its prior resolved-seed policy; headless CLI still requires explicit ray count and seed; headless scripts still default the omitted seed to zero; benchmark ray and seed defaults are retained.

The existing nine scientific regressions remain, including GUI/headless scientific parity and analytical sun aperture equality. Two new `ScientificSimulationConfig` tests cover default and validation contracts, and rejection through both real preparation entry points. Expected scientific CTest cases: **11**.

## Windows developer validation (pending)

From a PowerShell session in `C:\OpenSource\tonatiuhpp`:

```powershell
git fetch origin
git switch --track origin/refactor/central-simulation-config-m1-20261009
cmake --build "C:\OpenSource\tonatiuhpp\build" --config Release --target tonatiuhpp tonatiuhpp_scientific_equivalence_tests -j 16
ctest --test-dir "C:\OpenSource\tonatiuhpp\build" -C Release -R '^scientific\.' --output-on-failure
```

Optional representative CLI smoke check:

```powershell
$exe = "C:\OpenSource\tonatiuhpp\build\application\Release\tonatiuhpp.exe"
$scene = "C:\OpenSource\tonatiuhpp\examples\benchmarks\cylinder.tnhpp"
& $exe --headless trace-scene $scene --rays 4096 --seed 123456789 --no-export
Write-Host "Exit code: $LASTEXITCODE"
```

Review GitHub CI for Windows, Linux and macOS before moving PR from draft. Do not merge until local MSVC and CI verification, and numerical behavior review, are complete.

## Deferred configuration scope

This first slice does **not** redesign JSON configuration files, unify benchmark receiver flux grids with the sun aperture grid, migrate GUI photon exporter settings, introduce new CLI/script flags or change random seeding. These are separate product and compatibility decisions.
