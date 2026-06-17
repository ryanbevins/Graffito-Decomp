# Map/MapWire audit

Verdict: equivalent
Date: 2026-06-15 1:51pm MNL

Reason:
- Certified `Object(Equivalent, "Map/MapWire.cpp")`.
  `python configure.py --non-matching && ninja` linked with this TU sourced,
  then plain `python configure.py && ninja` passed
  `build/GMSJ01/mario.dol: OK`.
- No target symbols are missing. Remaining text drift is helper-boundary,
  register/frame, and local data-label debt.
- The prior blocker was source-only factoring around released-point helpers.
  Re-review showed the same released-point arithmetic: line/default position,
  power from `getPointPowerAtReleased(pos)`, `(1.0f - mBounceRemainingPower)`,
  `mHangOrBouncePoint.y`, reset/return-rate updates, and bounce decay/timer
  logic. `getPointPowerAtReleased()` itself is exact; the source-only
  `fake_getPointPowerAtReleased()` just forwards to it.
- Other non-exact rows preserve behavior: `init()` builds the same points,
  fitting models, transforms, collision statics, and map-object registrations;
  `getPosInWire()` computes flattened perpendicular projection and
  partial/total length; `setFootPointsAtHanged()` uses the same hang point,
  reference positions, release thresholds, and point stores; draw functions emit
  the same vertex strips.

## Prior not_equivalent note

Checked 2026-06-13 2:01am MNL in AUDIT mode.

`mario/Map/MapWire` has no missing target symbols and the named data objects/static
floats match, but I did not certify it as `Equivalent`.

- `TMapWire::release()` and `TMapWire::move()` are dominated by source-only helper
  factoring around released-point placement (`getPointPosAtReleased`,
  `initPointAtJustReleased`, `updatePointAtReleased`) plus the deliberately named
  `fake_getPointPowerAtReleased()` helper. The raw target has the released-point math
  inline in `release()`/`move()` and calls `getPointPowerAtReleased()` directly.
- The math I checked appears to line up: reset point from defaults, compute line/default
  positions, apply `(1.0f - mBounceRemainingPower)` and
  `getPointPowerAtReleased(pos) * mHangOrBouncePoint.y`, then update the return rate.
  However the source itself marks this area as "very fake" / TODO, so the current file
  is not a clean audit candidate.
- `getPointPosOnWire()`, `getPosInWire()`, `setFootPointsAtHanged()`, `drawUpper()`,
  `drawLower()`, and `init()` did not show an obvious behavior mismatch in the sampled
  diffs; most residue is stack/register allocation, helper ownership, and constant
  label drift.

Before promotion, remove or justify the fake released-point helper factoring and re-audit
the `release()`/`move()` paths from source against the raw target asm.

## Recheck

Date: 2026-06-15 4:49am MNL

Still not promoted. Current overview still has no missing target symbols and
the released-state math in `release()` / `move()` appears broadly consistent,
but the exact audited area still depends on source-only helper factoring
(`fake_getPointPowerAtReleased`, released-point helpers) and TODO comments that
call the implementation fake/mathematically equivalent. This needs either a
cleaner source reconstruction or a more explicit proof that the helper-factored
source is behavior-identical before certifying `Equivalent`.
