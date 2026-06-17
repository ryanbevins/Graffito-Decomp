# Player/MarioMove

Verdict: needs_impl
Date: 2026-06-13 4:05am MNL

Reason: not functionally certifiable because the rebuilt TU is missing target
static-constructor data. The objdiff overview reported missing local `.ctors`
entry `@6663`.

Offending symbols:
- `@6663` (`.ctors`)
