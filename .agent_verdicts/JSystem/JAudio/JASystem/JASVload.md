# JSystem/JAudio/JASystem/JASVload

Verdict: equivalent
Date: 2026-06-15 12:55pm MNL
Unit: `mario/JSystem/JAudio/JASystem/JASVload`
Source: `src/JSystem/JAudio/JASystem/JASVload.cpp`
Classification: `Object(Equivalent, "JSystem/JAudio/JASystem/JASVload.cpp")`

Reason: secondary safety-net re-verification kept the existing certification.
The overview has no missing target symbols. All target-owned functions match
exactly except `JASystem::Vload::loadFile` and `loadFileAsync`, and both diffs
are codegen-class only.

Reviewed behavior:
- `loadFile()` and `loadFileAsync()` extract the archive index from the high
  word, fetch the real handle, add the caller offset to the entry file offset,
  build `vlDirName[index] + "/" + vlArc[index]->unk10`, and call
  `JASystem::Dvd::loadToDramDvdT` with the same arguments as target.
- `loadFile()` waits until the volatile result word is nonzero and returns it.
- `loadFileAsync()` returns the requested size after scheduling the load.

Remaining debt: saved-GPR coloring for archive-index/offset temporaries and
local label names. Extra helper/accessor bodies are unused source-owned API
drift.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored normal config and passed
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.
