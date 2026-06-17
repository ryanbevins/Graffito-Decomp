# Enemy/bosswanwan

Verdict: needs_impl
Date: 2026-06-13 4:07am MNL

Reason: not functionally certifiable because the rebuilt TU is missing target
text and static-constructor data. The overview reported missing
`TVec3::set(float, float, float)`, local ctor/data entry `entry$3482`, and
several local ctor constants.

Offending symbols:
- `JGeometry::TVec3<float>::set<float>(float, float, float)`
- `entry$3482`
- `@1431`, `@1411`, `@1210` (`.ctors`)
- `@4961` (`.ctors`)
- `@5961`, `@5962` (`.ctors`)
