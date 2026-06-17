# Map/MapMakeData Audit

Verdict: `equivalent`

Last rechecked: 2026-06-15 7:54am MNL.

Safety-net recheck: current overview still has no missing rows. The non-exact
functions remain the already-reviewed `initAllCheckData`, `updateTrans`, and
`setCheckData`; residual differences are stack/register/FPR/vector-temp and
helper-owner drift. Today's full `python configure.py --non-matching && ninja`
proof linked this object from source, and normal `python configure.py && ninja`
passed `build/GMSJ01/mario.dol: OK`.

## 2026-06-13 10:25pm MNL - refreshed

Verdict remains `equivalent`.

Re-read current diffs for `initAllCheckData`, `updateTrans`, and
`setCheckData`. They still preserve the same collision-data allocation,
triangle vertex loads, normal/plane/min-max recomputation, per-entry grid
insertion, translation update, and final `unk50` copy. Residue is unchanged:
stack frame and vector temporary layout, FPR/GPR allocation, and helper-owner
labels for `TVec3::sub` / `TBGCheckData::updateTrans` /
`TBGCheckData::setVertex`. This tick's `--non-matching` source-link proof and
normal `mario.dol: OK` build also covered the existing `Equivalent` row.

## 2026-06-13 9:29am MNL - refreshed

Verdict remains `equivalent`.

Re-read full `--no-collapse` diffs for `initAllCheckData`, `updateTrans`, and
`setCheckData`. The remaining differences are frame/stack layout, GPR/FPR
allocation, temp-vector layout, and source-owned helper labels. Raw objdump
confirmed the first `updateTrans` call is `TVec3::sub` in both target and source;
the pretty diff's `TBGCheckData::updateTrans` label on the source side is
address/owner drift, not a behavioral call mismatch.

## 2026-06-12 9:45pm MNL - equivalent

Verdict: `equivalent`.

Promoted `Map/MapMakeData.cpp` from `NonMatching` to `Equivalent`.

Reason:
- No missing target symbols.
- Reviewed all three nonmatching text functions:
  `TMapCollisionBase::initAllCheckData(short, const float*, unsigned short, const TLiveActor*)`,
  `TMapCollisionBase::updateTrans(const JGeometry::TVec3<float>&)`, and
  `TMapCollisionBase::setCheckData(const float*, const short*, TBGCheckData*, int)`.
- `initAllCheckData` preserves the collision-data allocation, nested mesh
  loops, BG type/actor writes, `param_3 & 2` setup mode, optional per-triangle
  data source, byte flag writes, pointer advances, and `unkC` count.
- `updateTrans` preserves the translation offset from `unk50`, per-check-data
  point/min/max/plane update, grid insertion, and final `unk50` copy.
- `setCheckData` preserves the same indexed vertex loads, point copies, normal
  cross product, zero-normal guard, normalization, plane-distance computation,
  min/max Y updates, and `param_4 != 3` grid insertion.
- Remaining residue is codegen-class only: stack frame/slot layout, helper
  inlining/ownership (`TVec3::sub`, `TBGCheckData::updateTrans`,
  `TBGCheckData::setVertex`), and rogue-include destructor/ctor label drift.

Proof:
- `python configure.py --non-matching && ninja` linked with `MapMakeData` from
  source.
- `python configure.py && ninja` passed and verified `mario.dol: OK`.
