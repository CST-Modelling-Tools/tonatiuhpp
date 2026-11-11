# Tonatiuh++ v0.1.8.26 - Current capabilities

This Markdown file is kept because `.github/workflows/release.yml` uses `docs/release-notes-v${version}.md` when publishing a tagged version. It describes the current `v0.1.8.26` source capabilities; **no new public release has been authorized or announced**.

## Simulation and analysis

- One shared scientific ray-tracing kernel for the graphical application and nonvisual clients.
- Shared simulation preparation, execution, cancellation, and scripting services.
- Deterministic ray tracing with per-chunk `std::mt19937_64` streams, fixed 10,000-ray chunks, and reproducible seeded inputs on the verified fixtures.
- Interactive scene tracing, File photon export, script-driven runs, and graphical Flux Analysis.
- Native GUI and headless scientific equivalence checks, including fixed-seed photon content and ordered ray-history comparison.

## Headless operation

- GUI executable: `tonatiuhpp-gui` (Windows `.exe`), with no headless CLI dispatch.
- Independent QCoreApplication client: `tonatiuhpp-cli` (Windows `.exe`), using direct commands without `--headless` and no old executable aliases.
- Supported nonvisual commands: `--help`, `validate-scene`, `trace-scene`, `benchmark`, and `run-script`.
- Direct `trace-scene` supports no export or File photon export without an arbitrary total ray-count cap to an existing empty absolute directory, with optional surface and file settings.
- Shared JavaScript host with script-local paths, scene and domain services, and explicit GUI-only presentation boundaries.

See [Headless commands and benchmark](headless-benchmark.md) for current syntax and limitations.

## Scientific reference

The fixed 500-million-ray heliostat reference uses seed `123456789`, a 100 x 100 flux grid, total collected power `42.217052166502086 MW`, maximum flux `19.52519778288914 MW/m2`, and exact grid SHA-256 `36c7dc17cee8e7fdef820f08d4482e7c3dcdee3b5eed4748ea3949a6cf4af472`.

The final M5 Windows candidate matched the original unchanged reference and passed the independent artifact audit. The project's Windows, Linux, and macOS CI checks passed for the merged M5 pull request. No claim is made that all installer, deployment, or updater paths have been validated for a new public release.

The benchmark input scene, configuration, reference JSON, and binary grid remain in `examples/benchmarks/`.
