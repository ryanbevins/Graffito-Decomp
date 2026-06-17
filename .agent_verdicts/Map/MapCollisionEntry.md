Verdict: equivalent
Time: 2026-06-13 6:30am MNL
Unit: mario/Map/MapCollisionEntry
Source: src/Map/MapCollisionEntry.cpp
commit_reviewed: 9692d011

Reason:
- Promoted `Map/MapCollisionEntry.cpp` to `Object(Equivalent, ...)` and proved
  it with `python configure.py --non-matching && ninja`.
- All rodata/data/sdata2 sections match and no target symbol is missing.
- Remaining nonmatching functions (`TMapCollisionWarp::setUp`,
  `TMapCollisionMove::init`, `TMapCollisionMove::move`,
  `TMapCollisionMove::moveSRT`, and `TMapCollisionBase::setUpTrans`) differ
  only by stack-frame/local-temp placement and local-label residue. Allocation,
  setup calls, flag checks, matrix/vector constants, virtual dispatch, and
  returns match target behavior.
- Extra text is default virtual/no-op and helper ownership
  (`TMapCollisionBase`, `TMapCollisionMove`, `TMapCollisionWarp`,
  `TMapCollisionStatic`, and infectious string helpers); it does not block the
  source-link build.
- 2026-06-13 6:30am MNL recheck: overview still has no missing target rows,
  and `python configure.py --non-matching && ninja` linked from source.
- 2026-06-13 10:02am MNL recheck: full `--no-collapse` diffs for
  `TMapCollisionWarp::setUp`, `TMapCollisionMove::init`,
  `TMapCollisionMove::move`, `TMapCollisionMove::moveSRT`, and
  `TMapCollisionBase::setUpTrans` remain behavior-equivalent. The setup flag
  tests, collision entry allocation, check-data initialization loop,
  `setList`/`updateTrans`/`update` dispatch choices, TRS matrix build, constant
  rotation/scale vectors, virtual setup calls, and entry-count stores match the
  target. Residue is stack/local vector placement and helper owner labels.
  Source unchanged since the 10:00am proof batch, which passed both
  `--non-matching` source link and normal `mario.dol: OK`.
