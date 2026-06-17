verdict: equivalent
date: 2026-06-14 4:01pm MNL
unit: mario/Enemy/namekuri

Certified `mario/Enemy/namekuri` as `Equivalent` after fixing four remaining
behavior bugs found during audit:

- `TNameKuri::reset()` now initializes `unk1B0` to `1.0f` after
  `TWalkerEnemy::reset()`, matching the target store before randomizing
  `unk1B4`.
- `TNameKuri::setMeltAnm()` now starts BCK `1`; `setDeadAnm()` still starts
  BCK `0`.
- `TNameKuri::calcRootMatrix()` now reads the wall/side-plane normal from
  `unk138` in the `getWalker()->unk2C->unk10 > 0.0f` branch. The ordinary
  non-tilt branch still reads `mGroundPlane`.
- `TNameIndParCallback::execute()` now skips matrix/scale work while
  `LIVE_FLAG_CLIPPED_OUT` is set. The previous source had the branch inverted.

Remaining diffs reviewed as behavior-neutral byte debt: `setGoalPathMario`
helper boundary (`SMS_GetMarioHitActor()` call vs inlined `gpMarioAddress`
load), stack/frame size, FPR/GPR allocation, local matrix/vector construction
order, `TVec3::scale` helper boundary, anonymous rodata/data label offsets, and
source-only weak/helper owners. No target symbols are missing.

Verification:
- `python configure.py && ninja` passed before promotion.
- `python tools/decomp-diff.py -u mario/Enemy/namekuri -s missing` reported no
  rows.
- Promoted `Enemy/namekuri.cpp` to `Object(Equivalent, ...)`.
- `python configure.py --non-matching && ninja` linked with `namekuri` sourced.
- `python configure.py && ninja` restored matching config and passed
  `build/GMSJ01/mario.dol: OK`.
