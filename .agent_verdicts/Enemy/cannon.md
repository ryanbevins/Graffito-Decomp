# Enemy/cannon

Verdict: needs_impl
Date: 2026-06-13 4:06am MNL

Reason: not functionally certifiable because the rebuilt TU is missing target
static-constructor tables. The objdiff overview reported missing
`xzTable$3107`, `sCannonDomPartsJointTable$3043`, and local ctor `@5781`.

Offending symbols:
- `xzTable$3107`
- `sCannonDomPartsJointTable$3043`
- `@5781` (`.ctors`)
