# M3.3 — Bounded tracker state and repeat-interception parity (local)

**State (local Windows acceptance, October 21, 2026): PASSED.** Native fixed-seed GUI and true-headless tracker scripts produced exactly identical `12/12` CSV rows for azimuths 90 and 110 degrees (2,048 rays per interception, 200x200 sun grid). The independently repeated headless outputs matched each other. Intercepted powers were `4701.7761516804867 W` and `5170.4040280008849 W`. Hosted CI and PR integration remain pending.

## Contract

- Add `NodeObject.getParameter(name)` as a read-only counterpart to `setParameter`, returning Coin's serialized field representation (raw string for `SoSFString`); for `TSeparatorKit` use its transform part, matching setter semantics. Missing fields throw JS errors at the user call site. There are no new field-writing or physics operations.
- Same in-memory script builds a one-axis tracker and specular primary reflector, plus planar absorbing receiver. It checks tracker armature/target configuration, actual sun field readback, two 2,048-ray interception results at 90 and 110 degrees azimuth, exact repeated-result stability at each sun position, and the computed target-angle field after each run. A 12-row CSV records those observations with double-precision powers.
- Automated CTest executes the **exact** script twice in real true-headless processes from a different working directory. It compares the complete artifacts (line endings normalized), retains a baseline, then removes the last headless output to avoid accepting a stale file as GUI evidence.
- Native GUI gate uses fixed seed 0, 200x200 sun grid and the same generated script. It verifies GUI responsiveness/scene tree and compares actual GUI-generated 12-row artifact against the headless baseline. Neither interface is assumed equivalent until measured.

## Accepted scope and remaining boundaries

The bounded native acceptance confirms the tracker armature/target field readback, tracker angle response to the two tested sun positions, repeated fixed-seed interception and exact GUI/headless artifact parity for this fixture. It does not prove arbitrary tracker/scene equivalence or photon-export parity. Separate M3.3 Windows-local lifecycle, save/reload and cancellation gates subsequently passed, as recorded in `docs/m3-script-parity-local.md` and `PROJECT_STATUS.md`. Hosted three-platform CI, code review and merge remain pending. The frozen 500-million-ray benchmark and accepted scientific references were not rerun or changed.
