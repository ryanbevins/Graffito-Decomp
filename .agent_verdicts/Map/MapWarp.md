Verdict: equivalent
Time: 2026-06-13 6:32am MNL
Unit: mario/Map/MapWarp
Source: src/Map/MapWarp.cpp

Reason:
- Re-reviewed the three nonmatching functions. `TMapWarp::changeModel(int)`
  differs only by stack size while preserving the current-child sleep,
  requested-child awake, and active index update.
- `TMapWarp::watchToWarp()` preserves the ground warp/map-change checks,
  model sleep/awake transitions, Mario warp request from current position plus
  per-warp offset, map-change index update, cube-stream matrix setup, and
  flow-vs-wind movement dispatch. Residue is stack/register/temp placement and
  local helper owner labeling.
- `TMapWarp::init(JSUMemoryInputStream&)` preserves the stream reads, name-table
  lookup, point-vector storage, warp-info pair construction, reverse-offset
  setup, and Mare stage distance override. Residue is stack layout, register
  coloring, constructor/helper owner labels, and source-local table provenance.
- No missing symbols; named data/rodata/sdata2 rows match.
- Proof: `python configure.py --non-matching && ninja` linked from source, then
  plain `python configure.py && ninja` passed and verified `mario.dol: OK`.
- 2026-06-13 6:32am MNL recheck: overview still has no missing target rows,
  and `python configure.py --non-matching && ninja` linked from source.

Reverified: 2026-06-13 10:54am MNL — still equivalent. Re-read `init`,
`watchToWarp`, and `changeModel`. Stream reads, name-table lookup, point-vector
and reverse-offset construction, model sleep/awake transitions, Mario warp
request, map-change index update, cube-stream matrix setup, and flow/wind
dispatch remain behaviorally identical. Remaining drift is stack/register/temp
placement and helper/owner-label presentation. Proof passed again with `python
configure.py --non-matching && ninja`, then plain `python configure.py && ninja`
restored the matching config and verified `build/GMSJ01/mario.dol: OK`.
