# System/EventWatcher

Verdict: needs_impl
Date: 2026-06-13 4:07am MNL

Reason: not functionally certifiable because the rebuilt TU is missing target
functions and local static data. The objdiff overview reported missing
`SpcTrace`, vector/map-object helpers, and several local ctor entries.

Offending symbols:
- `SpcTrace(const char*, ...)`
- `JGeometry::TVec3<float>::set(const Vec&)`
- `TMapObjBase::appear()`
- `TMapObjBase::kill()`
- `@3988`, `@4300` (`.ctors`)
- `@3931`, `@3932`, `@3989`, `@3990` (`.ctors`)
