Verdict: equivalent
Time: 2026-06-13 6:31am MNL

Reason: reviewed all nonmatching functions and verified source link with
`python configure.py --non-matching && ninja`; normal `python configure.py &&
ninja` then passed with `mario.dol: OK`.

Reviewed functions:
- `TMapObjGrassManager::perform`: behavior matches swing-offset update,
  angle wrap, per-group near/far classification, map-2 forced-near behavior,
  and draw pass ordering (`initDrawNear`, all near groups, far vertex format,
  all far groups). Residue is stack size, register/FPR allocation, and source
  expression shape for distance math.
- `TMapObjGrassManager::initDrawNear`: same inverse view-matrix column scaling,
  `mDrawVec`/`mDrawVecS16` writes, color-array setup, GX channel/TEV/blend/
  alpha/Z/cull state, with only frame/local-label drift.
- `TMapObjGrassGroup::drawNear`: same `unk78 == 0` gate and three vertices per
  blade: bottom-left color 1, swinging top color 0, bottom-right color 1.
- `TMapObjGrassGroup::drawFar`: same `unk78 == 1` gate and s16 vertex stream,
  including `unk74[i]` as the top vertex Y.

No missing symbols. Extra weak/helper rows are source-link-safe owner drift
from included base classes, vector helpers, and rogue MSound static lists.

2026-06-13 6:31am MNL recheck: overview still has no missing target rows, and
`python configure.py --non-matching && ninja` linked from source.

Reverified: 2026-06-13 10:57am MNL — still equivalent. Re-read all four
nonmatching functions. Manager `perform` still preserves swing/color update,
angle wrap, near/far classification, map-2 force-near behavior, and draw-pass
ordering. `initDrawNear` still preserves inverse-view scaling, draw-vector
stores, and GX state setup. `drawFar`/`drawNear` still emit the same three
vertices per blade and color/height choices. Remaining drift is stack/register
layout, distance expression shape, vertex-stream scheduling, and local
constant/table owner labels. Proof passed again with `python configure.py
--non-matching && ninja`, then plain `python configure.py && ninja` restored the
matching config and verified `build/GMSJ01/mario.dol: OK`.
