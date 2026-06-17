# Enemy/bossgesso

Verdict: needs_impl
Date: 2026-06-13 4:07am MNL

Reason: not functionally certifiable because the rebuilt TU is missing target
functions and static data. The objdiff overview reported missing animation
accessors, `SMS_GetMarioPos()`, attack/index arrays, and several local ctor
entries.

Offending symbols:
- `MActorAnmDataEach<J3DAnmTransformKey>::getAnmPtr(int) const`
- `TMActorKeeper::getMActorAnmData() const`
- `SMS_GetMarioPos()`
- `entry$3707`
- `@5955`, `@7548` (`.ctors`)
- `idx$3335`
- `idxarray$3497`
- `idxarray$3517`
- `@5954`, `@5956`-`@5959` (`.ctors`)
- `@7101` (`.ctors`)
- `@8116` (`.ctors`)
