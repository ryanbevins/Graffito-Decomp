# Enemy/bosseel

Verdict: needs_impl
Date: 2026-06-13 4:06am MNL

Reason: not functionally certifiable because the rebuilt TU is missing many
target static-constructor strings/tables. The overview shows missing boss eel
name/path data, collision/joint tables, and several local ctor entries.

Offending symbols:
- `@2111`, `@2556`-`@2559` (`.ctors`)
- `eyeTable$3627`
- `@4418`, `@4421`-`@4423` (`.ctors`)
- `@6784`, `@6786`-`@6798` (`.ctors`)
- `@7896`, `@8912`, `@8911`, `@8987`, `@8988` (`.ctors`)
- `@1431`, `@1411`, `@1210` (`.ctors`)
- `entry$3126`
- `sEyePartsJointTable$3535`
- `sToothPartsJointTable$3546`
- `sCollisionJointTable$3578`
- `sCollisionFileTable$3582`
