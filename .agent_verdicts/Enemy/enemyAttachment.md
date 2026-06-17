# mario/Enemy/enemyAttachment

Verdict: equivalent
Status: certified_equivalent
Time: 2026-06-14 4:07am MNL

## Reason
Re-audited after the tick 712 implementation fixes and promoted
`Enemy/enemyAttachment.cpp` to `Object(Equivalent, ...)`.

- `TEnemyAttachment::bind()` now has the target virtual-call behavior:
  `recoverScale()` at slot `0x13c`, `getNowGravity()` at slot `0x140`, then
  the later `setBehavior()` / `forceKill()` sequence. Remaining drift is
  stack/FPR allocation, `TBGWallCheckRecord` construction shape, and inline
  `TVec3::sub` versus direct component subtraction.
- `TEnemyAttachment::perform()` now dispatches inactive flag-2 handling through
  `behaveToHost()` slot `0x128`; the current diff is frame-size only.
- `TEnemyPolluteModel::perform()` performs the same active/culled checks,
  animation-end clear, matrix/scale update, `calcAnm()`, and pollution stamp.
  Diff is saved-register and stack layout.
- `TEnemyPolluteModelManager::perform()` performs the same frustum setup,
  active-model visibility updates, and per-model virtual perform loop. Diff is
  float argument load/order and register layout.
- `generatePolluteModel()` performs the same ground check, illegal/water-surface
  guard, pollute-model state update, transform setup, base matrix copy,
  activation flags, `setAnm()`, and ring-index wrap. Target inlines
  `TBGCheckData::isWaterSurface()` and `TEnemyPolluteModel::generate()` while
  current source owns helper text out of line; this is byte/codegen debt, not a
  behavior difference.

No target text symbols are missing. Extra rows are weak/helper owner debt
(`TEnemyPolluteModel::generate`, `identity33`, `JDrama::TViewObj`, and small
header helpers), and the `.data` mismatch is vtable/weak-owner relocation debt.

## Proof
- `python configure.py --non-matching && ninja` linked successfully with
  `Enemy/enemyAttachment.cpp` source-linked.
- `python configure.py && ninja` passed afterward with
  `build/GMSJ01/mario.dol: OK`.
