verdict: needs_impl
date: 2026-06-13 3:55am MNL
tu: mario/Enemy/smallEnemy
source: src/Enemy/smallEnemy.cpp

2026-06-15 8:37pm MNL recheck: still `needs_impl`. Current overview has no
missing target rows, but the source still carries explicit TODO/unknown areas
around `getSaveParam2()` helper shape, random interval handling, water-hit
actor typing, and hit-water-jump inline structure. Low-score shared-base
functions remain (`expandCollision` 11.6%, `isMarioInWater` 40.4%,
`generateItem` 62.7%, `moveObject` 83.5%, `reset` 79.4%). Do not promote until
these behavior centers are audited or reconstructed.

Reason:
- Not certified. The overview has no missing target rows, but the source still
  carries explicit fabricated/unknown-source areas in shared base-class
  behavior, including fabricated `getSaveParam2()` helpers, uncertain water-hit
  actor typing in `decHpByWater()`, and random-interval helper TODOs in
  `init()` / hit-water-jump logic.
- Low-score functions such as `TSmallEnemy::expandCollision()` and
  `TSmallEnemy::isMarioInWater() const` were inspected and appear partly
  helper-boundary/codegen related, but this TU is a shared enemy base class and
  needs a full behavior audit before promotion.

Offending functions/areas:
- `TSmallEnemy::expandCollision()`
- `TSmallEnemy::isMarioInWater() const`
- `TSmallEnemy::decHpByWater(THitActor*)`
- `TNerveSmallEnemyHitWaterJump::execute(TSpineBase<TLiveActor>*) const`
- fabricated/unknown `getSaveParam2()` helper declarations
