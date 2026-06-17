# mario/JSystem/JSupport/JSUList

Verdict: matching
Date: 2026-06-13 4:13am MNL

Reason:
- `python tools/decomp-diff.py -u mario/JSystem/JSupport/JSUList` reports
  all nine text symbols as byte-matching with no nonmatching, missing, or extra
  rows.
- Promoted `JSystem/JSupport/JSUList.cpp` from `Object(Equivalent, ...)` to
  `Object(Matching, ...)`; the normal `python configure.py && ninja` build
  passed and verified `mario.dol: OK`.

Offending functions: none.
