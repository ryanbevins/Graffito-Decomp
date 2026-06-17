# Enemy/limitkoopa

Verdict: needs_impl
Date: 2026-06-13 3:51am MNL

Reason: not functionally certifiable because the rebuilt TU is missing target
functions and static-constructor data. The objdiff overview reported missing
`JGeometry::TVec3<float>::set<float>(const JGeometry::TVec3<float>&)`,
`JGeometry::TVec3<float>::sub(...)`, `__sinit_limitkoopa_cpp`, local ctor
entries `@2850`, `@2852`, `entry$3213`, and `[.ctors-0]`.

Offending symbols:
- `JGeometry::TVec3<float>::set<float>(const JGeometry::TVec3<float>&)`
- `JGeometry::TVec3<float>::sub(const JGeometry::TVec3<float>&, const JGeometry::TVec3<float>&)`
- `__sinit_limitkoopa_cpp`
- `@2850` (`.ctors`)
- `@2852` (`.ctors`)
- `entry$3213` (`.ctors`)
- `[.ctors-0]`
