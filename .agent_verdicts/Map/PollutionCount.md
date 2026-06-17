# Map/PollutionCount.cpp

Verdict: equivalent
Time: 2026-06-12 9:56pm MNL

Build proof:
- `python configure.py --non-matching && ninja`
- `python configure.py && ninja`

## 2026-06-13 9:34am MNL - refreshed

Verdict remains equivalent after full `--no-collapse` diffs for all five
nonmatching functions:

- `TPollutionCounterLayer::calcViewMtx()`
- `TPollutionCounterLayer::countTexDegree(int)`
- `TPollutionCounterLayer::drawRevivalTexStamp(int) const`
- `TPollutionCounterLayer::drawJointObjStamp(int) const`
- `TPollutionCounterObj::countObjDegree() const`

The refreshed diffs preserve the earlier classification: state save/restore,
GX setup, texture/layer/stamp/object draw order, matrix construction, shape
draw loops, and draw-sync callback paths are behavior-identical. Residue is
stack/register/matrix-temporary layout, static helper-label owner drift, color
constant ownership, and `SMatrix34C<f32>` constructor emission; the constructor
is an empty 0x4 weak `blr`, so those target-only calls have no runtime effect.

No missing symbols.

Reviewed nonmatching functions:
- `TPollutionCounterLayer::calcViewMtx()`
  - Equivalent. Saves/restores `j3dSys.mViewMtx` and draw buffers, builds the world-to-pollution matrix for each layer, swaps both draw buffers to the layer buffer, runs model stamp `viewCalc()`/`entry()` for matching layer ids, then restores global state.
  - Residue is stack-frame size, temporary matrix layout, register allocation, and helper-label owner drift.
- `TPollutionCounterLayer::countTexDegree(int)`
  - Equivalent. Checks the layer enable byte, reinitializes GX, clears/draws the black background, loads the layer texture, initializes pollution-layer TEV state, draws the layer quad, sets the draw-sync counter, handles model stamp buffers, then draws joint, texture, revival, and prohibit-area stamps in the same order.
  - Residue is stack/local `GXTexObj` placement and helper-label owner drift.
- `TPollutionCounterLayer::drawRevivalTexStamp(int) const`
  - Equivalent. GX setup, frame-delay cycling, layer/type filters, texture load, quad vertex/texcoord writes, and loop advancement match.
  - Residue is stack slot layout and local color constant ownership.
- `TPollutionCounterLayer::drawJointObjStamp(int) const`
  - Equivalent. Filters joint-object tasks by layer id, selects joint layer/model, sets color channel and TEV state from the stamp type, builds the world-to-pollution matrix, loads vertices, and draws each shape.
  - Residue is stack/matrix temporary layout, register allocation, and static helper-label owner drift.
- `TPollutionCounterObj::countObjDegree() const`
  - Equivalent. Reinitializes GX, initializes draw-object GX state, builds scale/rotation/translation matrices from joint min bounds, loads the position matrix, draws each object counter, and sets the draw-sync token.
  - Residue is constructor/no-op matrix temporary emission, stack layout, and helper-label owner drift.

Extra symbols are source-emitted static helpers/inlines (`drawTexStamp`, `drawShape`, `makeWorldToPollutionMtx`, `loadPollutionLayer`, draw-sync helpers, list destructors, and color/rodata carriers), not missing target behavior.

## 2026-06-13 11:09pm MNL - reverified

Verdict remains equivalent.

Rechecked the current overview and focused diffs for the five nonmatching text
functions. The same classification still holds:

- `calcViewMtx()` preserves the matrix save/restore, pollution-layer loop,
  draw-buffer swap, matching-layer filter, and model `viewCalc()`/`entry()`
  calls. Drift is stack/frame layout, matrix temporary layout, and helper-label
  ownership.
- `countTexDegree(int)` preserves the layer-enable guard, texture load, GX
  setup, draw-sync token sequence, model buffer handling, and stamp draw order.
  Drift is stack-local `GXTexObj` placement and label ownership.
- `drawRevivalTexStamp(int) const` preserves the GX setup, frame-delay filter,
  texture load, quad writes, and loop advancement. Drift is stack layout and
  color constant ownership.
- `drawJointObjStamp(int) const` preserves the layer/type filters, TEV/color
  setup, world-to-pollution matrix construction, vertex loading, and shape draw
  loops. Drift is stack/matrix layout and register allocation.
- `countObjDegree() const` preserves the GX reinit, draw-object setup,
  joint-min scale/rotation/translation matrix construction, texture load, shape
  drawing, and draw-sync callback. Target-only `SMatrix34C<f32>` ctor calls are
  empty `blr` bodies, so they do not change runtime behavior.

No missing symbols. The earlier full `--non-matching` source-link proof remains
covered by the current tick's `python configure.py --non-matching && ninja`,
which passed before the normal matching rebuild.
