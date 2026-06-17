verdict: needs_impl
date: 2026-06-13 4:02am MNL
tu: mario/MoveBG/MapObjPinna
source: src/MoveBG/MapObjPinna.cpp

Reason:
- Not certification-ready. Overview has missing target `.ctors` data symbols,
  with broad `.rodata` / `.data` / `.sdata2` drift.
- No `Equivalent` promotion was attempted because target symbols are absent.

Offending symbols:
- `@2111`, `@2178`, `@2179`, `@2180`, `@2181`
- `@3818`
