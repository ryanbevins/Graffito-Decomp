# mario/MarioUtil/ModelUtil

Verdict: equivalent
Date: 2026-06-13 5:14pm MNL

Certified `Object(Equivalent, "MarioUtil/ModelUtil.cpp")` after the
`TYoshi::onYoshi()` owner split removed the prior source-link blocker.

Behavior review:

- The model factory helpers, `TMultiBtk` helpers, and
  `SMS_RideMoveCalcLocalPos(...)` are byte-identical.
- `SMS_RideMoveByGroundActor(...)` has the same ground check, riding actor
  replacement path, actor-matrix fetch/copy path, inverse/local-position update
  path, yaw-delta update, and null-clear path. The only visible drift is stack
  frame size and stack-slot offsets for the `TBGCheckData*` out parameter and
  temporary matrices.
- `TYoshi::onYoshi()` no longer appears as an extra in this TU.

Proof:

- `python configure.py --non-matching && ninja` linked successfully from source.

Verdict: needs_impl  
Date: 2026-06-13 12:03pm MNL

Do not promote yet. The visible text behavior is close: the model factory
helpers and `TMultiBtk` helpers match, and
`SMS_RideMoveByGroundActor(...)` differs mostly by stack-frame/slot offsets
around the same ground check, riding actor update, matrix copy/inverse/multiply,
and yaw delta stores.

Current blocker is source-link ownership:

- A temporary `Object(Equivalent, "MarioUtil/ModelUtil.cpp")` promotion failed
  `python configure.py --non-matching && ninja`.
- Linker error: multiply-defined `TYoshi::onYoshi()` in `MarioAction.o`,
  previously defined in `ModelUtil.o`.

The source-only `__sinit_ModelUtil_cpp`/static-init artifacts remain byte debt,
but the duplicate `TYoshi::onYoshi()` owner is the concrete reason this TU
cannot currently link from source.
