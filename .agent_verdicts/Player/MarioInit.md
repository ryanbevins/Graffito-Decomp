# Player/MarioInit Audit

Verdict: needs_impl  
Date: 2026-06-13 3:56am MNL

Overview mode shows missing target data symbols, so the TU cannot be certified:

- Missing `.ctors`: `@1431`, `@1411`, `@1210`, `@2594`, `@2727`, `@3334`,
  `@6572`, `@6573`, `@6574`, `@6575`, `@6576`, and `@6578`.
- The rebuilt object also emits many target-absent parameter/helper owners,
  including `TMario::*Params` constructors, shine-table helpers,
  `TYoshi::onYoshi()`, `TWaterGun::isEmitting()`, `TNozzleBase::getNozzleKind()`,
  and extra shine conversion/table data.

Because every target symbol/data row must be present before `Equivalent`, this
remains `NonMatching`; no behavioral certification was attempted.
