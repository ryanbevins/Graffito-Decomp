# Enemy/chuuhana

Verdict: needs_impl
Date: 2026-06-13 4:05am MNL

Reason: not functionally certifiable because the rebuilt TU is missing target
static-constructor data. The objdiff overview reported missing
`graphlist$2835`.

Offending symbols:
- `graphlist$2835` (`.ctors`)
