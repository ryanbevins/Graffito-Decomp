# Player/ModelWaterManager

Verdict: needs_impl
Date: 2026-06-13 4:06am MNL

Reason: not functionally certifiable because the rebuilt TU is missing target
text and static data. The objdiff overview reported missing the
`TWaterHitActor` thunk dtor, several local ctor entries, `gWaterManagerPlaneInfo`,
and the large `tmp_data` table.

Offending symbols:
- `@32@__dt__14TWaterHitActorFv`
- `@3162` (`.ctors`)
- `@1431` (`.ctors`)
- `@1411` (`.ctors`)
- `@1210` (`.ctors`)
- `gWaterManagerPlaneInfo`
- `tmp_data`
- `@3825` (`.ctors`)
