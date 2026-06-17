Verdict: equivalent
Status: promoted
Date: 2026-06-15 2:43pm MNL
Unit: Player/MarioCollision

Promoted `Player/MarioCollision.cpp` to `Object(Equivalent, ...)`.

Audit findings:
- Fixed a real behavior mismatch in
  `TMario::damageExec(THitActor*, int, int, int, float, int, float, short)`:
  target disables the dropping-animation branch when action flag `0x2000` is
  set; source had forced `canPlayAnimation = true`.
- Restored the target-owned weak
  `JGeometry::TVec3<float>::operator*=(float)` body via a MarioCollision-only
  declaration split. This is required by original `MarioSpecial.o`
  (`wireWait`, `wireSWait`, `wireHanging`, `wireRolling`) when
  `MarioCollision` is sourced.
- Re-reviewed the damage-position vector block against target asm. Target
  spells it as `sub`/`dot`/`scale`/copy/`operator*=`/copy/`add`; source
  computes the same `mPosition + normalize(hittingActor->mPosition -
  mPosition) * 50.0f` with the same zero-vector fallback.
- Remaining non-exact rows are behavior-neutral: stack/register/FPR placement,
  vector helper-boundary choices, one missing unreferenced `TVec3` copy-ctor
  weak row, local label/data drift, and source-only unreferenced helper owners.

Proofs:
- `python configure.py --non-matching && ninja` linked and built the DOL with
  `MarioCollision` sourced.
- `python configure.py && ninja` passed `build/GMSJ01/mario.dol: OK`.

Prior blocker:
- 2026-06-15 1:56pm MNL: not promoted because `damageExec()`/`calcDamagePos()`
  and the missing vector weak-owner rows needed a combined behavior and
  source-link audit.
