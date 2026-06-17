# mario/Map/PollutionEvent

Verdict: equivalent
Date: 2026-06-13 6:06am MNL

Reason:
- Re-verified during the AUDIT sweep. `TPollutionTest::loadAfter`,
  destructor, `perform`, `__sinit_PollutionEvent_cpp`, the `TPollutionTest`
  vtable, and `.ctors` entries are exact.
- Remaining overview drift is weak-owner/data ownership only: source emits
  `JDrama::TViewObj` destructor/vtable plus the `JSUList<...>` destructors
  pulled in by the rogue sound includes. These are target-present weak symbols
  owned outside this TU and do not alter runtime behavior.
- Proof: `python configure.py --non-matching && ninja` linked from source,
  then normal `python configure.py && ninja` passed and verified
  `mario.dol: OK`.

- 2026-06-13 8:08am MNL Matching promotion probe: objdiff/report still show exact target-owned sections, but changing only this row to `Object(Matching, ...)` made the normal DOL checksum fail. Keep it `Equivalent`; the remaining issue is linker relocation/symbol-resolution byte debt, not an observed behavioral difference.

Offending functions: none.

2026-06-13 11:42am MNL recheck: verdict remains `equivalent`. Current overview
still has exact owned text/ctor rows; remaining drift is source-owned
`JDrama::TViewObj` and `JSUList<...>` weak/destructor ownership plus data label
ownership. Shared proof from this tick passed: `python configure.py
--non-matching && ninja`, then normal `python configure.py && ninja` verified
`build/GMSJ01/mario.dol: OK`.
