# mario/MarioUtil/ModelUtil

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
