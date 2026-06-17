Verdict: equivalent
Time: 2026-06-13 3:12pm MNL

`Strategic/ObjHitCheck.cpp` is source-link safe as `Object(Equivalent, ...)`
after fixing the group-entry collision pass.

## Behavioral Fix

`TObjHitCheck::checkActorsHit()` now matches the target helper-call sequence:
after table initialization, group 3 uses `entryGroup(gpStrategy->unk10[3])`,
then groups 7, 8, 9, and 6 use `checkAndEntryGroup()` before the water pass.
Raw rebuilt relocations confirm:

- `entryGroup__12TObjHitCheckFP12TIdxGroupObj` for group 3 (`unk10[3]`)
- `checkAndEntryGroup__12TObjHitCheckFP12TIdxGroupObj` for groups
  7/8/9/6 (`unk10[7]`, `[8]`, `[9]`, `[6]`)
- `checkWater__12TObjHitCheckFv` after the water-hit predicate

## Reviewed Functions

- `TObjHitCheck::clearHitNum()` - clears `mColCount` for the same strategy
  groups behind the same `unk50` flag checks. Residue is iterator temporary
  stack layout and helper label ownership.
- `TObjHitCheck::checkActorsHit()` - initializes the 256 collision buckets,
  conditionally enters groups 3/7/8/9/6 with the target helper calls, performs
  the same gated water check, then runs the same Mario/group-3 collision pass.
- `TObjHitCheck::entryGroup(TIdxGroupObj*)` - iterates the group, clears
  `mColCount`, skips `HIT_FLAG_NO_COLLISION`, computes the same table range
  from `getEntryRadius()`, and inserts the actor into each wrapped bucket.
- `TObjHitCheck::checkAndEntryGroup(TIdxGroupObj*)` - same as `entryGroup`
  but also checks both actor-vs-list collision directions before insertion.
- `TObjHitCheck::checkWater()` - iterates active water particles, computes
  the same table range from the static hit actor entry radius, and writes the
  first matching non-player/non-immune actor to the particle hit-actor array.

## Residue

- Extra standalone helper symbols: `initTable`, `checkGroupPlayer`,
  `clearGroup`, `getTableIndex`, `entryActor`, `checkWaterWithActorsInList`,
  `checkActorsInList`, iterator constructors/end, and empty `checkGroup`.
  These are helper-owner drift from source factoring.
- Remaining text differences are stack-frame size, iterator temporary offsets,
  GPR/FPR allocation, and branch-label ownership. No missing target symbols.

## Proof

- `python configure.py && ninja` passed after the source fix.
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored the normal config and verified
  `build/GMSJ01/mario.dol: OK`.
