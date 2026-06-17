# Camera/CameraWarp

Verdict: equivalent
Date: 2026-06-13 1:07pm MNL

Reverified current `Object(Equivalent, "Camera/CameraWarp.cpp")` again during
the audit-only sweep.

Reviewed functions:
- `CPolarSubCamera::addMoveCameraAndMario(const Vec&)`: same delta applied to
  camera position/at fields (`0x10`, `0x3c`, `0x124`, `0x148`), Mario camera
  position, the inbetween camera, and saved/current camera state vectors
  (`0x80`, `0x8c`, `0x98`, `0xb4`, `0xc0`, `0xcc`). Low score is helper and
  temporary source shape, not behavior.
- `CPolarSubCamera::warpPosAndAt(float, short)`: same mode guard, save-param
  copy, usual-lookat capture, distance clamp for L-button and normal camera
  modes, angle/distance recompute, `CLBPolarToCross`, second mode guard,
  height-pan kill, state field synchronization, inbetween warp/reset,
  `calcNowTargetFromPosAndAt_`, and state snapshot copy. Residue is stack and
  FPR/register allocation.
- `CPolarSubCamera::warpPosAndAt(const Vec&, const Vec&)` already byte-matches.

Validation:
- Shared proof: `python configure.py --non-matching && ninja` linked from source, then `python configure.py && ninja` restored the normal matching config and verified `build/GMSJ01/mario.dol: OK` at 2026-06-13 1:07pm MNL.
