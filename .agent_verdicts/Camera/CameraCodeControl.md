Verdict: equivalent
Time: 2026-06-13 6:33am MNL
Unit: mario/Camera/CameraCodeControl
Source: src/Camera/CameraCodeControl.cpp

Reason:
- Reviewed the only nonmatching function,
  `CPolarSubCamera::controlByCameraCode_`. The door-opening path preserves the
  camera-mode/timer guard, mode change, warp angle offset, neutral key update,
  and false result. The normal path preserves the Mario-position copy plus
  vertical offset, cube loop bounds, `isInCube` call, map-tool lookup, mode/tool
  comparison, `changeCamModeSpecifyCamMapTool_` call, output code assignment,
  data-no fallback, and return values.
- Remaining diffs are codegen-class: stack-frame/local-slot layout,
  register allocation, `add+lwz` versus `lwzx`, and argument reload placement.
- No target symbols are missing.

Verification:
- `python configure.py --non-matching && ninja`
- `python configure.py && ninja`
- 2026-06-13 6:33am MNL recheck: overview still has no missing target rows,
  and `python configure.py --non-matching && ninja` linked from source.
- 2026-06-13 10:02am MNL recheck: full `--no-collapse` diff still preserves
  the same door-opening camera-mode/timer guard, mode change, warp angle update,
  neutral key reset, Mario-position height offset, cube loop, `isInCube`,
  camera-map-tool lookup, mode/tool comparison, optional
  `changeCamModeSpecifyCamMapTool_`, output code store, data-no fallback, and
  return values. Remaining differences are frame/register allocation,
  `add+lwz` versus `lwzx`, and argument reload placement. Source unchanged
  since the 10:00am proof batch, which passed both `--non-matching` source link
  and normal `mario.dol: OK`.
- 2026-06-15 11:44pm MNL safety recheck: current full `--no-collapse` diff
  still has the same door-opening path, neutral-key reset, Mario position +75
  local, cube loop bounds, `isInCube`, map-tool lookup, mode/tool comparison,
  optional camera-mode change, output code writes, data-no fallback, and return
  values. No missing target symbols. The `--non-matching` proof from the
  `Enemy/fruitsboat` certification tick linked all current `Equivalent` rows
  from source, and the following normal build verified `build/GMSJ01/mario.dol:
  OK`.
