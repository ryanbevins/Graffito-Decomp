# Enemy/hanasambo

Verdict: needs_impl
Date: 2026-06-13 4:04am MNL

Reason: not functionally certifiable because the rebuilt TU is missing target
static-constructor data. The objdiff overview reported missing `@2118`,
`@2357`-`@2360`, `entry$2760`, `entry$2894`, and `entry$3085`.

Offending symbols:
- `@2118` (`.ctors`)
- `@2357` (`.ctors`)
- `@2358` (`.ctors`)
- `@2359` (`.ctors`)
- `@2360` (`.ctors`)
- `entry$2760` (`.ctors`)
- `entry$2894` (`.ctors`)
- `entry$3085` (`.ctors`)
