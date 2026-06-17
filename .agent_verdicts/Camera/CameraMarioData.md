# mario/Camera/CameraMarioData

Verdict: equivalent
Date: 2026-06-13 8:17pm MNL

Reason:
- The prior source-link blocker is fixed: `src/Camera/CameraMarioData.cpp`
  now owns the global `.sbss` pointer `TCameraMarioData* gpCameraMario`.
- Current overview has no missing/extra artifacts and all data sections are
  exact, including `gpCameraMario`.
- `isMarioLeanMirror()` is stack-frame-only (`0x20` target vs `0x18` build).
- `isMarioIndoor()` performs the same type set test
  (`0x105`, `0x106`, `0x108`, `0x109`); the build folds the first two cases
  into `(u16)(type - 0x105) <= 1`.
- `calcAndSetMarioData()` has the same status switch, distance stores, clamps,
  status timer update, water-gun/nozzle calls, `CLBCalcRatio<s16>` call, and
  ratio clamp. Remaining text drift is stack/register coloring and local
  constant labels only.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored the matching config and passed with
  `build/GMSJ01/mario.dol: OK`.
