# Enemy/wireBinder

Verdict: equivalent
Date: 2026-06-13 1:07pm MNL

Reverified current `Object(Equivalent, "Enemy/wireBinder.cpp")` again during
the audit-only sweep.

Reviewed functions:
- `TWireBinder::isEndWire(const TVec3f&, float) const`,
  `TWireBinder::getPoint(TVec3f*, const TVec3f&) const`, and
  `TWireBinder::getDirAtPos(const TVec3f&, float) const`: same wire lookup,
  position projection, endpoint/direction predicates, and point subtraction.
  Residue is stack layout and vector-helper ownership.
- `TWireBinder::bind(TLiveActor*)`: same next-frame position construction,
  wire projection, point-on-wire query, NaN fallback to actor position,
  airborne flag update, and final linear-velocity write. Residue is stack
  layout/helper label drift.
- `TWireBinder::init(const TVec3f&)`: same wire-number lookup, `-1` failure
  return, start/end point subtraction, zero-vector fallback for near-zero
  length, and normalized direction write. Target calls `TVec3::dot`/`scale`
  while source inlines the dot/scale math, but the arithmetic and stores are
  equivalent.

No missing symbols. The source-only `TVec3<float>::sub` row is helper-owner
drift.

Validation:
- Shared proof: `python configure.py --non-matching && ninja` linked from source, then `python configure.py && ninja` restored the normal matching config and verified `build/GMSJ01/mario.dol: OK` at 2026-06-13 1:07pm MNL.
