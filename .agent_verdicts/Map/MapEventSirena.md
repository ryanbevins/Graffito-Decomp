## Map/MapEventSirena

Verdict: equivalent
Audited: 2026-06-12 11:04am MNL
Reverified: 2026-06-13 6:48am MNL — current source still links under
`python configure.py --non-matching && ninja`; restored matching build with
`python configure.py && ninja` and DOL hash check passed.

Promoted `Object(NonMatching, "Map/MapEventSirena.cpp")` to `Equivalent`.

Evidence:
- `load`, `loadAfter`, and `watch` are the only nonmatching target functions;
  destructor, constructor, pollution hooks, static init, rodata, and sdata2 are
  byte-exact.
- `load` preserves the same `TMapEventSink::load` call, unused string read,
  warp-position / warp-y stream reads, and two guarded particle loads
  (`0x68`, `0x1e4`). Differences are frame size and local label/address
  presentation.
- `loadAfter` preserves the same root `search("ホテル上げカメラ")`, actor
  pointer store, timing fields, camera distance, and shine-position constants.
- `watch` preserves the same `unk64` gate and event side effects: pollution
  layer flag, `unk28 = 0`, demo-camera fire, shine demo creation, flag set,
  Mario warp, and two particle emits at the map-object manager position.
  Remaining differences are stack/register scheduling around the temporary
  `JDrama::TFlagT<u16>` argument and local-label residue.
- The `.data` mismatch is extra weak/base-owner residue (`TMapEventSink`,
  `JDrama::TViewObj`, JSUList dtors, infectious strings); required target data
  is present and source-link validation succeeds.
- `python configure.py --non-matching && ninja` linked a source DOL with this
  TU enabled.
- `python configure.py && ninja` restored the matching build and passed the
  DOL hash check.

2026-06-13 10:47am MNL recheck: verdict remains `equivalent`. Re-read the
current diffs for `load`, `loadAfter`, and `watch`. `load` still performs the
same base load, unused string read, warp-position/Y reads, and guarded particle
loads. `loadAfter` still searches/stores the hotel camera actor, timing fields,
camera distance, and shine-position constants. `watch` still has the same
`unk64` gate, pollution-layer flag set, `unk28 = 0`, demo-camera fire,
shine-demo creation, flag set, Mario warp, and two particle emits at the
map-object manager position. The remaining low-score drift is stack/register
scheduling around the temporary `JDrama::TFlagT<u16>` and local labels. Proof
refreshed with `python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
