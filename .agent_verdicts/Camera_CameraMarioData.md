## Verdict: equivalent

Date: 2026-06-13 8:17pm MNL

Reason: the prior source-link blocker is fixed. `CameraMarioData.cpp` now owns
the global `.sbss` pointer `TCameraMarioData* gpCameraMario`, the current
overview has no missing/extra artifacts, and all remaining text diffs are
codegen-class:

- `isMarioLeanMirror()`: stack-frame-only (`0x20` target vs `0x18` build).
- `isMarioIndoor()`: same type set test
  (`0x105`, `0x106`, `0x108`, `0x109`); build folds
  `type == 0x105 || type == 0x106` into `(u16)(type - 0x105) <= 1`.
- `calcAndSetMarioData()`: same status switch, movement deltas, clamps, status
  timer update, water-gun/nozzle calls, `CLBCalcRatio<s16>` call, and ratio
  clamp; remaining drift is target frame/register coloring and local constant
  labels.

Proof: `python configure.py --non-matching && ninja` linked from source, then
`python configure.py && ninja` restored the matching config and passed with
`build/GMSJ01/mario.dol: OK`.
