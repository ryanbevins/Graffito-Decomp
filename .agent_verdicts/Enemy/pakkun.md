# Enemy/pakkun

Verdict: equivalent
Date: 2026-06-15 10:49pm MNL

Proof:
- `python configure.py --non-matching && ninja` linked `pakkun.cpp` from
  source and produced `build/GMSJ01/mario.dol`.
- Restored plain `python configure.py && ninja` passed
  `build/GMSJ01/mario.dol: OK`.

Audit fixes landed before promotion:
- `TPakkunSeed::forceKill()` and `TPakkunSeed::behaveToHitGround()` now match
  the target's unconditional `mGroundPlane` dereferences; the previous null
  guards were behavior-bearing source drift.
- `TPakkun::onShootLiner()` uses `mSLSeedSpeedS`, matching the target load at
  `TPakkunParams + 0x320`.
- `TPakkun::perform()` flag `2` path now calls
  `calcRootMatrix(); updateAnmSound(); mMActor->calcAnm();`, matching the
  target vtable calls.
- `TPakkunManager::TPakkunManager()` no longer clears `mStayParams`; target
  initializes `mWaterEmitInfo`, `mHideWaterEmitInfo`, globals, and `unk5C`
  only. This constructor now byte-matches.

Remaining non-exact rows reviewed as codegen/data-label debt:
- Nerve executes, Pakkun/StayPakkun/Seed behavior, matrix callbacks, item
  generation, water/hit-ground handling, load/init paths, clipping, and model
  data creation follow target control flow and calls.
- `JGeometry::TVec3<float>::set<float>(float, float, float)` remains a missing
  local helper row, but `pakkun.o` has no undefined reference to it; source code
  inlines the same vector stores.
- `entry$3011` is the target local label for the existing six-word model-data
  table (`entry`) used by `TPakkunManager::createModelData()`.
- Residue is stack/register/FPR scheduling, helper-boundary, static-guard,
  const-pool, and local-label shape rather than behavior.
