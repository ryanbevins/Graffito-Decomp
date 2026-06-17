Verdict: equivalent
Date: 2026-06-14 5:07am MNL
TU: mario/Player/MarioJump

Proof:
- Fixed three behavior blockers found during the strict AUDIT pass, then
  promoted `Player/MarioJump.cpp` to `Object(Equivalent, ...)`.
- `python configure.py --non-matching && ninja` linked cleanly with
  `MarioJump` source-linked.
- Follow-up normal `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.

Behavior fixes included in the certification:
- `TMario::stayWall()` wall-kick input now always transitions to
  `0x02000886`, seeds `mVel.y = 52.0f`, conditionally lowers it to `1.0f`
  against `mFloorPosition.x`, and uses the target back-trigger masks
  (`mInput & 0x8000`, `mEnabledFrameMeaning & 0x2000`). The no-wall fallback
  now calls `setPlayerVelocity(0.0f)`.
- `TMario::doJumping()` now uses `JMASCos(angleDiff)` for forward acceleration
  and `JMASSin(angleDiff)` for side velocity, matching the raw target trig
  table usage.
- `TMario::hipAttacking()` now uses the target `70.0f` distance threshold
  before snapping to hip-drop interactors.

Remaining byte debt:
- `jumpMain()` still has large dispatcher/switch-shape drift, but reviewed
  cases dispatch to the same handlers and status constants after the fixes.
- `diving()`, `rocketing()`, `stayWall()`, `jumpingBasic()`, `doJumping()`,
  and `hipAttacking()` retain frame/register/helper-boundary/FPR-order drift.
- Extra weak/list/destructor symbols and ctor/rodata label differences do not
  block equivalence; the source-linked build has no undefined-symbol failure.
