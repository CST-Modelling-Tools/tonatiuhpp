# Architectural A0: opt-in headless tracker equivalence

## Scope and decision

The native GUI sets tracker parent links when loading a document and updates its tracker transforms in the loaded scene. The legacy headless CLI reads a scene then constructs the instance tree without those updates. A tracker fixture can therefore produce different optics from the same file and seed.

This change provides a deliberately opt-in headless compatibility path:

```powershell
& "build/application/Release/tonatiuhpp.exe" --headless trace-scene "tests/scientific/fixtures/one-axis-tracker.tnhpp" --rays 20001 --seed 123456789 --no-export --update-trackers
```

`--update-trackers` calls `TSceneKit::updateParents()` then `updateTrackers()` **before** headless scene-instance construction and aperture sizing. Without the flag the legacy path is unchanged. This is intentional: global automatic tracker updates would need separate scientific sign-off, especially for the immutable 500-million-ray reference and older CLI/script callers.

The option is accepted only by `--headless trace-scene`; benchmark and headless script interfaces are not modified. The command prints `tracker_update: enabled` for affirmative opt-in; no extra line is written in default mode.

## Automated acceptance

Two scientific tests supplement the existing 19:

- `ScientificTracker.HeadlessOptInMatchesGuiLifecycleAndDiffersFromLegacy`: independently load the tracker fixture in all preparation modes using 20,001 rays and seed 123456789; compare full sorted exact hit multisets and preparation/result diagnostics between GUI-style and headless opt-in, repeat both opt-in and legacy paths, and assert legacy output differs from tracked output. This is a lifecycle-equivalent GUI-style trace, **not** native MainWindow event automation.
- `ScientificTracker.OptInPreservesNoTrackerScene`: opt-in does not change scientific results on the cylinder fixture.

Two CTest smoke tests invoke the actual headless `trace-scene` executable with and without the flag using the tracker scene.

```powershell
cmake --build build --config Release --parallel 16
ctest --test-dir build -C Release -R '^scientific[.]' --output-on-failure
ctest --test-dir build -C Release -R '^headless[.]trace_tracker_' --output-on-failure
```

CI must pass on Windows, macOS and Ubuntu. The fixture and all benchmark reference inputs are unchanged. No numerical reference reset is permitted. Native GUI/manual end-to-end equivalence, full photon export parity and 500M reference reproduction remain distinct milestones.
