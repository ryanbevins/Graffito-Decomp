# JSystem/JDrama/JDRSmJ3DAct

Verdict: equivalent
Date: 2026-06-13 2:21pm MNL

Refreshed existing `Object(Equivalent, "JSystem/JDrama/JDRSmJ3DAct.cpp")`
during the AUDIT sweep. Current source-link proof was covered by this tick's
successful `python configure.py --non-matching && ninja` batch.

Reviewed functions:
- `JDrama::TSmJ3DAct::load(JSUMemoryInputStream&)` byte-matches.
- `JDrama::TSmJ3DAct::perform(unsigned long, JDrama::TGraphics*)` preserves
  the same behavior: on perform bit `2`, it builds translation plus Z/Y/X
  Euler rotation matrices, copies the result into the model base transform,
  copies actor scaling into the model base scale, then either calls model
  `calc()` directly or updates the frame controller, writes the animation
  frame, temporarily installs `unk54` as joint 0's matrix calc, calls `calc()`,
  and restores the previous matrix calc. It then calls model `entry()` on bit
  `0x200` and `viewCalc()` on bit `4`. Residue is matrix temporary layout,
  stack frame size, register/FPR allocation, and source-owned weak matrix
  helper emission.
- `JDrama::TSmJ3DAct::~TSmJ3DAct()`, the adjustor thunk, and
  `J3DFrameCtrl::~J3DFrameCtrl()` byte-match.
- Remaining rodata/data differences are source ownership of weak helpers and
  local label ordering; source-link validation accepted them.

Validation:
- `python configure.py --non-matching && ninja` linked successfully with
  `JDRSmJ3DAct` from source.
- `python configure.py && ninja` restored the matching config and verified
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.
