# MoveBG/MapObjBlock audit

Verdict: `equivalent`

Certified: 2026-06-15 7:46am MNL.

`MoveBG/MapObjBlock.cpp` is source-linkable and behaviorally equivalent after
the audit pass. Proof:

- `git diff --check`
- `python configure.py --non-matching && ninja`
- `python configure.py && ninja` (`build/GMSJ01/mario.dol: OK`)

Behavior fixes made during certification:

- Restored target `.sdata` tunables:
  `TSandBlock::{mSandScaleUp,mSandScaleDown,mSandScaleMin,mWaitTimeToFall,mSandWaitTime}`
  and `TIceBlock::{mMeltSpeedWater,mMeltSpeedAuto,mAutoMeltScale}`.
- `TSandBlock::control()` recovery compares `mScaling.y` against
  `mInitialScaling.x`, matching the target offset reads.
- `TIceBlock::touchWater()` calls `getWaterSpeed()` with the sender/water hit
  actor as the implicit object, then uses `makeObjDead()` for the melt-dead
  transition.
- `TIceBlock::control()` calls `setObjHitData(0)` before hiding collision during
  auto-melt and uses `makeObjDead()` for the final dead transition.
- `TTelesaBlock::setGroundCollision()` now calls vtable slot 3 directly instead
  of loading through the function pointer.

Remaining non-exact diffs are byte/codegen debt:

- `TIceBlock::calc()` inlines `J3DTexMtxInfo::setEffectMtx`; target calls the
  helper, but the stores are equivalent.
- `TTelesaBlock::perform()` differs in register/frame layout, bool branch shape,
  scratch matrix store ordering, and cached vs repeated `getModel()` calls.
- `TLeanBlock` and `TSandBlock` residue is stack/FPR/register allocation,
  switch lowering, vector temp placement, and helper-boundary shape.
- Aggregate `.rodata`/`.data`/`.sdata` drift is owner/label/include debt; target
  symbols are present and the source-link proof passed.
