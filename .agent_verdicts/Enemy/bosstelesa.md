# Enemy/bosstelesa

Verdict: needs_impl
Date: 2026-06-13 4:08am MNL

Reason: not functionally certifiable because the rebuilt TU is missing target
static-constructor data. The overview reported missing local ctor strings,
slot/vector tables, and entry tables.

Offending symbols:
- `@2315`, `@2554`-`@2557` (`.ctors`)
- `xzTable$3488`
- `entry$3173`
- `entry$3276`
