verdict: needs_impl
date: 2026-06-13 3:49am MNL
tu: mario/Enemy/bgtentacle
source: src/Enemy/bgtentacle.cpp

Reason:
- Not certification-ready. Overview has missing target data symbols
  `@1431`, `@1411`, `@1210`, `@3922`, and `@3923`; the rebuilt object also
  emits many target-absent helper owners.
- Source still contains explicit fake/TODO implementation notes around
  helper/inline recovery, hit-flag behavior, and a `char trash[0x10]`
  placeholder. Those are implementation/matching debt that needs review before
  promising functional identity.

Offending symbols/areas:
- missing `.ctors` data: `@1431`, `@1411`, `@1210`
- missing `.ctors` data: `@3922`, `@3923`
- `TBGTentacle::calcAttackGuideAnm()`
- `TBGTakeHit::perform(unsigned long, JDrama::TGraphics*)`
- source TODO/fake blocks around helper extraction and hit-flag operations
