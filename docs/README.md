# Tonatiuh++ documentation

This folder documents **the current codebase**, not the sequence of changes used to build it. The authoritative implementation is the source and its tests. Scientific reference files and example scenes live under `examples/benchmarks/` and must not be changed as part of documentation cleanup.

## Start here

- [Architecture](architecture.md): shared simulation and scripting services, permanent GUI/CLI separation, and official plugins.
- [Build and test](developer-setup.md): development setup, native testing, and safe local build practices.
- [Headless commands and benchmark](headless-benchmark.md): CLI usage, script interface, photon File export, and benchmark file formats.
- [Scientific validation](scientific-validation.md): reproducibility checks, immutable benchmark inputs, and optional full-scale verification.
- [Continuous integration](change-aware-ci.md): which checks run for documentation-only and code changes.
- [Benchmark configuration schema](benchmark_config_schema_v1.json): current machine-readable benchmark configuration contract.

## Other maintained files

- `release-notes-v0.1.8.26.md` is retained because the existing release workflow requires a notes file for the current project version. Its presence does **not** mean that a public release has been approved.
- `../PROJECT_STATUS.md` holds a concise snapshot of the current project state and outstanding work.
- `../AGENT.md` contains persistent engineering and review rules.
- The application help sources in `../help/` are separate from this developer documentation.

Historical milestones, superseded design plans, prior-release notes, and dated acceptance diaries are not maintained in this folder. Git history retains earlier versions if they are ever needed.
