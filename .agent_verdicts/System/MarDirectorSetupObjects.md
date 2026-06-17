# System/MarDirectorSetupObjects

Verdict: needs_impl  
Date: 2026-06-13 1:06am MNL

Reason:
- Not promoted. The objdiff overview still has missing target symbols, which
  fails the strict audit gate before functional equivalence can be certified.

Blocking symbols:
- Missing `.text`: `JGeometry::TVec3<float>::set<float>(float, float, float)`.
- Missing `.ctors`: `@1490`, `@2110`, `@3755`, `@3897`.
- Nonmatching data rows remain for `@3810`, `@3898`, `.rodata`, `.data`, and
  `.sdata2`, with source-owned extras for shine/scenario tables. These need a
  source-shape/data-owner pass before this TU can be audited green.

Functions not fully certified:
- `TMarDirector::setupObjects()`
- `TMarDirector::decideMarioPosIdx()`
