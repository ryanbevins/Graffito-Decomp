Verdict: equivalent
Time: 2026-06-13 6:31am MNL
Unit: Map/Map

Audited all nonmatching text symbols and promoted the TU to
`Object(Equivalent, ...)` in `configure.py`.

- No missing target symbols in overview.
- `TMap::update`, `isTouchedOneWall`, and `isTouchedOneWallAndMoveXZ` match behavior;
  residue is branch-shape/register/stack noise around bool normalization and local
  wall-check records.
- `initStage`, `initStageCommon`, `initPinnaParco`, `initMare`, and `initMonte` match
  the source-visible stage setup behavior. The low fuzzy rows for the common/Mare/Monte
  helpers are dominated by JGadget list iterator construction, object pointer temporary
  stack layout, and rodata label drift; the target calls and object/particle setup are
  represented in the source.
- Aggregate `.rodata`/`.data` rows are noisy, but the named strings, vtable, and static
  objects are accounted for in per-object rows or harmless source-side helper/vtable
  extras.
- Proof: `python configure.py --non-matching && ninja` linked
  `build/GMSJ01/mario.dol` from source, then plain `python configure.py && ninja`
  passed and verified `build/GMSJ01/mario.dol: OK`.
- 2026-06-13 6:31am MNL recheck: overview still has no missing target rows,
  and `python configure.py --non-matching && ninja` linked from source.
- 2026-06-13 12:51pm MNL recheck: refreshed the full nonmatching text set
  (`isTouchedOneWallAndMoveXZ`, `isTouchedOneWall`, `update`, `initStage`,
  `initStageCommon`, `initPinnaParco`, `initMare`, and `initMonte`). The
  current diffs still preserve the same wall-record calls, map-update side
  effects, stage object allocations, list insertions, particle loads, and branch
  conditions. Remaining drift is stack/frame size, local iterator temporary
  shape, saved-register coloring, branch layout with equivalent predicates, and
  helper/data owner labels. Proof passed again with `python configure.py
  --non-matching && ninja`, then plain `python configure.py && ninja` verified
  `build/GMSJ01/mario.dol: OK`.
