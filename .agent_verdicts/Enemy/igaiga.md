# Enemy/igaiga

Verdict: needs_impl
Date: 2026-06-13 4:06am MNL

Reason: not functionally certifiable because the rebuilt TU is missing target
static-constructor data. The objdiff overview reported missing local ctor
entries around the roll-enemy setup plus missing animation/graph lists.

Offending symbols:
- `@2150` (`.ctors`)
- `@2378`-`@2381` (`.ctors`)
- `@3262` (`.ctors`)
- `anmlist$3317`
- `graphlist$3323`
- `@5524` (`.ctors`)
