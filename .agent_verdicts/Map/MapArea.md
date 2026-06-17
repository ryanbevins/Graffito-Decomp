# Map/MapArea Audit

Verdict: equivalent
Date: 2026-06-13 2:43pm MNL

Revalidated during the AUDIT secondary safety sweep. Current overview still has
only the two known text diffs and no missing/extra symbols.

Current full diff classification:
- `checkLinesCollision`: same two segment cross-product sign tests and same
  false/true returns; residue is frame size and constant-label numbering.
- `TMapCollisionData::polygonIsInGrid`: same negative-normal fast accept, three
  polygon-vertex grid checks, four grid-corner point-in-polygon checks, and four
  grid-edge-vs-polygon-edge collision checks. The large frame delta is stack
  layout/codegen debt; the branch predicates and geometric computations are
  unchanged.

Proof reused from this tick: `python configure.py --non-matching && ninja`
linked with this row from source, then `python configure.py && ninja` restored
normal config and verified `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

Verdict: equivalent  
Date: 2026-06-13 9:08am MNL

Kept as `Object(Equivalent, "Map/MapArea.cpp")`.

Proof:

- `python tools/decomp-diff.py -u mario/Map/MapArea` shows no missing or
  extra target rows.
- `python configure.py --non-matching && ninja` linked from source in the
  current sweep.
- `python configure.py && ninja` restored the matching config and verified
  `build/GMSJ01/mario.dol: OK`.

Behavior review:

- `TMapCollisionData::polygonIsInGrid()` preserves the same negative-normal
  accept, point-in-grid checks for all three polygon vertices,
  point-in-polygon checks for all four grid corners, and line/polygon edge
  collision checks for all four grid edges.
- `checkLinesCollision()` performs the same segment cross-product sign tests
  and boolean returns.
- Remaining differences are frame size/stack slots, temporary pointer/register
  choices for repeated polygon vertices, branch-label layout, and constant
  label numbering.

Offending functions: none.
