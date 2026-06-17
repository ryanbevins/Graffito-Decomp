# Map/PollutionObj audit

Verdict: equivalent  
Date: 2026-06-13 6:45am MNL
Status: reverified current source-link proof

Proof: `python configure.py --non-matching && ninja` links cleanly with
`Map/PollutionObj.cpp` built from source. All target `.text`, vtable/data, and
static-init symbols are present. A follow-up plain `python configure.py &&
ninja` restored the matching configuration and verified
`build/GMSJ01/mario.dol: OK`.

Reason: Non-100% function diffs are codegen-class
stack-frame/slot, register-coloring, FPR-coloring, and local label residue.

Reviewed functions:
- `TPollutionObj::initAreaInfo(TPollutionLayer*)`: same layer assignment,
  virtual tex-coordinate calls, bounds clamps, and child recursion; residue is
  stack frame size and local symbol labels.
- `TPollutionObj::updateDepthMap()`: same nested map loops and
  `setDepth(x, y, getDepthFromMap(x, y))` path; residue is stack frame size and
  local call-label presentation.
- `TPollutionObj::getDepthFromMap(int, int)`: same coordinate scaling,
  four-corner `checkGround` calls, near-height tests, center `checkGround`,
  `worldToDepth`, and `0xff` fallback; residue is stack frame size and
  FPR/register coloring.

Notes: source emits extra unused JSUList weak destructors because of the rogue
MSound includes needed for matching static init/BSS, but the source-link build
succeeds and the behavior-visible symbols are present.

2026-06-13 10:47am MNL recheck: verdict remains `equivalent`. Re-read the
current diffs for `initAreaInfo`, `updateDepthMap`, and `getDepthFromMap`.
The area setup still assigns the layer, calls the same virtual tex-coordinate
helpers, clamps bounds, and recurses through children. The depth-map update
still runs the same nested loops and `setDepth(x, y, getDepthFromMap(x, y))`
path. `getDepthFromMap` still computes the same scaled coordinates, performs
the four corner `checkGround` calls, near-height tests, center ground check,
`worldToDepth`, and `0xff` fallback. Residue is stack frame/slot and FPR/GPR
coloring. Proof refreshed with `python configure.py --non-matching && ninja`,
then normal `python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
