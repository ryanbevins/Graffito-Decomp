# Map/MapMakeList Audit

## 2026-06-14 4:39am MNL - equivalent

Verdict: `equivalent`.

`mario/Map/MapMakeList` is behavior-equivalent and source-links under
`--non-matching`.

Fix made during this audit:
- `TMapCollisionData::getGridArea` now uses `mGridExtentY` for the two Z grid
  coordinate conversions:
  `*param_4 = (minZ + mGridExtentY) * 0.0009765625f;`
  `*param_6 = (maxZ + mGridExtentY) * 0.0009765625f;`
  The previous source used `mGridExtentX` for those Z conversions and differed
  on non-square grids.

Review summary:
- `getGridArea` now has the target extent behavior. Remaining diff is
  codegen-class min/max evaluation order and stack-slot layout.
- `addCheckDataToGrid` has the same loops, `getGridArea` gate, plane-type
  dispatch, `polygonIsInGrid` calls, list root selection, `allocCheckList`, and
  `addAfterPreNode` behavior. Remaining drift is stack/FPR/GPR layout, local
  helper call-boundary ownership (`getListRoot` / `addCheckDataToList` source
  helpers vs target inlining), and equivalent float temporary ordering.
- `removeCheckListData` and `updateCheckListNode` are codegen-class only:
  frame/register layout and symbol-label drift, with matching list unlink,
  `memmove`, pointer retargeting, and entry-ID compaction behavior.
- No target symbols are missing. Extra helper functions are source-owned byte
  debt and do not block source linking.

Proof:
```bash
python configure.py --non-matching && ninja
python configure.py && ninja
```

The `--non-matching` build linked `Map/MapMakeList.cpp` from source, and the
normal matching build ended with `build/GMSJ01/mario.dol: OK`.
