## JSystem/JKernel/JKRCompArchive

Verdict: equivalent
Audited: 2026-06-13 9:44am MNL

Promoted `Object(NonMatching, "JSystem/JKernel/JKRCompArchive.cpp")` to
`Equivalent`.

Evidence:
- `JKRCompArchive::open(long)` is a 100.0% fuzzy mismatch with only frame size
  and stack-save offsets differing.
- `JKRCompArchive::fetchResource(void*, unsigned long, SDIFileEntry*,
  unsigned long*)` preserves the same aligned-size calculation, zero-size
  panic, cached-data copy path, compression conversion, in-archive memory
  copy/decompression path, ARAM path, DVD path, bad-mode panic, output-size
  store, and return value. Differences are register coloring and local-label
  offsets.
- The remaining `.data` mismatch is base/weak-owner residue (`JKRFile` vtable
  and local dummy table ownership). Required rodata and `JKRCompArchive`
  vtable data are present.
- Recheck at 7:10am: `open(long)` still differs only by frame/save-slot offsets
  while preserving every allocation, DVD read, ARAM load, table pointer setup,
  cleanup, and mount-state branch. The buffer `fetchResource` overload still
  preserves cache copy, compression conversion, in-memory decompress, ARAM
  fetch, DVD fetch, bad-mode panic, output-size write, and return value; residue
  is GPR coloring and label offsets.
- Recheck at 9:44am: fresh full diffs confirm the same verdict. `open(long)`
  remains behavior-identical with frame/save-slot drift only, and the buffer
  `fetchResource` overload preserves the same cache/decompress/ARAM/DVD/panic
  paths with only GPR coloring and local-label offsets.
- Recheck at 1:21pm: current full diffs again show only codegen-class residue.
  `open(long)` keeps the same archive allocation, DVD load, mount-mode split,
  ARAM side-load, table pointer setup, cleanup, and success flag behavior.
  `fetchResource(void*,...)` keeps the same aligned-size clamp, panic, cached
  copy, compressed decode, ARAM/DVD fallback, output-size store, and return
  value. Remaining drift is frame size, saved-register coloring, and helper/
  vtable owner labels.
- `python configure.py --non-matching && ninja` linked a source DOL with this
  TU enabled.
- `python configure.py && ninja` restored the matching build and passed the
  DOL hash check.
