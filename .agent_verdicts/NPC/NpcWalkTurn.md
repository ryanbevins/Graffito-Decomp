# NPC/NpcWalkTurn

Verdict: equivalent
Date: 2026-06-14 11:34am MNL

Promoted `mario/NPC/NpcWalkTurn` to `Object(Equivalent, ...)`.

Build proof:
- `python configure.py --non-matching && ninja` linked with `NpcWalkTurn.o`
  sourced.
- `python configure.py && ninja` restored the matching config and passed
  `build/GMSJ01/mario.dol: OK`.

Behavior review:
- `isTurnToMarioWhenApproach`, `isTurnToMarioWhenTalk`, and
  `isNeedTurnToFirstState` are byte-identical.
- `execTurnToFirstState` differs only in stack-frame size/stack-slot placement
  and helper label attribution; the branch instructions and operations are the
  same.
- `execUTurn` differs only in frame/FPR/register spelling and helper label
  attribution. The target and source both compute the current graph-point yaw,
  reject blocked states, wrap target/current yaw, apply the live-flag side
  adjustment, chase `mRotation.y`, rewrap, and return true when chasing
  finishes.
- `execWalk(bool)` now matches target behavior after the implementation fix:
  bad entry states reset `mMarchSpeed`/`mTurnSpeed` and exit; the
  turn-to-current-node path rotates toward `unkF4.getPoint()` and clears bit 0
  of `unk1DA` below `0.001f`; the normal path uses the same animation-kind
  speed/accel selection, march-speed chase, turn-speed assignment, XZ distance
  test, and calls `walkToCurPathNode()` when the squared distance is at least
  `CLBSquared(10.0f)`.

Remaining byte debt:
- Target owns a 16B local `JGeometry::TVec3<float>::set<float>` helper after
  `execWalk`; source inlines that helper at the call site and therefore has no
  undefined reference. This is helper-boundary/ownership debt only.
- Other visible diffs are frame size, stack slot layout, saved FPR/GPR choice,
  `fabs` vs compare/negate spelling, switch branch layout, and local template
  label attribution.
