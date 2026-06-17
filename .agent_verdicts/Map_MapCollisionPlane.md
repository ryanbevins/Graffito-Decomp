# Map/MapCollisionPlane

Verdict: `Equivalent` reverified.

## 2026-06-13 1:29pm MNL

- Proof earlier this tick, before any source/config changes:
  - `python configure.py --non-matching && ninja` linked `mario.elf` and `mario.dol`.
  - `python configure.py && ninja` linked and checksum passed.
- Reviewed `TMapCheckGroundPlane::checkPlaneGround(float, float, float, const TBGCheckData**)` at `95.9%`.
- Behavior matches: extent reject writes `TMapCollisionData::mIllegalCheckData` and returns `-32767.0f`; in-bounds path converts world coordinates to grid indices, computes in-tile offsets, chooses triangle `0` or `1` from the same grid formula, stores the selected `TBGCheckData*`, and solves the plane equation for Y.
- Residual drift is codegen only: frame size, FPR coloring, and integer-conversion spill slot order around the two `fctiwz` conversions.
