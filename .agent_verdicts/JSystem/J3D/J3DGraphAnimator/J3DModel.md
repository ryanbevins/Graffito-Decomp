# JSystem/J3D/J3DGraphAnimator/J3DModel audit

Verdict: equivalent

Checked 2026-06-14 1:05pm MNL in AUDIT mode.

Proof:

- Promoted `JSystem/J3D/J3DGraphAnimator/J3DModel.cpp` to
  `Object(Equivalent, ...)` locally.
- `python configure.py --non-matching && ninja` linked successfully with this
  object sourced.
- No missing target symbols remain. `J3DUnit01` and `.sdata-0` now match.

Reviewed nonmatching text:

- `J3DModel::calcWeightEnvelopeMtx()` is behaviorally present after the
  implementation tick. The target uses paired-single row-pair accumulation, but
  each lane computes the same fused multiply-add chain as the scalar source:
  current joint animation matrix times inverse envelope matrix, weighted and
  accumulated, with the `{ 0.0f, 1.0f }` translation lane and the same
  `mEvlpScaleFlagArr[i] &= mScaleFlagArr[index]` update. The 0.0% score is PS
  instruction-selection / frame debt, not an empty-body blocker.
- `makeHierarchy`, `entryTexMtxAnimator`, `entryTevRegAnimator`,
  `setTexNoAnimator`, `setTexMtxAnimator`, `entryModelData`, `update`, `calc`,
  `viewCalc`, and `prepareShapePackets` were reviewed. Remaining differences
  are stack size, saved-register choice, helper/weak ownership labels,
  indexed-load spelling, and local label drift.
- Source-only `J3DTexMtxInfo::operator=` / `J3DMtxCalc` weak helpers are
  symbol-owner debt. They do not leave undefined references and did not block
  the source-link proof.
