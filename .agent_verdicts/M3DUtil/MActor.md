# M3DUtil/MActor audit

Verdict: equivalent  
Date: 2026-06-13 8:27am MNL

Reason: all target behavior-bearing symbols are present and
`python configure.py --non-matching && ninja` links successfully with
`M3DUtil/MActor.cpp` as `Object(Equivalent, ...)`.

Reviewed nonmatching functions:
- `MActor::updateMatAnm`, `updateOut`, `updateIn`, `frameUpdate`, `entry`,
  `calc`, and `calcAnm`: animation array traversal, virtual update calls,
  entry/calc guards, draw-buffer reset path, and loop bounds match; residue is
  stack-frame size.
- `MActor::checkCurAnm`: lookup/name compare path and final current-index
  comparison match; residue is stack size and compare operand order.
- `MActor::perform`: flag-gated update/entry/calc/light paths and final
  draw-buffer reset call match; residue is stack-frame size.
- `MActor::setLightData`: light-type guard, `SMS_CalcMatAnmAndMakeDL` call,
  light draw-buffer lookup, and `unk3C` stores match; residue is stack-frame
  size.
- `MActor::isCurAnmAlreadyEnd`: frame-controller null/default true path,
  completion/loop checks, end-frame float compare, and return value match;
  residue is stack slot placement.
- `MActor::setModel`: model/material setup, texgen scan, animation
  `setModel`/material-ID init loops, sub-animation model loop, material-anm
  creation path, `initDL`, and sample-model-data guard match; residue is stack
  slots and 8-bit/16-bit loop-index register coloring.
- `MActor::MActor`: field initialization, animation allocation/setup blocks,
  material animation setup calls, sub-animation list loop, and return match;
  residue is stack slot/constant-label ownership.

Notes:
- Extra emitted animation-base and frame-controller destructors/vtable data are
  weak ownership residue, not missing target behavior.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` passed afterward and verified
  `build/GMSJ01/mario.dol: OK`.

2026-06-13 12:54pm MNL recheck:
- Current overview still has no missing target symbols.
- Re-read the nonmatching text set, including the corrected
  `MActor::checkCurAnm(const char*, int)` overload. The animation traversal,
  virtual update calls, flag-gated perform/entry/calc paths, light data setup,
  current-animation lookup, frame-controller end checks, model/material setup,
  animation `setModel` loops, material-animation setup, `initDL`, and
  constructor allocation/setup blocks still match behavior.
- Remaining drift is stack-frame/save-slot size, 8-bit versus 16-bit loop-index
  coloring in `setModel`, compare operand order in `checkCurAnm`, local
  constant-label ownership, and weak destructor/vtable ownership. Reused this
  tick's successful source-link and normal DOL proof batch.
