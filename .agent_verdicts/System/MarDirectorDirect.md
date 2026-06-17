# System/MarDirectorDirect

Verdict: equivalent
Date: 2026-06-14 3:03pm MNL

Promoted `System/MarDirectorDirect.cpp` to `Object(Equivalent, ...)`.

Functional fixes made during audit:
- `TMarDirector::direct()` now clears gamepad flag `0x40`, matching the target
  loop after `TMarioGamePad::updateMeaning()`.
- `TMarDirector::setMario()` now gates nozzle restoration on Mario flag
  `0x8000`, skips the target stage set, restores Rocket for stage `0x3C`, and
  restores saved+Spray in the default path.
- `TMarDirector::updateGameMode()` now uses the target 720-frame camera-demo
  timeout and clears director flag `0x40` when the queue drains.
- `TMarDirector::moveStage()` now sends stages `2, 3, 4, 5, 6, 8` to app-state
  `8` for both `unkE4` and `unkB4`.

Certification:
- `python tools/decomp-diff.py -u mario/System/MarDirectorDirect -s missing`
  reports no missing target symbols.
- The edited `direct()` range now matches the target `rlwinm ..., 26, 24`
  gamepad flag clear.
- `python configure.py --non-matching && ninja` links successfully.
- `python configure.py && ninja` restores the normal matching build and reports
  `build/GMSJ01/mario.dol: OK`.

Remaining diffs are classified as codegen/ownership residue: register and stack
local-copy shape in the large state functions, objdiff local-label drift, and
source-owned helper/destructor/infectious-string extras.
