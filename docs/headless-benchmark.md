# Headless commands, scripting, and benchmark

Tonatiuh++ supports nonvisual operation through the permanent standalone `tonatiuhpp-cli` executable (`tonatiuhpp-cli.exe` on Windows). It shares one scientific kernel, simulation services, official plugins, and scripting implementation with `tonatiuhpp-gui`. The GUI no longer offers a `--headless` command path.

The CLI commands below are direct commands. There is no `--headless` prefix or old executable alias.

## Commands

```text
tonatiuhpp-cli --help
tonatiuhpp-cli validate-scene examples/benchmarks/cylinder.tnhpp
tonatiuhpp-cli trace-scene examples/benchmarks/cylinder.tnhpp --rays 10000 --seed 123456789 --no-export
tonatiuhpp-cli benchmark examples/benchmarks/benchmark_config_v2.example.json
tonatiuhpp-cli run-script path/to/script.tnhpps
```

**Do not run the provided v2 benchmark configuration casually:** it specifies 500,000,000 rays. See [Scientific validation](scientific-validation.md) for smaller routine checks and an explicitly controlled full-scale run.

The CLI reports successful completion or an error through its exit status; argument errors return code `2`. The standalone CLI requires its common runtime dependencies and official plugins.

## Scene tracing and File export

`trace-scene` requires a scene file, `--rays N`, `--seed S`, and exactly one of `--no-export` or `--photon-export-dir DIR`.

No-export example:

```text
tonatiuhpp-cli trace-scene examples/benchmarks/cylinder.tnhpp --rays 2048 --seed 123456789 --no-export
```

File export example (create a fresh **empty absolute** directory first):

```text
tonatiuhpp-cli trace-scene examples/benchmarks/cylinder.tnhpp --rays 2048 --seed 123456789 --photon-export-dir C:/TonatiuhTemp/PhotonOutput --photon-file-name RunA --photon-file-size -1
```

Optional File-export switches are `--photon-file-name NAME`, `--photon-file-size N` (`-1` selects one file), and repeatable `--photon-surface URL` for surface selection. Direct File export has **no artificial 10-million-ray total limit**. It can process large runs while flushing photon records in bounded batches to disk. The requested ray count must fit the supported ray-count type, and the filesystem must have enough capacity; write failures remain fatal. `--photon-file-size N` controls photons **per output segment**, not the total number of photons allowed. The exporter still rejects conflicting options and unsafe output destinations. File-export and no-export modes are mutually exclusive.

The `--update-trackers` option is explicit: it applies GUI-like tracker updates in `trace-scene`. Do not assume an omitted option has the same effective tracker behavior for every scene.

For reproducibility, compare scientific hit/history signatures and canonical photon records, not file write order or parallel photon IDs.

## Headless scripts

Use:

```text
tonatiuhpp-cli run-script path/to/script.tnhpps
```

The shared `ScriptHost` supplies JavaScript execution and script-relative paths. The basic nonvisual API includes:

```javascript
print(value);
tn.writeJson(path, value);
tn.validateScene(path);
tn.runBenchmark(path);
tn.traceScene({ scene: scenePath, rays: 2048, seed: 123456789, noExport: true });
```

`tonatiuh` is also an alias for the script API. The file-based `tn.traceScene` form shown above **does not support photon File export**; use the direct CLI File option or the separately tested domain Run/export scripting APIs as appropriate. Scripts can also use an asynchronous no-export `tn.startTrace` / `tn.cancelTrace` / `tn.finishTrace` session when the script host exposes that API. Finish or cancel active sessions to release resources.

The graphical Script Editor additionally provides presentation-only capabilities; UI actions, dialogs and widgets cannot be assumed to work in a true-headless process. The source and tests under `tests/scripts/` define which advanced script operations have verified coverage. Do not assume that every bundled historical example is headless-compatible.

For automation, prefer script-written JSON output via `tn.writeJson(...)`. Elapsed time and rays per second are useful measurements, not deterministic scientific results.

## Flux benchmark configuration

The benchmark command reads a JSON configuration, validates the scene, traces rays without photon export, and writes scalar statistics and optionally a CSV and binary receiver flux map.

- Machine-readable configuration: [benchmark configuration schema](benchmark_config_schema_v1.json).
- Current reference configuration: `examples/benchmarks/benchmark_config_v2.example.json`.
- Immutable scientific reference: `examples/benchmarks/benchmark_reference_500M_v2.json`, with corresponding binary and CSV grids.
- The execution identifier remains `"benchmark": "benchmark_v1"`; the `v2` filename identifies the frozen scientific reference, not another simulation algorithm.

The benchmark output includes `total_power_mw`, minimum/average/maximum flux in `MW/m2`, `flux_grid_sha256`, `worker_count`, `chunk_count`, `chunk_size`, and timing fields. `worker_count` is runtime metadata. The ray tracer uses fixed 10,000-ray chunks, with deterministic random streams associated with chunk identity; `worker_count` and `chunk_size` are not supported user-configurable benchmark inputs.

### Binary and CSV grid format

The optional binary flux grid consists of row-major, little-endian IEEE-754 64-bit floating-point values (no header), with dimensions from `target_grid.width` and `target_grid.height`. Units are `MW/m2`. Its SHA-256 is stored in `flux_grid_sha256`. The optional CSV grid contains the same values in row-major order. An output file's byte order and all 64-bit values must be preserved when comparing exact grid hashes.

A reference JSON can provide expected scalar values, tolerances, flux-grid files, and an exact grid hash. The canonical 500-million-ray benchmark requires an **exact binary-grid SHA-256 match**, in addition to total-power and peak-flux checks. Across different operating systems, floating-point results are not generally guaranteed to be byte-identical; use the documented scientific tolerances rather than asserting universal cross-platform hash identity.
