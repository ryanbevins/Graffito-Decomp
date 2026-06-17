# mario/Player/MarioCap

Verdict: equivalent
Status: certified_equivalent
Time: 2026-06-14 4:12am MNL

## Reason
Re-audited the stale `needs_impl` verdict and fixed the remaining behavior
differences before promoting `Player/MarioCap.cpp` to `Object(Equivalent, ...)`.

Behavior fixes made during audit:
- `TMarioCap::perform()` now clears helmet model shape flag bit 0 when the
  helmet model is active and sets it when inactive, matching the target loop.
  The old source had the active/inactive loops inverted.
- The same update phase now calls `J3DModel::calc()` for sunglasses, current cap,
  and helmet models. The old source called `update()`, which dispatches a
  different J3DModel vtable slot.
- `TMarioCap::TMarioCap(TMario*)` now initializes `unk4` directly to
  `E_CAP_MODEL_HAT`; the old source ORed into the uninitialized flag field via
  `setModelActive()`.

Remaining reviewed diffs are codegen/data ownership only:
- Constructor resource/model setup, texture TIMG copy, dirty texture replacement,
  diver helmet / sunglasses creation, `null_airtube` lookup, matrix copies,
  `TMultiMtxEffect` setup, tremble model init, and material-packet init all
  perform the same operations. Diff is stack frame, register coloring, rodata
  offsets, and loop index width/source shape.
- `perform()` performs the same hat toggle, tremble-distance/action checks,
  Mario camera helmet flag activation, helmet/sunglasses/current-model
  calc/viewCalc/entry dispatch, TEV alpha update, and tremble movement. Diff is
  stack/vector temporary shape, local `JGeometry::TUtil<f32>::sqrt` ownership,
  register coloring, and equivalent branch layout.
- Missing rodata `@1763` is the standard Shift-JIS no-memory string
  (`メモリが足りません\n`). The rebuilt object has no unresolved reference to it;
  this is rodata/owner byte-debt, not an observable behavior blocker.

## Proof
- `python configure.py --non-matching && ninja` linked successfully with
  `Player/MarioCap.cpp` source-linked.
- `python configure.py && ninja` passed afterward with
  `build/GMSJ01/mario.dol: OK`.
