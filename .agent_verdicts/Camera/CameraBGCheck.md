# Camera/CameraBGCheck audit

Verdict: equivalent
Checked: 2026-06-14 5:18pm MNL

`mario/Camera/CameraBGCheck` is functionally equivalent and links from source.

Proof:

- `python configure.py --non-matching && ninja` passed after promoting
  `Camera/CameraBGCheck.cpp` to `Object(Equivalent, ...)`.
- This tick's `python configure.py --non-matching && ninja` again linked all
  current `Equivalent` objects, including `CameraBGCheck`, from source.
- This tick's `python configure.py && ninja` restored normal config and passed
  `build/GMSJ01/mario.dol: OK`.

Reviewed nonmatching functions:

- `CPolarSubCamera::execGroundCheck_(Vec)`: same option `0xA4` gap read,
  `CLBLinearInbetween` call, mode `0x2A` clamps, ground query, valid camera
  clip filter, Y snap store, and return flag. Remaining drift is frame size,
  stack slot/register choices, and an equivalent address temp for `0x84`.
- `CPolarSubCamera::execRoofCheck_(Vec)`: same Monte synthetic roof path,
  `checkRoof` path, valid clip test, option `0xF4` subtraction, Y clamp/store,
  and return flag. Remaining drift is stack/register/address-temp codegen.
- `CPolarSubCamera::execWallCheck_(Vec*)`: the prior NaN-sensitive radius guard
  is fixed; target and source now use `fcmpo absD, radius; bge continue`.
  Remaining drift is frame size, wall-record stack offsets, loop/index register
  allocation, and vector temp placement. Wall validity filtering, push-rate
  load, camera X/Z stores, output `Vec` X/Z stores, and return flag match.
- `CPolarSubCamera::isNeedGroundCheck_()`: same disable gates, normal/tower
  mode calls, max sine-height comparison, timer floor to `0x78`, and return
  behavior. Remaining drift is address-temp insertion and FPR/source-shape for
  the max expression.
- `CPolarSubCamera::calcInHouseNo_(bool)`: same cached-position early-out,
  suppression predicates, `0x2CA = -1` skip path, near-nine sample setup,
  offset sample fill, two-height ground probe loop, `0x600` indoor hit test,
  `mData` write to `0x2CA`, and shared timer update paths. Remaining drift is
  frame/register layout, stack slots, FPR coloring, and loop/source shape.

Data/symbol notes:

- No missing symbols are reported. The only extra is
  `JGeometry::TVec3<float>::TVec3()`, a target-absent helper owner with no link
  undefined.
- `.sdata2` contains the same constants used by the functions but in different
  local-label/order layout; this is byte debt, not behavior debt.
