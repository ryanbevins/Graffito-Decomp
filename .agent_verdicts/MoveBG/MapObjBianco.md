verdict: needs_impl
date: 2026-06-13 4:02am MNL
tu: mario/MoveBG/MapObjBianco
source: src/MoveBG/MapObjBianco.cpp

Reason:
- Not certification-ready. Overview has missing target `.ctors` data symbols
  and large `.rodata` / `.data` drift.
- Several low-score functions also remain, but the missing symbols alone block
  source-defined completeness.

Offending symbols:
- `@2111`, `@2178`, `@2179`, `@2180`, `@2181`
- `@3250`, `@3251`, `@3254`, `@4234`
