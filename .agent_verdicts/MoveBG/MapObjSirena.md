verdict: needs_impl
date: 2026-06-13 4:02am MNL
tu: mario/MoveBG/MapObjSirena
source: src/MoveBG/MapObjSirena.cpp

Reason:
- Not certification-ready. Overview has missing target `.ctors` data symbols
  in the slot/casino object data region.
- Large data-section drift and target-absent helper owners remain, so no
  `Equivalent` promotion was attempted.

Offending symbols:
- `@4592`
- `@5166`
- `@4629`
