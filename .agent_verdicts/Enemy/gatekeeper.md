# Enemy/gatekeeper Audit

Verdict: needs_impl  
Date: 2026-06-13 3:58am MNL

Overview mode shows missing target data symbols, so the TU cannot be certified:

- Missing `.ctors`: `@2111`, `@2197`, `@2198`, `@2199`, `@2200`,
  `entry$2988`, `@5254`, `@5255`, `@5261`, `@5262`, `@5267`, `@3012`,
  and `@3014`.
- Missing `.sdata`: `[.sdata-0]` (8B).
- The rebuilt object also emits target-absent helper owners such as
  `TYoshi::onYoshi()`, many `TNerveBGK*::theNerve()` owners, JGadget iterator
  helpers, and matrix-calc virtual thunks.

Existing implementation notes already describe remaining BGK nerve/data work.
Because target data rows are missing, this remains `NonMatching`; no
`configure.py` promotion was attempted.
