# JSystem/JParticle/JPAMath

## Verdict: equivalent

Date: 2026-06-13 9:11pm MNL

Promoted `JSystem/JParticle/JPAMath.cpp` back to
`Object(Equivalent, ...)` after the refined-sqrt implementation follow-up and
current source-link proof.

Reviewed functions:
- `JPAVecToRotaMtx(float (*)[4], TVec3<float>, TVec3<float>)`: target and
  source now both compute the cross product, dot product, one-step
  Newton-refined reciprocal-square-root length, epsilon fallback to zero axis,
  `__fres` normalization, and the same Rodrigues-style rotation matrix stores.
  Remaining low fuzzy score is stack/register/expression-scheduling debt.
- `JPAConvertFixVecToFloatVec`: same three fix-to-float conversions and stores;
  drift is stack-frame/slot size and constant-label ownership.
- `JPAGetRMtxSTVecElement`: byte-level 100.0% except stack spill slot labels in
  the sqrt local; behavior is identical scale extraction, identity setup,
  guarded normalized-column stores, and translation extraction.
- `JPAGetRMtxTVecElement`: same scale extraction, identity setup, guarded
  normalized-column stores, and translation extraction; drift is frame size,
  FPR coloring, stack slots, and constant labels.

Validation:
- `python tools/decomp-diff.py -u mario/JSystem/JParticle/JPAMath` reported no
  missing or extra symbols and exact `.sdata2`.
- `python configure.py --non-matching && ninja` linked successfully with
  `JPAMath` from source.
- `python configure.py && ninja` passed with `build/GMSJ01/mario.dol: OK`.

## Verdict: ready_for_audit

Date: 2026-06-13 8:59pm MNL

Implementation follow-up fixed the structural blocker from the 8:47pm audit
downgrade.

Changes:
- `JPAVecToRotaMtx(float (*)[4], TVec3<float>, TVec3<float>)` now computes the
  cross-product length with the target's one-step Newton-refined
  reciprocal-square-root path: `frsqrte`, `0.5f`, `3.0f`, then
  `crossLenSq * estimate`.
- The epsilon comparison now owns target `@1671` as `3.8146973e-06f`.
- Vector normalization uses the target `__fres(crossLen)` reciprocal instead of
  full `1.0f / crossLen`.
- Target-absent empty helper bodies (`JPAGetScaleXYRotateMtx`,
  `JPAGetScaleYZRotateMtx`, `JPAGetZRotateMtx`, conversion/bound/extraction
  stubs) were removed from `JPAMath.cpp`.

Current status:
- `python tools/decomp-diff.py -u mario/JSystem/JParticle/JPAMath` reports no
  missing or extra symbols, and `.sdata2` is exact.
- Remaining nonmatching functions are codegen-class by review:
  `JPAVecToRotaMtx` has stack/register/expression scheduling residue over the
  same refined sqrt, fallback, normalization, and final Rodrigues matrix stores;
  `JPAConvertFixVecToFloatVec`, `JPAGetRMtxSTVecElement`, and
  `JPAGetRMtxTVecElement` retain their earlier stack/FPR/constant-label residue.

Proof:
- Temporary local `Object(Equivalent, "JSystem/JParticle/JPAMath.cpp")` passed
  `python configure.py --non-matching && ninja`.
- Restored `configure.py`; normal `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.

## Verdict: not_equivalent

Date: 2026-06-13 8:47pm MNL

Reason: stale `Equivalent` recheck found a real numeric behavioral difference,
so the row was downgraded to `Object(NonMatching, ...)`.

Offending function:
- `JPAVecToRotaMtx(float (*)[4], TVec3<float>, TVec3<float>)`: target computes
  the cross-product length with a Newton-refined reciprocal-square-root path
  using `0.5f` and `3.0f` constants (`@1669`/`@1670`) before the epsilon
  fallback. Current source calls the simpler `JPASqrtf` approximation, whose
  rebuilt object lacks those constants and uses only `x * frsqrte(x)`. The
  matrix formula is otherwise the same, but this changes floating-point
  results and is not merely codegen drift.

Other observed residue remains codegen/data ownership:
- `JPAConvertFixVecToFloatVec`: same three fix-to-float conversions and stores;
  stack-slot and constant-label drift only.
- `JPAGetRMtxSTVecElement` and `JPAGetRMtxTVecElement`: same scale extraction,
  identity setup, guarded normalization stores, and translation extraction;
  stack/FPR/constant-label drift only.
- Extra helper stubs are currently unreferenced by the rebuilt source-link set,
  but they are not the reason for this downgrade.

Proof:
- `python tools/decomp-diff.py -u mario/JSystem/JParticle/JPAMath -d JPAVecToRotaMtx --no-collapse`
  plus raw target/rebuilt disassembly showed the sqrt-constant difference.
- `python configure.py && ninja` passed after the downgrade.

Verdict: equivalent
Date: 2026-06-12 11:29pm MNL

Promoted `JSystem/JParticle/JPAMath.cpp` to `Object(Equivalent, ...)` after
source-link proof.

Reviewed functions:
- Rotation-matrix helpers, `JPASqrtf`, `JPAConvertFixToFloat`,
  `JPAGetKeyFrameValue`, and key-frame interpolation byte-match.
- `JPAVecToRotaMtx(float (*)[4], TVec3<float>, TVec3<float>)` has a low fuzzy
  score but preserves the same cross-product components, dot product,
  cross-length normalization/fallback, and final Rodrigues-style rotation
  matrix stores. Residue is expression scheduling, temporary placement, and
  FPR allocation.
- `JPAConvertFixVecToFloatVec`, `JPAGetRMtxSTVecElement`, and
  `JPAGetRMtxTVecElement` preserve fix-to-float conversion, scale extraction,
  normalized rotation matrix output, and translation extraction. Residue is
  stack slot/FPR allocation and local constant-label ownership.
- Source-only 4-byte helper stubs and small `.sdata2` ownership drift are not
  referenced in a way that blocks source linking.

Validation:
- `python configure.py --non-matching && ninja` linked successfully with
  `JPAMath` from source.
- `python configure.py && ninja` passed and reported `mario.dol: OK`.

2026-06-13 9:24am MNL recheck: full diffs re-reviewed. `JPAVecToRotaMtx`
still computes the same cross product, dot, zero/epsilon fallback, normalized
axis, and final rotation matrix stores; the low fuzzy score is expression
scheduling/FPR/stack and local constant ownership. The fix-vector and rotation
matrix extraction helpers still differ only by stack slots, FPR coloring, and
constant labels. Shared proof passed: `python configure.py --non-matching &&
ninja`, then `python configure.py && ninja` verified `mario.dol: OK`.
