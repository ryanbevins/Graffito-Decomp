# Enemy/BathtubBinder audit

Verdict: equivalent

Checked 2026-06-14 2:11pm MNL during AUDIT source-link pass.

Unit: `mario/Enemy/BathtubBinder`

Proof:
- Promoted `Enemy/BathtubBinder.cpp` to `Object(Equivalent, ...)`.
- `python configure.py --non-matching && ninja` linked with the source object.
- `python configure.py && ninja` restored the normal matching build and passed
  `build/GMSJ01/mario.dol: OK`.

Reason:
- No missing target symbols remain; the local
  `JGeometry::TVec3<float>::set<float>(float, float, float)` helper is present
  and 100% matching.
- `TBathtubBinder::float_(TLiveActor*)` performs the target behavior: rotation
  matrix sampling, three bathtub-radius clamp blocks, water-height sampling and
  interpolation, pitch clamp/ease, final XZ clamp, and base-height floor.
- Remaining text diffs are codegen-class: stack-frame size, GPR/FPR coloring,
  local helper/copy-temp shape, branch spelling around frsqrte zero handling,
  and local symbol ordering.
- Remaining data diffs are ownership/layout debt from source-only infectious
  string/small-data rows (`MtxCalcTypeName`, `dummyMactorStringValue1`,
  `SMS_NO_MEMORY_MESSAGE`, `.sdata-0`) and do not affect behavior.
