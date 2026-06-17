# Enemy/Kazekun Audit

Verdict: equivalent
Recorded: 2026-06-13 2:52am MNL
Updated: 2026-06-14 10:36pm MNL

Unit: `mario/Enemy/Kazekun`
Source: `src/Enemy/Kazekun.cpp`

## Source-Link Status

`Enemy/Kazekun.cpp` is promoted to `Object(Equivalent, ...)`.
`python configure.py --non-matching && ninja` linked cleanly through
`build/GMSJ01/mario.dol`, then `python configure.py && ninja` restored the
normal matching config and passed `build/GMSJ01/mario.dol: OK`.

## Verdict

Behavior is equivalent. Before promotion, `TNerveKazekunTurn::execute` was
fixed to compare `mAroundTime` and spine time through `f32` conversion, matching
the target's `fsubs` compare sequence. Full audit of the remaining nonmatching
rows found codegen/helper-boundary debt only:

- Attack, `doAttackPose`, and `flyAroundMario` have the same vector/matrix/
  quaternion/velocity dataflow as target; low fuzzy comes from inlined
  quaternion/vector helpers, stack size, FPR allocation, and local helper owner
  differences.
- `attackToMario`, `behaveToWater`, and `calcRootMatrix` match the same
  latest-nerve predicates and resulting message/reset/particle actions; displayed
  destructor/call labels are objdiff symbol-alignment noise.
- Params, `init`, `reset`, `SMS_CalcToDirMatrix`, and `getQuat` have matching
  offsets, constants, stores, and control flow with rodata/static-label/register
  residue.

## Remaining Audit Question

- `JGeometry::TVec4<float>::set<float>(float, float, float, float)` (20B) is
  still displayed as missing target text, but `Kazekun.o` has no undefined
  `TVec4::set` reference. Current source inlines the helper at call sites; this
  is byte debt, not an equivalence blocker.

## Cleared During Implementation

- `JGeometry::TRotation3<JGeometry::TMatrix34<JGeometry::SMatrix34C<float>>>::getQuat(JGeometry::TQuat4<float>&) const`
  now emits locally at 688B and 99.5% fuzzy after the Kazekun owner split in
  `JGRotation3.hpp` / `Kazekun.cpp`.
- `JGeometry::TVec3<float>::set<float>(float, float, float)` now emits locally
  and matches the target 16B body after routing Attack's constructed vectors
  through tiny return-by-value helpers.
