# mario/MSound/MSoundDebug

Verdict: matching
Date: 2026-06-13 6:19am MNL

Reason:
- `JADPrm<unsigned char>::JADPrm(unsigned char, const char*)`,
  `__sinit_MSoundDebug_cpp`, and the target `.ctors` row match exactly.
- The only objdiff extras are source-owned weak `JSUList<T>::~JSUList()` bodies
  from included sound-list types; no target-owned function calls or data
  semantics differ.
- Source-link proof passed under `python configure.py --non-matching && ninja`,
  then normal `python configure.py && ninja` restored the matching config and
  verified `build/GMSJ01/mario.dol: OK`.

- 2026-06-13 8:08am MNL promotion: changed the row from `Object(Equivalent, ...)` to `Object(Matching, ...)`; the normal `python configure.py && ninja` build passed and verified `build/GMSJ01/mario.dol: OK`.

Offending functions: none.
