# mario/Camera/CameraMode

Verdict: equivalent  
Date: 2026-06-14 12:36am MNL

Promoted `Camera/CameraMode.cpp` to `Object(Equivalent, ...)` after
recovering the required `CPolarSubCamera::mCamKindNameSaveFile[0x49]` data
table from `build/GMSJ01/asm/Camera/CameraMode.s`.

Text review:
- `isLButtonCameraInbetween`, `isTalkCameraInbetween`, and
  `isNormalCameraCompletely` remain non-byte-matching, but the reviewed diffs
  are branch-layout and bool materialization/codegen residue. The tested mode
  sets and `isNowInbetween()` gate semantics match the original.
- `isSlopeCameraMode` and the other predicate helpers are byte-matching or
  behavior-equivalent.

Data review:
- `mCamKindNameSaveFile` now has all 73 camera kind `.prm` paths in the exact
  pointer order from the original data table.
- The previous source-link blocker in `CPolarSubCamera::CPolarSubCamera` is
  resolved; `cameragc.o` can now reference the table from source-linked
  `CameraMode.o`.

Proof:
- `python configure.py --non-matching && ninja` linked `mario.elf` and built
  `mario.dol`.
- `python configure.py && ninja` passed with `build/GMSJ01/mario.dol: OK`.
