# Animal/Bird

Verdict: equivalent
Date: 2026-06-14 8:31pm MNL

Scope reviewed:
- Re-read overview and strict diffs after the implementation handoff for
  `TNerveAnimalBirdWalkOnGround::execute`, `TAnimalBird::bind`,
  `TAnimalBird::moveObject`, `TAnimalBird::load`, `TAnimalBird::doLanding`,
  `TAnimalBird::receiveMessage`, the landing/prelanding/comeback/change-to-coin/
  graph-wander/wait nerves, `doFlyToCurPathNode`, and `isFindMario`.
- Raw target asm confirmed `load` actor-type/color classification, blue-coin
  handling, `SMS_InitPacket_OneTevColor` use, and the waterproof fall/damping
  ratios in `doFlyToCurPathNode`.

Equivalence notes:
- `MsWrap<float>(float, float, float)` is now emitted in the TU-local target slot
  and objdiff reports it 100%.
- The remaining missing `JGeometry::TVec3<float>::set<float>(float, float,
  float)` helper is a 16B owner/call-boundary residue from `TQuat4::rotate`.
  Source inlines the same three stores, and `powerpc-eabi-nm -u` confirms there
  is no undefined reference.
- Remaining data/local rows (`@1490`, `@1940`, `@2018`-family, `entry$3023`,
  `bird_bastable`, `@unnamed@::cMatName`, `@4897`, `@4898`) are label/owner debt.
  No reviewed diff showed a behavior-changing condition, call, constant, state
  store, or return-value difference.

Build proof:
- `python configure.py --non-matching && ninja` passed with
  `Animal/Bird.cpp` linked from source.
- `python configure.py && ninja` passed after restoring the standard matching
  config, with `build/GMSJ01/mario.dol: OK`.
