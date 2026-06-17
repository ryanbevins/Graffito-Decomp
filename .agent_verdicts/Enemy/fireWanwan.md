# Enemy/fireWanwan

Verdict: equivalent
Date: 2026-06-15 8:10pm MNL

Audit promoted `Enemy/fireWanwan.cpp` to `Equivalent` after fixing one real
shared behavior bug. The target-owned weak
`JGeometry::TQuat4<float>::setRotate(from, to, amount)` in this TU stores the
identity quaternion whenever the cross-product length is `<= epsilon`; it does
not perform an anti-parallel +Y-axis PI fallback. Source now matches that
behavior, and the helper diff rose to 98.8%, with only register/FPR/constant
label drift left.

No missing target symbols remain. The exact target helper owners from the
implementation pass are still present:

- `JGeometry::TVec4<float>::TVec4()` — 100%, 4B
- `TTakeActor::isTaken() const` — 100%, 28B
- `@unnamed@::ArrayWrapper<TTailRubber::Node>::operator[](int) const` — 100%, 16B
- `@unnamed@::ArrayWrapper<TTailRubber::Node>::size() const` — 100%, 8B

Reviewed remaining low/non-exact rows as behavior-equivalent codegen debt:
`TQuat4::slerp`, `TNerveFireWanwanRecoverGraph`, `TNerveFireWanwanFindMario`,
`TFireWanwan::bindBody`, `TFireWanwan::moveObject`,
`TFireWanwanTailNode::perform`, `TTailRubber::adjust/restrict`, and
tail-hit perform/init helpers differ by helper-boundary choices, inline versus
call shape, frame/register/FPR placement, and local-label/data-label drift.
Source-only empty helpers (`clipNodes`, `bindBody`, `getBodyNthPos`,
`getBodyTailPow`, `getBodyHeadPow`, `getTailLength`) remain unreferenced extras
and are not target-owned behavior.

Proof:

- `python tools/decomp-diff.py -u mario/Enemy/fireWanwan -s missing` reports no
  missing rows.
- `python configure.py --non-matching && ninja` linked with
  `Enemy/fireWanwan` sourced.
- Plain `python configure.py && ninja` passed `build/GMSJ01/mario.dol: OK`.
