# mario/System/MarDirectorPreEntry

Verdict: equivalent
Date: 2026-06-13 6:19am MNL

Reason:
- The only emitted function, `TMarDirector::preEntry(TPerformList*)`, keeps
  the same name-ref lookup order, same `TPerformList::push_back` calls, same
  constants, same viewport allocation/construction, and same conditional
  indirect-scene block.
- The residual diff is stack-frame/local-slot size and rodata label ownership
  only; all target strings and constants are present with matching bytes.
- Source-link proof passed under `python configure.py --non-matching && ninja`,
  then normal `python configure.py && ninja` restored the matching config and
  verified `build/GMSJ01/mario.dol: OK`.

2026-06-13 10:11am MNL recheck:
- Current overview still has no missing target symbols. `preEntry` still has
  the same name-ref lookup order, `TPerformList::push_back` sequence, flag
  constants, viewport construction, and conditional indirect-scene block.
- Remaining residue is stack-frame/local-slot size and rodata label ownership.

Offending functions: none.
