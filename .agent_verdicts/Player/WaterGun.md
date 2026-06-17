# Player/WaterGun

Verdict: needs_impl
Date: 2026-06-13 4:07am MNL

Reason: not functionally certifiable because the rebuilt TU is missing target
static-constructor data, including the dirty texture filename/name symbols.

Offending symbols:
- `@2386`, `@2442`-`@2445` (`.ctors`)
- `cDirtyFileName`
- `cDirtyTexName`
- `@2694` (`.ctors`)
- `@4359`, `@4361`, `@4363`, `@4368`-`@4371` (`.ctors`)
