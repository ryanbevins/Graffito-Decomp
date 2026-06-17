# MoveBG/MapObjPollution audit

Verdict: equivalent
Date: 2026-06-13 7:56am MNL

Reason: all functions are either byte-matching or behaviorally aligned, and
`python configure.py --non-matching && ninja` linked the TU from source.

Fresh recheck: full pass repeated on 2026-06-13 7:56am MNL. No missing target
text symbols were present; all text mismatches are extra helper/weak owners.
Raw asm confirms the `registerRevivalTexStamp` call receives the same layer
width/height, revival id, and texture resource arguments as the source.

Function review:
- `TMapObjRevivalPollution::loadAfter()`: calls base `loadAfter()`, iterates
  `unk10` revival-polluter entries, fetches the pollution layer, passes the
  layer dimensions, texture/id fields, and resource pointer to
  `registerRevivalTexStamp`, then stores the returned stamp id to `unk8`.
  Remaining drift is stack frame size, register coloring, and helper-owner
  scheduling around the inlined `TRevivalPolluter::registerPolluteTex` body.
- All other text functions, including constructors, `load`, `perform`,
  `TPolluterBase` methods, destructor thunks, and `__sinit_MapObjPollution_cpp`,
  byte-match.

Notes:
- Source emits extra out-of-line helper/weak owners, but the required
  `--non-matching` source-link proof passed.

Reverified: 2026-06-13 10:52am MNL — still equivalent. Re-read
`TMapObjRevivalPollution::loadAfter()`; it still calls base `loadAfter()`,
iterates the same entry count, fetches the same pollution layer, passes the same
texture stamp arguments to `registerRevivalTexStamp`, and stores the returned
stamp id. The apparent argument-order movement is register scheduling only.
Proof passed again with `python configure.py --non-matching && ninja`, then
plain `python configure.py && ninja` restored the matching config and verified
`build/GMSJ01/mario.dol: OK`.
