# GC2D/SelectShine2

Verdict: equivalent
Date: 2026-06-15 11:25am MNL

Certification:
- Audited all remaining non-exact text rows in `mario/GC2D/SelectShine2`.
  The `move()` and `TSelectShine` ctor diffs are helper-boundary,
  stack/local-placement, register, and data-label residue; their control flow,
  calls, field stores, emitter setup, and model transforms match behavior.
- Fixed five real behavior blockers before promotion:
  constructor random/velocity/timing arguments, carousel clamp comparison
  directions, JMA angle scaling, yaw `atan2f` first-argument sign, and the
  draw-phase flag (`flags & 8`, not `flags & 4`).
- `perform()` now preserves the target's move/update clamps, shine-position
  recomputation, yaw update, frame animation, entry/viewCalc path, and
  draw-init/draw path. `initData()` now preserves both shine/empty material
  animation setup, slot population, initial yaw/position math, and emitter
  enable behavior.
- `Object(Equivalent, "GC2D/SelectShine2.cpp")` passed
  `python configure.py --non-matching && ninja`, linking the DOL with this TU
  sourced. Restored the normal config and `python configure.py && ninja`
  passed `build/GMSJ01/mario.dol: OK`.

Remaining byte debt:
- `move`, ctor, `perform`, and `initData` still carry stack/register/local
  placement residue and helper-boundary differences such as the extra
  `TVec3<f32>::add`/`TVec3<f32>::TVec3()` rows.
- `.data`/`.sdata2` have label/ownership drift, plus unreferenced
  `JDrama::TViewObj` dtor/vtable and infectious dummy string owner rows.
