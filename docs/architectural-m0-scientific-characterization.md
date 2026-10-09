# Architectural M0 — Multi-reflection, tracker and exact-hit characterization

**Status:** New development branch awaiting cross-platform CI and scientific review.

This increments the original Architectural M0 scientific safety net without changing production physics, ray propagation, RNG, scheduling, photon exports, or the immutable 500-million-ray reference.

## Characterization

The two new test-only scenes exercise the actual `MaterialSpecular` plugin with zero optical slope and reflectivity 0.95:

- `specular-two-reflections.tnhpp`: two tilted reflectors lead to an absorbing receiver placed below both mirrors to avoid shadowing the initial downward sun rays. Tests require callback hits on all three surfaces, repeated deterministic traces, GUI-style borrowed versus headless-owned preparation agreement, and photon-recording versus no-recording agreement. The recorded-photon test additionally checks for at least one consecutive `Sun (id 0) -> First (id 1) -> Second (id 2) -> Receiver (id 3)` path, proving two reflections occurred along the same ray rather than assuming a global hit count must exceed launched rays.
- `one-axis-tracker.tnhpp`: built-in `TrackerArmature1A` updates the primary mirror toward a receiver. One test verifies tracking angle and transform change, sun-position response, and disabled behavior; another compares scientific event signatures from independently loaded scene instances.

The tests explicitly call `TSceneKit::updateParents()` **then** `updateTrackers()` before the tracker trace. The former initializes the TrackerKit parent link needed to apply the calculated rotation to the primary transform. Native GUI normally prepares tracker state via its scene update lifecycle; existing headless preparation does **not** itself run these updates. These tests prove equivalence for identically prepared tracker states, not automatic GUI/CLI tracker preparation.

## Stronger event signature

Previous comparison relied on 16-bin-per-axis histograms, front/back counts, and other summaries. New comparisons also collect each real `RayTracerHit` callback event as (instance URL, side, exact 64-bit position components), sort the events, and compare the complete multisets. This detects sub-bin position changes, surface identity changes, and side changes independently of callback ordering among worker threads.

Scope remains the events the existing callback emits, **not** full per-ray sequence identity or entire photon file output. Exact bitwise equality is asserted only within each OS/compiler toolchain. Cross-platform scientific results are not required to be bitwise identical.

## Validation

Run after Release compilation on Windows, macOS, and Ubuntu:

```powershell
cmake --build build --config Release --parallel 16
ctest --test-dir build -C Release -R '^scientific[.]' --output-on-failure
```

Five added scientific tests target exact event sensitivity, repeated two-reflection traces, photon recording parity, tracker angle response, and identically updated tracker-state scientific parity.

The benchmark reference, official scene and scientific algorithm remain unchanged. Additional native GUI/CLI end-to-end comparison, recording file equivalence and performance reference are separate remaining Architectural M0 gates.
