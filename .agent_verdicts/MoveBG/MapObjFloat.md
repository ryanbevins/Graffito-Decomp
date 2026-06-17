## MoveBG/MapObjFloat

Verdict: equivalent
Audited: 2026-06-12 10:59am MNL
Reverified: 2026-06-13 6:48am MNL — current source still links under
`python configure.py --non-matching && ninja`; restored matching build with
`python configure.py && ninja` and DOL hash check passed.

Promoted `Object(NonMatching, "MoveBG/MapObjFloat.cpp")` to `Equivalent`.

Evidence:
- `TMapObjFloatOnSea::initMapObj()` is the only nonmatching `.text`
  symbol. The full diff shows the same `TLeanBlock::initMapObj()` call,
  same `param_table` lookup by `strcmp`, same field copies from the selected
  table row, same two particle archive loads guarded by `gParticleFlagLoaded`,
  and same `TMapObjLibWave` allocation.
- The remaining function differences are register coloring and local label /
  stack-slot residue. No missing branch, call, constant, or store was found.
- The `.data` mismatch is extra weak/base-owner residue around inherited
  hierarchy symbols and rogue-include dtors; the required target data and ctor
  content are present.
- `python configure.py --non-matching && ninja` linked a source DOL with this
  TU enabled.
- `python configure.py && ninja` restored the matching build and passed the
  DOL hash check.

Reverified: 2026-06-13 10:52am MNL — still equivalent. Re-read the current
full diff for `TMapObjFloatOnSea::initMapObj()`; it still performs the same
lean-block init, `param_table` string selection, field copies, guarded particle
loads, and wave allocation. Remaining drift is register/local-label layout and
inherited weak/data owner residue. Proof passed again with `python configure.py
--non-matching && ninja`, then plain `python configure.py && ninja` restored the
matching config and verified `build/GMSJ01/mario.dol: OK`.
