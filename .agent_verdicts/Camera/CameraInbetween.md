# mario/Camera/CameraInbetween

Verdict: equivalent
Date: 2026-06-13 1:07pm MNL

Reason:
- Reverified current `Object(Equivalent, "Camera/CameraInbetween.cpp")` again
  during the audit-only sweep.
- `TCameraInbetween::execCameraInbetween(...)` is equivalent. The diff is dominated by load order, register choice, equivalent signed-angle subtraction (`local - current`), and a reordered absolute-value check before `CLBPolarToCross`.
- `TCameraInbetween::addMoveCameraAndMario(const Vec&)` is equivalent. Both sides add the same displacement to `mTargetPos`, `mTargetAt`, and `mPrevAt`, then recompute both polar coordinate pairs.
- The `.sdata2` mismatch is constant ordering/layout, not value drift: target/source contain the same effective constants used by the functions (`0.0f`, `0.001f`, `1.0f`, `0.1f`, and the signed-int-to-double literal).

Proof:
- `python tools/decomp-diff.py -u mario/Camera/CameraInbetween`
- `python tools/decomp-diff.py -u mario/Camera/CameraInbetween -d "execCameraInbetween" --no-collapse`
- `python tools/decomp-diff.py -u mario/Camera/CameraInbetween -d "addMoveCameraAndMario" --no-collapse`
- Shared proof: `python configure.py --non-matching && ninja` passed, then `python configure.py && ninja` restored the normal matching config and verified `build/GMSJ01/mario.dol: OK` at 2026-06-13 1:07pm MNL.
