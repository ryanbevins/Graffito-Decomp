## Verdict: equivalent

Date: 2026-06-13 11:29pm MNL

Stale-cache refresh for current `Object(Equivalent, ...)`.

Current overview:
- No missing or extra symbols.
- `.sdata2` constants are exact.
- Two nonmatching text functions remain: `checkLinesCollision(...)` and
  `TMapCollisionData::polygonIsInGrid(...)`.

Behavior review:
- `checkLinesCollision(...)` performs the same two segment-side tests:
  compute the cross products for the first segment, reject when both endpoints
  are on the same side, then compute the second segment's cross products and
  reject on the same-side condition. Current diff is stack frame/local label
  layout only.
- `polygonIsInGrid(...)` performs the same early true for downward normals,
  the same three polygon-vertex-in-grid tests, the same four grid-corner
  point-in-polygon tests, and the same four grid-edge-vs-polygon collision
  checks. The residual diff is the old frame-size gap (`0x2c0` target vs
  `0xc0` rebuild), saved-register/base-pointer choices, and equivalent
  branch-label layout around the same helper-call sequence.

Proof:
- This tick's `python configure.py --non-matching && ninja` source-linked the
  current `Equivalent` set successfully.
- `python configure.py && ninja` restored the normal matching config and passed
  with `build/GMSJ01/mario.dol: OK`.
