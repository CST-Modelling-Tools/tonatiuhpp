# M3 — Benchmark reference-v2 artifact integrity

**Status:** development branch; cross-platform CI pending.

This fast, standard-library-only CI check validates the **existing** published 500-million-ray benchmark-v2 reference files, without performing optical ray tracing. It pins the existing reference metadata, physical power/flux scalar values and SHA-256 rather than silently accepting a changed baseline. It reads the entire 80,000-byte binary grid, validates its little-endian float64 length and SHA-256, checks exact CSV-to-binary double-value agreement across the 100 × 100 grid, and independently checks min/average/max flux against the recorded metadata.

Run locally from the repository root:

```powershell
python tests/scientific/verify_reference_v2.py
```

The CI workflow runs this check on Windows, Ubuntu and macOS before the regular build and CTest suite. The check uses no NumPy or other new Python dependencies.

**Scope limitation:** This is artifact integrity, **not** proof of physical correctness, GUI/headless equivalence, cross-platform float-for-float matching, or an actual 500-million-ray benchmark rerun. The example configuration and reference name `benchmark_heliostat_field_v1.tnhpp`, but that scene is **absent** from `examples/benchmarks/` in the current GitHub repository. The full reference reproduction needs the authentic scene and appropriate machine/time budget; do not synthesize a replacement scene or change the approved hashes.

No production C++ implementation, tests, benchmark files or reference values are modified. Maintain the existing 21 scientific/headless tests and all other CTests. Before merging, verify CI passes on Windows, Ubuntu and macOS and review the scientific scope.
