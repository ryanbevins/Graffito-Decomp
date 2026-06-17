# Enemy/beam Audit

verdict: equivalent
date: 2026-06-13 10:36pm MNL
unit: Enemy/beam

Reverified during tick 691. The current overview still has one nonmatching text
function, `TConeBeam::calcVertices(int)`, plus data/constant placement drift.
The existing behavior review below still holds: no missing target symbols and
no structural behavior gap. The `--non-matching` proof run for this tick linked
the whole source-linked set successfully, including `Enemy/beam`.

## 2026-06-13 9:29am MNL - refreshed

Verdict remains `equivalent`.

Re-read the full `--no-collapse` diff for
`TConeBeam::calcVertices(int)`. The residual differences are still stack/FPR
allocation, local temp placement, and equivalent expression scheduling in the
ray/plane intersection math. Raw objdump confirmed the apparent
`drawConeBeamAux` labels in the pretty diff are symbol-label drift: both target
and source call `scale__Q29JGeometry8TVec3<f>Ff` at the vertex-axis scale sites.

## 2026-06-12 9:42pm MNL - equivalent

Verdict: `equivalent`.

Promoted `Enemy/beam.cpp` from `NonMatching` to `Equivalent`.

Reason:
- No missing target symbols.
- The only nonmatching text function is `TConeBeam::calcVertices(int)`.
- Reviewed the full `--no-collapse` diff. Both branches preserve behavior:
  the no-background loop builds vertices from the two perpendicular axes,
  scale, sin/cos angle step, and `unk0C`; the background loop loads the plane
  normal/distance, computes cone angle with `matan`, normalizes the cone axis,
  intersects each cone ray against the plane, and writes `mVtx[i]`.
- Remaining residue is codegen-class only: stack frame/slot layout, saved FPR
  allocation, local helper inlining/ownership (`coneInPlane`, `TVec3::scale`),
  and constant-label ownership.

Proof:
- `python configure.py --non-matching && ninja` linked with `Enemy/beam` from
  source.
- `python configure.py && ninja` passed and verified `mario.dol: OK`.
