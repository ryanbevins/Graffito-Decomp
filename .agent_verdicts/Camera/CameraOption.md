# Camera/CameraOption audit

Verdict: equivalent  
Date: 2026-06-13 7:25am MNL

Reason: `Camera/CameraOption.cpp` links from source after commit `3eb4a3a7`
defined the target SBSS owner `gpCameraOption`. `python configure.py
--non-matching && ninja` succeeded with the TU promoted to
`Object(Equivalent, ...)`.

Reviewed functions:
- `moveToDown`, `moveToUp`, and `moveToLoadFromTitle`: byte-identical.
- `TCameraOption::TCameraOption`: same stores, constants, calls, branches, and
  return value; remaining diff is stack-frame/local-slot size plus SDA label
  ownership.
- `CPolarSubCamera::ctrlOptionCamera_`: same option timers, chase calls,
  cube lookup, map-tool transition, final camera copies, and FOV store;
  remaining diff is stack-frame/local-slot size plus SDA label ownership.

Notes:
- Extra source-owned `dummyMactorStringValue1`, `SMS_NO_MEMORY_MESSAGE`, and
  weak `TNameRefAryT<TCameraMapTool, JDrama::TNameRef>::searchF` are
  non-behavioral ownership/layout residue.

Reverified in the current audit sweep. Full current diffs for
`TCameraOption::TCameraOption` and `CPolarSubCamera::ctrlOptionCamera_` still
show the same stores, camera-map-tool lookup/calls, timer chase sequences,
cube-camera transition, final camera-field copies, and FOV store. Residue is
frame/stack-local size plus SDA/local-label ownership. Source link proof passed
in the same batch as the 2026-06-13 7:25am MNL notes refresh.

2026-06-13 10:47am MNL recheck: verdict remains `equivalent`. Re-read the
current constructor and `ctrlOptionCamera_` diffs. The constructor still
initializes the same fields, consults the same camera map tool, runs the same
position/angle helper calls, and falls back identically. `ctrlOptionCamera_`
still performs the same timer-driven chase sequences, Mario/cube-camera lookup,
map-tool transition, final camera-field copies, and FOV store. Remaining drift
is stack-local size and SDA/local-label ownership. Proof refreshed with
`python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
