# Camera/cameralib

Verdict: equivalent
Date: 2026-06-15 1:17pm MNL

Reason:
- Promoted `Object(Equivalent, "Camera/cameralib.cpp")`.
- `python tools/decomp-diff.py -u mario/Camera/cameralib -s missing` reports no
  missing target symbols; extra rows are source-only vector/helper owners
  (`TVec3::add/sub/scale/dot/setLength`, `set<f32>`, and local
  `normalizeInner*`) with no source-link undefineds.
- Strict review found no behavioral blockers. Non-exact rows are
  helper-boundary, stack/register/local-placement, and `.sdata2` constant-order
  debt. The raw target asm for `CLBCalcNearNinePos` follows the same camera
  math as the source: normalize the near vector, write the center point,
  extract `matan` angles, build roll-adjusted up/right basis vectors through
  `setRotate`, fill side points, then normalize diagonal vectors for corners.
  Other non-exact rows preserve the same calls, branch conditions, stores, and
  constants.
- `python configure.py --non-matching && ninja` linked with `cameralib` sourced.
- Plain `python configure.py && ninja` passed with `build/GMSJ01/mario.dol: OK`.
