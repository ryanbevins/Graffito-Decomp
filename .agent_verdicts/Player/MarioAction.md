Verdict: equivalent
Time: 2026-06-13 6:31am MNL
Unit: mario/Player/MarioAction
Source: src/Player/MarioAction.cpp

Reason:
- `TMario::actnMain()` matches exactly.
- `TMario::taking()` matches exactly.
- `__sinit_MarioAction_cpp`, the actual `.ctors` relocation, and target
  `.sdata2` rows match exactly.
- Objdiff extras are unreferenced weak helper bodies from included headers plus
  local symbol-label artifacts; direct `objdump -r -s -j .ctors` shows both
  target and source have the same single `.ctors` relocation to
  `__sinit_MarioAction_cpp`.
- Re-verification of existing `Object(Equivalent, ...)` linked cleanly under
  `python configure.py --non-matching && ninja`.
- 2026-06-13 6:31am MNL recheck: overview still has exact target-owned
  functions/data, and `python configure.py --non-matching && ninja` linked from
  source.
- 2026-06-13 8:08am MNL Matching promotion probe: objdiff/report still show exact target-owned sections, but changing only this row to `Object(Matching, ...)` made the normal DOL checksum fail. Keep it `Equivalent`; the remaining issue is linker relocation/symbol-resolution byte debt, not an observed behavioral difference.
- 2026-06-13 11:42am MNL recheck: verdict remains `equivalent`. Current
  overview still has exact `actnMain`, `taking`, `__sinit`, `.ctors`, and
  `.sdata2`; extras are source-owned weak/header helpers and local
  rodata/ctors label artifacts. Shared proof from this tick passed:
  `python configure.py --non-matching && ninja`, then normal
  `python configure.py && ninja` verified `build/GMSJ01/mario.dol: OK`.
