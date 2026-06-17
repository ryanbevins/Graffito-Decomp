# Enemy/Kumokun Audit

Verdict: equivalent  
Date: 2026-06-14 2:43am MNL

Source-link proof:

- `Object(Equivalent, "Enemy/Kumokun.cpp")`
- `python configure.py --non-matching && ninja` linked from source.
- Normal `python configure.py && ninja` then passed with
  `build/GMSJ01/mario.dol: OK`.

Behavior fixes made during re-audit:

- `TKumokunManager::load()` now writes hit params to attack radius, attack
  height, damage radius, and damage height; the previous source wrote attack
  radius twice.
- `TKumokun::bindOnFlying()` now calls `checkWallPlane()` with `mHeadHeight`
  as the Y probe offset and `mBodyRadius` as the wall radius, matching target.
- `TKumokun::checkOnMovingFloor()` now snaps accepted ground movement to the
  computed ground height (`dVar10`) instead of the sampled Y.
- `TKumokun::checkOnMovingRoof()` now snaps accepted roof movement to the
  computed roof height (`dVar10`) instead of the sampled Y.
- `TKumokun::prepareFly()` now seeds `mVelocity`; target stores the fly vector
  to offsets `0xac/0xb0/0xb4`, not `mLinearVelocity`.

Remaining nonmatching diffs are codegen-class:

- stack frame size/slot drift in the movement helpers and nerves;
- FPR/register allocation differences in quaternion/vector math;
- local helper and singleton/static-init ownership label drift;
- data/rodata/sdata drift from helper ownership and local constants.

No missing target symbols remain.

Previous verdict:

Verdict: ready_for_reaudit  
Date: 2026-06-14 2:29am MNL

Implementation update:

- `TKumokun::initAttachPlane()` now constructs the first
  `TBGWallCheckRecord` at `mPosition.y + mHeadHeight`, matching the target
  first wall-probe height. Normal `python configure.py && ninja` passed.
- Focused diff still has stack/FPR/source-shape residue and existing helper
  ownership extras, but the previously recorded collision-height behavior
  blocker is fixed. Re-run AUDIT for the final `Equivalent` verdict and
  source-link proof.

Earlier verdict:

Verdict: not_equivalent  
Date: 2026-06-13 3:31am MNL

Temporary source-link proof passed:

- `Object(Equivalent, "Enemy/Kumokun.cpp")`
- `python configure.py --non-matching && ninja`

The TU still had a behavioral mismatch and stayed `NonMatching`.

Offending function:

- `TKumokun::initAttachPlane()`: the target's first wall probe constructs the
  `TBGWallCheckRecord` position with `mPosition.y + mHeadHeight`
  (`lfs 0x14`, `lfs 0xc0`, `fadds f2, f1, f0`) before calling
  `TMap::isTouchedWallsAndMoveXZ`. Previous source constructed the record with
  plain `mPosition.y`, changing the initial attach-wall query height.
