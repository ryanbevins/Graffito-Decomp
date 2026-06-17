# MoveBG/MapObjTree audit

Verdict: equivalent
Checked: 2026-06-14 5:20pm MNL

`mario/MoveBG/MapObjTree` is functionally equivalent and source-links cleanly
as `Equivalent`.

Fixes made during audit:

- Corrected target-owned statics in `.sdata`: `sWaitTime = 1`,
  `mBananaTreeJumpPower = 1000.0f`, `mScaleSpeedY = 0.005f`,
  `mStatusChangeScaleY = 0.3f`, `mScaleSpeedXZ = 0.007f`, and
  `mScaleMin = 0.1f`.
- Removed source-only `TMapObjTree::mNearMiddle` and `mMiddleFar` statics; they
  have no target symbols and were unused.
- Recovered the target `initEach()` actor-type table from asm literals:
  types `0x40000034..37` use the small spring set
  `(0.001f, 0.006f, 0.01f, 0.97f)`, while `0x40000039` uses
  `(0.004f, 0.008f, 0.03f, 0.9f)`.
- Fixed the hidden/polluted scale-tree path to set `mDamageHeight = 30.0f`.
- Fixed the map-event rumble gate in `TMapObjTreeScale::control()`: target
  shakes when `mEventSink` is null or `mEventSink->isBuried(1)` returns true.
- Fixed particle scatter math to use a `400.0f` span and `200.0f` offset around
  the tree position.

Remaining non-100% diffs are codegen/ownership class:

- `TMapObjTree::initMapObj()` and `controlLeaf(int)` differ mainly by
  stack/register layout and out-of-line `JGeometry::gekko_ps_copy12()` calls
  versus inlined paired-single copies.
- `TMapObjTreeScale` constructor/load/control/touchWater residue is stack/frame,
  local label ownership, bool materialization, and weak/header-owner drift.
- Extra source symbols are weak/header-owner emissions from included base/helper
  classes; there are no missing target symbols after the fixes.

Proof:

- `python configure.py --non-matching && ninja` linked successfully with
  `MoveBG/MapObjTree.cpp` from source.
- `python configure.py && ninja` restored the normal matching configuration and
  verified `build/GMSJ01/mario.dol: OK`.

Reverification: 2026-06-13 8:21pm MNL

- Current overview still has the same nonmatching/codegen-owner residue and no
  new missing behavior symbols. Extra weak/helper emissions remain owner debt
  already covered by the proof.
- The current tick's `python configure.py --non-matching && ninja` proof linked
  all `Equivalent` rows from source, then `python configure.py && ninja`
  restored the matching config with `build/GMSJ01/mario.dol: OK`.

Reverification: 2026-06-14 5:20pm MNL

- Current overview remains source-link-safe with no missing target symbols.
  Remaining diffs are the known codegen/ownership class: matrix copy helper
  boundaries, stack/register layout, `.rodata`/`.sdata`/`.sdata2` order, and
  extra weak/header-owner emissions.
- This tick's `python configure.py --non-matching && ninja` and normal
  `python configure.py && ninja` proof builds covered the current object.
