# Enemy/hamukuri

Verdict: needs_impl
Date: 2026-06-13 4:04am MNL

Reason: not functionally certifiable because the rebuilt TU is missing target
static data. The objdiff overview reported missing local `.ctors` entries
`@6521`, `@6522`, and `@6523`. Several major behavior functions also remain
low-score and would need deeper audit after symbol completeness is restored.

Offending symbols:
- `@6521` (`.ctors`)
- `@6522` (`.ctors`)
- `@6523` (`.ctors`)
