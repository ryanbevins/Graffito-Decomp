# System/MSoundMainSide

Verdict: equivalent
Date: 2026-06-15 12:17pm MNL

## Reason

`System/MSoundMainSide.cpp` now defines all target text symbols, including the
formerly missing TU-owned weak rows for `JGadget::TVector<void*>::begin()` and
`std::sqrtf(float)`. Static `MSStageInfo`/`MSStage::smMSStage` `.sbss` layout
matches the target.

Strict review found no behavioral blocker. Remaining non-exact rows are
codegen/data debt:

- `MSStageCubeSwitch::proc()` and `MSMainProc::getMonteVillageActorArea()`:
  stack/local-vector placement only.
- `MSStageCubeFade*::proc()` / `MSStageDistFade*::proc()`: frame size,
  FPR/GPR coloring, local vector temp placement, helper-boundary drift around
  list `begin()`/`sqrtf`, and equivalent distance/fade/pan/dolby arithmetic.
- `MSStage::init()` and `MSMainProc::setMSoundEnterStage()`: allocation/setup
  helper boundary, switch layout, register coloring, and local data-label drift;
  same stores, constants, calls, and branch conditions.
- `.ctors`/`.data`/`.sdata2`: label/order drift only, plus source-only
  unreferenced `JSUList` destructor owners.

## Proof

- `python configure.py --non-matching && ninja` linked with
  `System/MSoundMainSide.cpp` sourced.
- `python configure.py && ninja` passed `build/GMSJ01/mario.dol: OK`.
