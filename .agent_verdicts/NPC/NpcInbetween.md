# mario/NPC/NpcInbetween

Verdict: equivalent
Date: 2026-06-13 6:19am MNL

Reason:
- `TNpcInbetween::execMotionBlend(MActor*)` preserves the same forced-blend
  path, motion-blend countdown path, old-motion animation/frame updates, ratio
  calculation, and final `setMotionBlendRatio` call. Differences are frame
  size, register/FPR coloring, stack slots for the local `J3DFrameCtrl`, and
  local-label ownership.
- `TNpcInbetween::execPosInbetween(TVec3*)` preserves the same current-position
  snapshot, timer decrement, interpolation ratio, x/y/z lerp stores, and
  reset/target update path. Differences are frame size, register/FPR coloring,
  and constant-label ownership.
- Extra `J3DFrameCtrl` weak symbols are target-absent but unused; source-link
  validation succeeds.
- Source-link proof passed under `python configure.py --non-matching && ninja`,
  then normal `python configure.py && ninja` restored the matching config and
  verified `build/GMSJ01/mario.dol: OK`.

2026-06-13 10:11am MNL recheck:
- Current overview still has no missing target symbols. `execMotionBlend` keeps
  the same forced-blend and countdown paths, old-motion frame update, ratio
  calculation, and `setMotionBlendRatio` call. `execPosInbetween` keeps the
  same snapshot, timer decrement, interpolation, target stores, and reset path.
- Remaining residue is frame size, stack slots, register/FPR coloring, and
  local-label ownership.

Offending functions: none.
