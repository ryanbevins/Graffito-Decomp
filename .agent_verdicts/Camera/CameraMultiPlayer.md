# mario/Camera/CameraMultiPlayer

Verdict: equivalent
Date: 2026-06-13 1:07pm MNL

Reason:
- Reverified current `Object(Equivalent, "Camera/CameraMultiPlayer.cpp")` again
  during the audit-only sweep.
- `CPolarSubCamera::ctrlMultiPlayerCamera_()` is equivalent. Both sides fall back to current target/position when count is zero, otherwise sum all player positions, average them, add the camera Y offset, compute max pairwise distance squared, approximate the root, clamp the resulting distance, interpolate the angle, and call `CLBPolarToCross`.
- `removeMultiPlayer()` and `addMultiPlayer()` implement the same container count/capacity behavior and slot compaction/append logic; differences are register allocation and early-return layout.
- `createMultiPlayer()` constructs the same container and `TMultiPlayerData` array. The objdiff label on the constructor pointer is misleading; the source expression is the same array constructor path.

Proof:
- `python tools/decomp-diff.py -u mario/Camera/CameraMultiPlayer`
- `python tools/decomp-diff.py -u mario/Camera/CameraMultiPlayer -d "ctrlMultiPlayerCamera" --no-collapse`
- `python tools/decomp-diff.py -u mario/Camera/CameraMultiPlayer -d "removeMultiPlayer" --no-collapse`
- `python tools/decomp-diff.py -u mario/Camera/CameraMultiPlayer -d "addMultiPlayer" --no-collapse`
- `python tools/decomp-diff.py -u mario/Camera/CameraMultiPlayer -d "createMultiPlayer" --no-collapse`
- Shared proof: `python configure.py --non-matching && ninja` passed, then `python configure.py && ninja` restored the normal matching config and verified `build/GMSJ01/mario.dol: OK` at 2026-06-13 1:07pm MNL.
