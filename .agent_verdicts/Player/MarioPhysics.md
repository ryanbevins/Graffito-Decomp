# mario/Player/MarioPhysics

Verdict: equivalent
Status: equivalent
Time: 2026-06-15 3:09pm MNL

## Reason
Certified behavior-equivalent after fixing the two audit blockers found in
`TMario::waitProcess()` and `TMario::walkProcess()`.

Fixes made during audit:
- Horizontal extrapolation now uses the target `0.25f` frame step before the
  ground-normal multiply (`mVel.x/z * 0.25f * normalY`), instead of the old
  source's optimized-away `1.0f`.
- `mModelFaceAngle = mFaceAngle.y` now happens before converting
  `checkGroundAtWalking()` result `3` into return `2`, matching the target's
  unconditional model-facing update on that path.

No missing target symbols remain:
- `python tools/decomp-diff.py -u mario/Player/MarioPhysics -s missing` reports
  no rows.

Proof:
- `python configure.py --non-matching && ninja` linked with
  `Player/MarioPhysics.cpp` sourced.
- `python configure.py && ninja` passed with `build/GMSJ01/mario.dol: OK`.

Remaining non-exact rows are codegen/data debt: stack frame size and local-slot
placement, FPR/GPR register choices, sqrt/FMA expression shape, vector helper
call boundaries (`TVec3::sub`/`scale` in target vs inlined component math),
branch layout/condition encoding, source-only JSUList destructor owners, and
local rodata/static-init label ownership.
