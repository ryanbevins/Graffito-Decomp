# MoveBG/MapObjLib Audit

Verdict: equivalent
Recorded: 2026-06-14 7:08pm MNL

Unit: `mario/MoveBG/MapObjLib`
Source: `src/MoveBG/MapObjLib.cpp`

## 2026-06-14 7:08pm MNL audit verdict

Certified `Equivalent` after fixing two behavioral audit findings:

- `TMapObjBase::{getWaterID,getWaterPlane,getWaterSpeed,getWaterPos,waterHitPlane}`
  now read the water particle slot at `TWaterHitActor+0x68` as a full word,
  matching target `lwz` and the `TModelWaterManager` slot convention.
- `TMapObjTurn::control()` no longer initializes the matrix to identity for
  out-of-range `unk150`; target falls through with the stack matrix contents
  for invalid axis values.

Proof:

- `python configure.py --non-matching && ninja` linked with
  `MapObjLib.o` sourced.
- `python configure.py && ninja` restored normal config and passed
  `build/GMSJ01/mario.dol: OK`.

Remaining byte debt is behavior-neutral: frame/register/FPR allocation,
helper-boundary differences around JGeometry matrix/vector routines, branch
layout in `TMapObjTurn`, anonymous data/rodata label drift, source-only weak
helpers, and the 16B local `JGeometry::TVec3<float>::set<float>(float, float,
float)` missing row. The rebuilt object has no undefined reference to that
helper; source inlines the same stores, so it is helper-owner debt rather than
missing behavior.

## Prior blocker, resolved

The old `needs_impl` verdict came from missing target-owned helper text. The
2026-06-14 implementation pass recovered four helper owners and the remaining
16B `TVec3<float>::set<float>` row is now classified as local helper-owner
byte debt, not missing behavior.

## 2026-06-14 7:08pm MNL implementation update

Recovered four of the five helper owners with `MapObjLib.cpp`-only JGeometry
owner-routing:

- `JGeometry::SMatrix33C<float>::SMatrix33C()` now emits and matches 100%.
- `JGeometry::SMatrix33C<float>::at(unsigned long, unsigned long) const` now
  emits and matches 100%.
- `JGeometry::TRotation3<TMatrix33<SMatrix33C<float>>>::setEular(float, float,
  float)` now emits and matches 100%.
- `JGeometry::TRotation3<TMatrix34<SMatrix34C<float>>>::setRotate(const
  TVec3<float>&, float)` now emits at the target 340B size; current diff is
  97.1% and reads as frame/FPR/register-expression residue with the same
  normalization, sin/cos, and matrix-store semantics.

Remaining `python tools/decomp-diff.py -u mario/MoveBG/MapObjLib -s missing`
row:

- `JGeometry::TVec3<float>::set<float>(float, float, float)` (16B).

That final row appears to be a local member-template helper that source inlines
with no source-link undefined. Natural emission routes were tried and rejected:
explicit specialization spelling is unsupported by MWCC for this explicit class
specialization's member template, an out-of-class member-template definition is
illegal, and in-class `#pragma dont_inline` around the template body compiles
but still does not emit the helper. Under the current implementation focus note,
this should be treated as an audit reclassification candidate rather than
missing behavior.
