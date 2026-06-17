# Player/MarioDraw

Verdict: needs_impl
Date: 2026-06-13 4:08am MNL

Reason: not functionally certifiable because the rebuilt TU is missing target
functions and static data. The objdiff overview reported missing vector/model
matrix helpers plus several Mario animation/model data entries.

Offending symbols:
- `JGeometry::TVec3<float>::set<float>(float, float, float)`
- `M3UMtxCalcSIAnmBlendQuat::M3UMtxCalcSIAnmBlendQuat()`
- `M3UModelCommonMario::getMtxCalc(const M3UMtxCalcSetInfo&)`
- `@1936` (`.ctors`)
- `@3179`, `@3181`, `@3202`, `@3204`, `@3206` (`.ctors`)
- `@3340`, `@3342`, `@3344` (`.ctors`)
- `@1431`, `@1411`, `@1210` (`.ctors`)
- `M3UModelCommonMario::__vtable`
- `@4948`-`@4951` (`.ctors`)
- `@6706`, `@6707` (`.ctors`)
