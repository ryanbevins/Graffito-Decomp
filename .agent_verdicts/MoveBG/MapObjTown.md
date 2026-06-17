# MoveBG/MapObjTown

Verdict: needs_impl
Date: 2026-06-13 3:51am MNL

Reason: not functionally certifiable because the rebuilt TU is missing
target static-constructor data. The objdiff overview reported missing local
`.ctors` entries `@2111`, `@2178`, `@2179`, `@2180`, `@2181`, and `@3400`.

Offending symbols:
- `@2111` (`.ctors`)
- `@2178` (`.ctors`)
- `@2179` (`.ctors`)
- `@2180` (`.ctors`)
- `@2181` (`.ctors`)
- `@3400` (`.ctors`)
