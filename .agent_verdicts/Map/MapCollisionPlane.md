## Map/MapCollisionPlane

Verdict: equivalent
Audited: 2026-06-12 11:01am MNL
Reverified: 2026-06-13 6:48am MNL — current source still links under
`python configure.py --non-matching && ninja`; restored matching build with
`python configure.py && ninja` and DOL hash check passed.

Promoted `Object(NonMatching, "Map/MapCollisionPlane.cpp")` to `Equivalent`.

Evidence:
- `TMapCheckGroundPlane::checkPlaneGround(float, float, float,
  const TBGCheckData**)` is the only nonmatching target function.
- The full diff preserves the same bounds checks and illegal result path, same
  world-to-grid conversions, same tile-local coordinate math, same two-triangle
  selection, and same plane-equation solve/store.
- Remaining differences are frame size, FPR/register coloring, and redundant
  integer-conversion stack traffic. No call, branch condition, constant, or
  memory store differs behaviorally.
- Static init, ctors, and `.sdata2` are byte-exact. Extra JSUList dtor symbols
  are the known rogue-include weak-owner residue and are unused.
- `python configure.py --non-matching && ninja` linked a source DOL with this
  TU enabled.
- `python configure.py && ninja` restored the matching build and passed the
  DOL hash check.

2026-06-13 10:49am MNL recheck: verdict remains `equivalent`. Re-read the
current `checkPlaneGround` diff. Bounds checks, illegal-data return, grid
conversion, tile-local coordinate math, triangle selection, and plane-equation
solve/store still match behaviorally. Remaining drift is frame size,
FPR/register coloring, and redundant integer-conversion stack traffic. Proof
refreshed with `python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
