# Player/MarioWait

Verdict: equivalent
Date: 2026-06-15 7:06pm MNL

Safety-net recheck for current `Object(Equivalent, "Player/MarioWait.cpp")`.
The stale first verdict was `not_equivalent`, but current strict review found
no behavior blocker.

Reviewed:
- `TMario::waitMain()` held-object put path: target and source both test the
  held actor type against `0x80000001`, then otherwise perform the same two
  `TMap::isTouchedOneWall()` probes before the put status change.
- `TMario::waitMain()` squat/standup dispatch: raw target asm routes
  `0x0C008222` and `0x0C000223` through `squatStandup()`, matching source.
- `TMario::waitMain()` throw-end path: raw target asm calls
  `checkThrowObject()` followed by `jumpEndCommon(0x65, 0x0C400201)`, matching
  source.
- `TMario::canPut()` computes the same forward and current-position wall probes
  with the held object's damage radius; the remaining diff is load/order
  codegen only.

Remaining debt is byte-level only: stack/register placement, branch-result
materialization, local data label `@3463`, and data/sdata2 ownership drift.

Proofs:
- `python configure.py --non-matching && ninja` linked successfully during the
  same audit tick after `Enemy/wireTrap` was promoted.
- Plain `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.
