# mario/Map/MapModel

Verdict: equivalent
Time: 2026-06-13 6:31am MNL

## Verdict
equivalent

## Date
2026-06-12 5:29am MNL

## Reason
All target symbols are present and the two nonmatching functions are
codegen-only:

- `TMapModel::initUnderpass()`: same joint/material lookup, texture-coordinate
  setup, `J3DTexMtx` allocation, and Z mode writes. Residue is target-larger
  frame (`0x70` vs `0x40`) with shifted save/restore offsets and local-label
  drift.
- `TMapModel::perform(unsigned long, JDrama::TGraphics*)`: same flag checks,
  Mario-underpass condition, camera matrix calls, material effect-matrix update,
  frame update, and actor perform. Residue is stack-slot placement for the
  camera vectors/matrices plus local-label drift.

`python configure.py --non-matching && ninja` linked cleanly after promoting the
TU.

2026-06-13 6:31am MNL recheck: overview still has no missing target rows, and
`python configure.py --non-matching && ninja` linked from source.

2026-06-13 10:11am MNL recheck:
- Current overview still has no missing target symbols. Full diffs for
  `initUnderpass` and `perform` still preserve the same material/name lookup,
  underpass material setup, tex-matrix allocation, Z-mode writes, Mario/camera
  gates, camera vector/matrix calls, effect-matrix update, frame update, and
  `MActor::perform` call.
- Remaining residue is stack-frame/local-slot placement, helper/rodata label
  ownership, and source-owned weak helper/data drift.
