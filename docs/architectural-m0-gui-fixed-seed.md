# Architectural M0 closeout — Optional fixed seed for GUI tracing

**Status:** Feature PR pending cross-platform CI and native GUI acceptance.
This is one narrowly scoped item in the original Architectural M0 safety net.

## Behavior

The existing **Ray tracing > Parameters > Rays** dialog has an opt-in
**Use fixed seed** checkbox and decimal input.

- **Unchecked (default):** GUI tracing keeps the original
  `QTime::currentTime().msec()` automatic master-seed selection.
- **Checked:** the chosen seed is passed unchanged to
  `GuiTracePreparationInput.configuration.masterSeed`. Zero is valid.
  The selection persists for later GUI tracing in the same `MainWindow`
  session until changed/unchecked; it is not saved to scene files or
  application settings.
- The accepted range is **0..4294967295**, matching the portable seed range
  of the current headless CLI parser (`ulong` is 32-bit on Windows).
  Empty/non-decimal/out-of-range inputs disable the dialog's OK button
  when the checkbox is on.
- Each real GUI tracing run logs `gui_master_seed: N (fixed)` or
  `gui_master_seed: N (automatic)` for reproducibility. Direct legacy
  `MainWindow::Run()` calls also use the window's current setting.
- Flux Analysis uses a separate seed lifecycle and is unchanged.

No optical physics, RNG, scheduler, exporter, benchmark reference or
frozen authentic-scene data are altered.

## Automated tests

Normal scientific CTest includes:

1. `ScientificGuiSeed.ParsesPortableDecimalInput` — input validation, zero,
   upper bound, overflow and non-decimal rejection.
2. `ScientificGuiSeed.FixedSelectionDoesNotChangeAutomaticMode` — automatic
   mode and lazy bypass of the old generator when a fixed seed is selected.
3. `ScientificGuiSeed.FixedSeedRepeatsThroughGuiStylePreparation` — two
   independent borrowed GUI-style traces of 20,001 rays with a non-default
   fixed seed, compared by the existing scientific signature.

These are not automated native `MainWindow` interaction tests.

## Manual GUI acceptance

Build the Release application and scientific tests on Windows and run:

```powershell
ctest --test-dir build -C Release -R '^scientific[.]' --output-on-failure
```

Launch Tonatiuh++ from a console; load
`examples/benchmarks/cylinder.tnhpp`; open the Ray tracing dialog;
select **No export**; enable **Use fixed seed** with `123456789`;
and perform two runs using the same scene, ray count and sun grid.
Check both console logs include `gui_master_seed: 123456789 (fixed)`.
Disable the checkbox and confirm the output reports `(automatic)`.
While enabled, clear the field and enter `4294967296` to confirm that
OK is disabled.

For the same no-export headless seed input, run:

```powershell
& "build/application/Release/tonatiuhpp.exe" --headless trace-scene "examples/benchmarks/cylinder.tnhpp" --rays 20001 --seed 123456789 --no-export
```

The GUI still does not emit a complete comparable hit signature. Full
native GUI/CLI equivalence, file-export comparison and additional M0
coverage remain separate acceptance gates. Equal seeds alone do not
prove end-to-end numerical equivalence.
