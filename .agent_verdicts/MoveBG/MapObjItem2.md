verdict: equivalent
date: 2026-06-15 7:03am MNL
unit: mario/MoveBG/MapObjItem2

Certified `Equivalent` after a strict recheck and two additional behavior fixes.

Fixes made during audit:
- `TMushroom1up::control()` now returns immediately when `mTaken != 1`,
  `mType != 2`, `mState == 0`, and `isAirborne()` is true. Target exits the
  function in this path; the previous source skipped the state change but kept
  steering toward/away from Mario.
- `TJumpBase::control()` state 3 now plays `jumpbase_set` from the BCK end
  frame with rate `0.0f`. Target uses the frozen/end-frame setup after clearing
  `unk64 & ~1` and removing collision; the previous source restarted the
  animation at frame 0 with `SMSGetAnmFrameRate()`.

Audit result:
- `TJumpBase::control()` raw asm now matches source behavior for states 0-5:
  state 0 freezes `jumpbase_shrink` at end frame and radius 50, state 1 sets up
  collision and advances shrink to state 0, state 2 advances `jumpbase_set` to
  state 3 with radius 100 and live-flag clear, state 3 clears `unk64`, removes
  collision, and freezes `jumpbase_set`, state 4 advances jump to state 3, and
  state 5 launches once along Mario yaw then returns to state 2 when grounded.
- `TMushroom1up::control()` now matches the target's taken path, type-2 idle
  clear path, airborne early return, Mario-facing steering, angle clamp/wrap,
  velocity accumulation, and timer behavior. Remaining diffs are stack/register
  allocation, local-vector expression shape, store order for two zeroed velocity
  vectors, and local data-label drift.
- Other nonmatching functions (`calcRootMatrix`, `initMapObj`, `perform`,
  `makeObjAppeared`, `touchPlayer`) were reviewed as behavior-identical with
  frame/register/branch-offset/data-label residue only.
- Missing objdiff rows `@2831` and `@2833` are local data-label debt; the object
  source-links and has no undefined symbol blocker.

Proof:
- `python configure.py --non-matching && ninja` linked `MapObjItem2` from source.
- `python configure.py && ninja` restored matching config and passed
  `build/GMSJ01/mario.dol: OK`.
