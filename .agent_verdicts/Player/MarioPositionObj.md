# Player/MarioPositionObj audit

Verdict: equivalent
Date: 2026-06-13 10:28am MNL

Reason: all functions are either byte-matching or behaviorally aligned, and
`python configure.py --non-matching && ninja` linked the TU from source.

Function review:
- `TMarioPositionObj::load(JSUMemoryInputStream&)`: calls the base load, then
  loops while `i < 8` and stream length exceeds position + `0x24`; each
  iteration reads the name string, three position floats into `unk10[i]`, three
  rotation/position floats into `unk70[i]`, and three discarded floats, then
  stores the final count to `unkD0`. Remaining drift is stack size, register
  coloring, and dummy-local stack-slot placement.
- `TMarioPositionObj::perform(unsigned long, JDrama::TGraphics*)`: byte-matches
  as an empty weak function.
- `TMarioPositionObj::~TMarioPositionObj()`: byte-matches.

Notes:
- Source emits extra `JSUMemoryInputStream::getPosition/getLength` and
  `JDrama::TViewObj` weak owners, but the required source-link proof passed.
- 2026-06-13 10:28am MNL recheck: full current diff still preserves the
  loader's base-call, bounded stream loop, three float groups, discarded dummy
  group, and final count store. The residue remains register, frame, and
  dummy-local stack-slot placement. Reused the current proof batch:
  `python configure.py --non-matching && ninja`, then normal
  `python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
