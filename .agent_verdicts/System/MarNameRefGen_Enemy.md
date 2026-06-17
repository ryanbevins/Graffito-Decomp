# System/MarNameRefGen_Enemy.cpp

Verdict: equivalent
Time: 2026-06-15 5:44am MNL

Reason:
- Re-audited after the implementation fixes from tick 806 and found no
  remaining behavior mismatch in `TMarNameRefGen::getNameRef_Enemy`.
- Added the inline `TNameKuriLauncher(const char*)` body so the factory branch
  expands to the target base `TLauncher` construction plus vtable stores,
  instead of referencing a nonexistent standalone constructor.
- The `AnimalMew` branch passes target `0x800001`; `BeamManager` stores the
  created manager into `gpBeamManager`; fabricated local class allocation sizes
  account for the real `JDrama::TNameRef` base; `TSamboFlower` and
  `TSamboFlowerManager` construction side effects match the target.
- Remaining drift is byte-debt only: rodata/SDA label offsets, stack frame size,
  helper ownership (`TRotation3` target no-op vs source `SMatrix34C` no-op),
  and extra local weak owners/vtable labels from lightweight class declarations.

Proof:
- `python configure.py --non-matching && ninja` linked with
  `System/MarNameRefGen_Enemy.cpp` sourced.
- `python configure.py && ninja` restored the normal matching config and passed
  `build/GMSJ01/mario.dol: OK`.

Current overview:
- `TMarNameRefGen::getNameRef_Enemy(const char*) const` is 99.9% fuzzy.
- All behavior-bearing named destructor/constructor rows reviewed for this TU
  are either byte-matching or behavior-equivalent.
