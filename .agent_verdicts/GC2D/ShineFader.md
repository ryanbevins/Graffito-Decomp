# mario/GC2D/ShineFader

Verdict: equivalent
Date: 2026-06-13 6:06am MNL

Reason:
- Re-verified during the AUDIT sweep. Every owned `TShineFader` function is
  exact: destructor, `load`, `registFadeout`, `perform`, and `update`.
  `TShineFader::__vtable` also matches.
- Remaining overview drift is data/weak ownership only: source emits
  `JDrama::TViewObj` and `TSmplFader` destructors/vtables that are
  target-present weak symbols owned outside this TU.
- Proof: `python configure.py --non-matching && ninja` linked from source,
  then normal `python configure.py && ninja` passed and verified
  `mario.dol: OK`.

- 2026-06-13 8:08am MNL Matching promotion probe: objdiff/report still show exact target-owned sections, but changing only this row to `Object(Matching, ...)` made the normal DOL checksum fail. Keep it `Equivalent`; the remaining issue is linker relocation/symbol-resolution byte debt, not an observed behavioral difference.

Offending functions: none.

2026-06-13 11:42am MNL recheck: verdict remains `equivalent`. Current overview
still has every owned `TShineFader` function and vtable exact; the remaining
data drift is source-owned `JDrama::TViewObj`/`TSmplFader` destructor/vtable
ownership. Shared proof from this tick passed: `python configure.py
--non-matching && ninja`, then normal `python configure.py && ninja` verified
`build/GMSJ01/mario.dol: OK`.
