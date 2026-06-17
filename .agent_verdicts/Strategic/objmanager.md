# mario/Strategic/objmanager

Verdict: equivalent
Date: 2026-06-13 6:19am MNL

Reason:
- `TObjManager::load(JSUMemoryInputStream&)` keeps the same base load,
  string read, `TNameRefGen` search, capacity read, and actor-pointer-array
  allocation. The only visible difference is the local string buffer stack
  slot (`0x2c` target vs `0x30` build).
- `TObjManager::perform(unsigned long, JDrama::TGraphics*)` keeps the same
  timer guards, object iteration, `testPerform` calls, and timer append calls.
  Residue is the known `TTimeRec` caller frame-inflation family and derived
  stack offsets.
- Other owned functions and target strings/data match; extras are source-owned
  weak/base helpers and infectious strings/data ownership.
- Source-link proof passed under `python configure.py --non-matching && ninja`,
  then normal `python configure.py && ninja` restored the matching config and
  verified `build/GMSJ01/mario.dol: OK`.

2026-06-13 10:11am MNL recheck:
- Current overview still has no missing target symbols. `load` keeps the same
  base load, name string read/search, capacity read, and pointer-array
  allocation. `perform` keeps the same timer guards, object iteration,
  `testPerform` calls, and timer append calls.
- Remaining residue is stack-slot placement, the known `TTimeRec` caller frame
  family, and source-owned weak/base helper/data ownership.

Offending functions: none.
