# mario/Map/MarineSnow

Verdict: equivalent
Date: 2026-06-13 6:06am MNL

Reason:
- Re-verified during the AUDIT sweep. Destructor, `perform`, `loadAfter`, the
  local `TVec3::set` helper, adjustor destructor thunk, rodata, vtable, and
  `.sdata2` rows match exactly.
- Full `--no-collapse` constructor diff shows the same base actor
  construction, zero/one vector initialization, null field stores, and final
  `TMarineSnow` vtable installation. The visible mismatches are relocation /
  label-owner drift from source-owned weak JDrama helper/destructor rows, not
  different operations.
- Proof: `python configure.py --non-matching && ninja` linked from source,
  then normal `python configure.py && ninja` passed and verified
  `mario.dol: OK`.

- 2026-06-13 8:08am MNL Matching promotion probe: objdiff/report still show exact target-owned sections, but changing only this row to `Object(Matching, ...)` made the normal DOL checksum fail. Keep it `Equivalent`; the remaining issue is linker relocation/symbol-resolution byte debt, not an observed behavioral difference.

Offending functions: none.

2026-06-13 11:42am MNL recheck: verdict remains `equivalent`. Current overview
still has only the constructor/data owner drift plus source-owned weak extras.
Full constructor diff preserves the same base actor construction, zero/one
vector stores, null member stores, and final `TMarineSnow` vtable install; the
visible mismatches are helper/vtable label ownership. Shared proof from this
tick passed: `python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` verified `build/GMSJ01/mario.dol: OK`.
