# Enemy/BossHanachanSub

Verdict: equivalent
Date: 2026-06-14 5:58pm MNL

Certified after the owner fix and one audit-time behavior correction:
`BHSCalcCentrifugalForce()` now returns zero when the turn ratio is exactly
`0.5f`, matching the target's `ratio >= 0.5f` branch.

The rebuilt object owns a 100% matching 28B weak
`TBGCheckData::isIllegalData() const` body in target order, and both proof
builds passed after promotion:

- `python configure.py --non-matching && ninja`
- `python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`

Current overview:

- no missing target text symbols remain;
- `TBGCheckData::isIllegalData() const` is `100.0%`;
- `TSpherePoint::TSpherePoint()`, `TWaterHitActor::onWaterHitCounter()`, and
  `TWaterHitActor::__vtable` are exact;
- remaining text diffs are in `TSphereLink` construction/move/revision math,
  `TWaterHitActor::receiveMessage`, and the two BHS math helpers.

Implementation review:

- `TSphereLink::moveHead()` preserves the target behavior: gravity/velocity
  integration for every point, head assignment, XZ wall movement, ground-water
  ignore checks guarded by `ground != nullptr && !ground->isIllegalData()`,
  per-segment normalization or `(0,1,0)` fallback, segment-length scaling,
  chained position update, second wall/ground pass, and velocity/previous
  position refresh.
- `TSphereLink::setDegreeZAndRevisionPosXZ()` preserves the target behavior:
  early no-op on unchanged degree, previous-segment/base-angle calculation,
  wrap into `[0,360)`, degree delta scaled by `m14`, rounded JMA angle, and
  X/Z revision by sin/cos.
- `TSphereLink::TSphereLink(...)` preserves count/allocation, field stores,
  angle-derived spacing, first point from `pos`, subsequent points offset from
  the previous point, zero velocity, segment length, and zero degree.
- `TWaterHitActor::receiveMessage()` preserves the message `0xf` gate and
  `gpMarDirector->unk124` acceptance for states `1`, `2`, or `4`, clearing or
  setting the water-hit counter accordingly.
- `BHSCalcRevisionDistXZByRotateZ()` and `BHSCalcCentrifugalForce()` preserve
  the same angle/quadrant conversion, rounding, threshold, sign, and JMA table
  semantics. The strict `0.5f` threshold edge was corrected during audit.

Remaining residue for audit/investigation:

- stack-frame and saved-register/FPR allocation differences;
- objdiff local-label drift in `.sdata2`;
- helper-boundary differences around `TVec3::sub`, `TVec3::scale`,
  `TVec3::add`, `TVec3::operator-=`, and `CLBRoundf<short>`;
- extra unreferenced weak/helper owners (`TWaterHitActor` destructor,
  JStage adjustors, vector helpers).
