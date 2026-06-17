verdict: needs_impl
date: 2026-06-13 4:02am MNL
tu: mario/MoveBG/MapObjMare
source: src/MoveBG/MapObjMare.cpp

Reason:
- Not certification-ready. Overview has missing target `.ctors` data symbols,
  including the standard infectious/header data cluster and several local
  constants.
- Functions may be mostly behavioral/codegen-complete, but missing target
  symbols fail the audit completeness bar.

Offending symbols:
- `@1490`, `@2111`, `@2211`, `@2212`, `@2213`, `@2214`
- `@2690`, `@2692`
- `@2934`, `@2954`, `@2997`
