# Tonatiuh++ v0.1.8.20 Release Notes Draft

Tonatiuh++ v0.1.8.20 adds the first release-ready headless execution and benchmark workflow inside the existing `tonatiuhpp` executable.

## Highlights

- Added `tonatiuhpp --headless validate-scene <scene.tnhpp>` for non-GUI scene-load validation.
- Added `tonatiuhpp --headless trace-scene <scene.tnhpp> --rays N --seed S --no-export` for deterministic no-export ray tracing.
- Added `tonatiuhpp --headless benchmark <benchmark_config.json>` for benchmark v1 runs with JSON result output.
- Moved headless tracing and benchmark runs onto the shared worker/chunk `RayTraceRunner` backend, including no-export execution without photon buffering.
- Added deterministic benchmark controls through fixed ray count, seed, worker count, and chunk size; result JSON reports effective `worker_count`, `chunk_count`, and `chunk_size`.
- Added the benchmark configuration JSON schema at `docs/benchmark_config_schema_v1.json` and the template config at `examples/benchmarks/benchmark_config_v1.example.json`.
- Added benchmark result metrics including total power, flux statistics, and a deterministic little-endian float64 row-major `flux_grid_sha256`.
- Added CSV and raw little-endian float64 binary flux-grid output/reference support. Binary reference grids are preferred when both CSV and binary references are supplied.
- Documented the benchmark v1 Zenodo dataset DOI: `https://doi.org/10.5281/zenodo.20395328`.

## Notes

- Benchmark mode currently supports no-export execution only.
- Whole result JSON files are not byte-for-byte deterministic because elapsed time and rays/s vary between runs. Compare scalar metrics and `flux_grid_sha256` for deterministic validation.
- Changing `chunk_size` intentionally changes deterministic chunk seeds, so the flux-grid hash is expected to change.
