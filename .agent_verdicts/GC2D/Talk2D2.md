# GC2D/Talk2D2

Verdict: needs_impl
Date: 2026-06-13 4:08am MNL

Reason: not functionally certifiable because the rebuilt TU is missing target
static data. The objdiff overview reported missing local `.ctors` entries
`@4550` and `@4551`.

Offending symbols:
- `@4550` (`.ctors`)
- `@4551` (`.ctors`)
