# Enemy/tinkoopa

Verdict: needs_impl
Date: 2026-06-13 4:08am MNL

Reason: not functionally certifiable because the rebuilt TU is missing target
static-constructor tables and filename arrays. The objdiff overview reported
missing local ctor strings, model/animation tables, effect filename tables,
and `MtxCalcTypeName`.

Offending symbols:
- `@2111`, `@2194`-`@2197` (`.ctors`)
- `entry$3488`
- `@1431`, `@1411`, `@1210` (`.ctors`)
- `MtxCalcTypeName`
- `table$3029`, `table$3036`, `table$3041`-`table$3047`
- `onetimeFilenames$3493`
- `loopFilenames$3507`
- `loopIndirectFilenames$3531`
