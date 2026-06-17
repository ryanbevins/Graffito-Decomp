verdict: needs_impl
date: 2026-06-13 3:55am MNL
tu: mario/Enemy/bosspakkun
source: src/Enemy/bosspakkun.cpp

Reason:
- Not certification-ready. Overview still has missing target data symbols in
  `.ctors` and `.sdata`, including `@2154`, `@2210` through `@2213`,
  `entry$3621`, `entry$3626`, `@3353`, `@3355`, and `[.sdata-0]`.
- The rebuilt object emits many target-absent helper owners. Even though many
  nonmatching functions look like codegen debt, the missing target symbols
  fail the audit bar for source-defined completeness.

Offending symbols/areas:
- missing `.ctors`: `@2154`, `@2210`, `@2211`, `@2212`, `@2213`
- missing `.ctors`: `entry$3621`, `entry$3626`
- missing `.ctors`: `@3353`, `@3355`
- missing `.sdata`: `[.sdata-0]`
