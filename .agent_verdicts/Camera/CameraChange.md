# Camera/CameraChange

Verdict: equivalent
Date: 2026-06-15 1:43am MNL

`Camera/CameraChange.cpp` is behaviorally equivalent and links from source.

Promotion proof:
- `python configure.py --non-matching && ninja` linked `mario.elf` and built
  `mario.dol` with `Object(Equivalent, "Camera/CameraChange.cpp")`.
- `python configure.py && ninja` restored the matching build and passed
  `build/GMSJ01/mario.dol: OK`.

Audit notes:
- Fixed one real behavioral blocker before certification:
  `CPolarSubCamera::changeCamModeSub_(int, int, bool)` now clears camera flags
  `0x1c` in the post-mode-change non-normal/non-tower branch, matching target
  `rlwinm r0, r0, 0, 30, 26`; the source previously cleared only `0x10`.
- The earlier implementation fixes in
  `CPolarSubCamera::execCameraModeChangeProc_(int)` hold under re-audit:
  the raw target mode-decision chain and early L-button/front-rotate gates are
  represented in source.
- Remaining text diffs are codegen-class: stack-frame sizes, saved-register
  sets, bool materialization, pointer-base caching, branch layout, jump-table
  label placement, and helper call-boundary spelling.
- Data/extra rows are source-link byte debt from included stage/sound-table
  helpers and local static data; no missing target text rows remain, and the
  source-linked build has no undefined symbols.
