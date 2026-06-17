# Enemy/bossManta

Verdict: needs_impl
Date: 2026-06-13 4:04am MNL

Reason: not functionally certifiable because the rebuilt TU is missing target
static-constructor data and particle filename tables. The objdiff overview
reported missing `@2805`, `@2807`, `onetimeFilenames$3261`,
`loopFilenames$3273`, `@3363`, and `@5507`.

Offending symbols:
- `@2805` (`.ctors`)
- `@2807` (`.ctors`)
- `onetimeFilenames$3261`
- `loopFilenames$3273`
- `@3363` (`.ctors`)
- `@5507` (`.ctors`)
