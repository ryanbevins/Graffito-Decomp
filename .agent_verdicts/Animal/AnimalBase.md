# Animal/AnimalBase

Verdict: equivalent
Date: 2026-06-13 4:43pm MNL

`mario/Animal/AnimalBase` is functionally equivalent and links from source.

Proof:

- `python configure.py --non-matching && ninja` passed after promoting
  `Animal/AnimalBase.cpp` to `Object(Equivalent, ...)`.

Audit notes:

- Fixed one real behavior-class mismatch during this audit: target uses signed
  `cmpw` for the shared-model draw-matrix loop in `TAnimalBase::perform()`.
  Source now uses `int count` / `int i`, and `perform` improved
  `83.5% -> 86.2%`.
- The old red verdict named missing local helper-owner symbols
  `JGeometry::TVec3<float>::set<float>(float, float, float)` and
  `MsClamp<float>(float, float, float)`. The rebuilt TU has no undefined
  references to either helper; their bodies are inlined into the source output,
  so this is byte debt, not a source-link or behavior blocker.
- `TAnimalBase::execWalk(bool)` and `getRotationFlyToDir(...)` perform the
  same speed chase, target direction selection, wrap/clamp steering,
  roll/pitch chase, quaternion rotation, and velocity/rotation stores.
  Remaining drift is frame/register/FPR shape and helper-owner call boundaries.
- `resetRandomCurPathNode()`, `initNoLoad_(TAnimalBase*)`, and `init()` use the
  same random offset ranges, actor-type branches, path-node copies, nerve setup,
  frame timer initialization, frame-control seeding, and group-list insertion.
  The `mFrameTimer` allocation still calls `__nwa__` in source versus target
  `__nw__`, but both global operators route to `JKRHeap::alloc(size, 4,
  nullptr)` for the same 8-byte allocation.
- `perform(unsigned long, JDrama::TGraphics*)` now has the target signed
  draw-matrix loop condition. Remaining matrix-copy and shared-animation diffs
  are stack/register/source-shape residue with the same calls, offsets, counts,
  and stores.
- `load`, `loadAfter`, `SMS_Eular2Quat`, and `__sinit_AnimalBase_cpp` are
  codegen/data-owner residue: stack size, FPR/register coloring, JALList owner
  labels/order, local constant labels, and inline helper ownership.
