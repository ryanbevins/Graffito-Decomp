# Player/MarioWait Audit

Status: equivalent
Updated: 2026-06-15 9:16am MNL
Unit: `mario/Player/MarioWait`
Source: `src/Player/MarioWait.cpp`
Configure: `Object(Equivalent, "Player/MarioWait.cpp")`

## Verdict

Certified functionally equivalent. The remaining nonmatching rows are codegen,
helper-boundary, register/stack-frame, or local constant-pool layout debt; no
remaining structural behavior difference was found.

## Reviewed Rows

- `TMario::waitMain()` 93.2%: same top-level setup, held-object wall checks,
  actor-type status transition, switch dispatch, helper calls, input masks,
  status changes, animation IDs, and return values. Raw target asm confirms the
  noisy `0x80000A36` throw-end region calls `checkThrowObject()` then
  `jumpEndCommon(0x65, 0x0C400201)`; decomp-diff's displayed call labels there
  are an address-drift artifact.
- `TMario::squating()` 95.9%: same input/status exits, water-gun/nozzle checks,
  hip-drop rumble/status, side-walk output handling, trig position updates,
  analog squat turn math, and final `waitProcess()`. Residue is stack frame,
  local placement, and GPR choices around `mWaterGun`/`mGamePad`.
- `TMario::waiting()` 100.0% fuzzy: same waiting-common gate, Mario/pump/sleep
  path, ground normal and timer checks, Monteman wait, low-health wait, and
  regular wait animation path. Remaining row is frame/label residue.
- `TMario::waitingCommonEvents()` 100.0% fuzzy: same input priorities, rotate
  jump/start paths, IConverge yaw update, can-squat path, rocket target setup,
  and final rotate-start return. Residue is frame-size/offset only.
- `TMario::canPut()` 76.2%: same two `gpMap->isTouchedOneWall` checks using the
  face-angle trig projection, held-object radius, and current position. Low
  fuzzy score is load scheduling/register/constant-label shape.
- `TMario::canSleep()`, `TMario::getSideWalkValues()`, and
  `TMario::stopCommon(int, int)`: near-exact; remaining drift is stack-frame,
  local placement, or branch-target offset from helper/layout differences.

## Data / Weak Residue

- Missing target `@3463` is a 4B local constant-pool label; the source object
  emits equivalent constant data under generated local labels plus the existing
  `.sdata2` row.
- Extra rows are weak/list/destructor emissions (`TNozzleBase::getNozzleKind`,
  JSUList destructors) and local constant-pool labels. They do not indicate a
  missing runtime behavior path.

## Proof

- `python configure.py --non-matching && ninja` linked with `MarioWait` sourced.
- `python configure.py && ninja` passed `build/GMSJ01/mario.dol: OK`.
