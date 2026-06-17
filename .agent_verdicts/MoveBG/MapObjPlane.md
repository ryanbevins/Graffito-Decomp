verdict: equivalent
date: 2026-06-14 9:03am MNL
unit: MoveBG/MapObjPlane

Reason: `TMapObjPlane::calcNrm(int, int)` blocker from the previous
`not_equivalent` verdict was fixed in tick 732: the fourth local face normal
now uses `unkFC` as the edge length instead of `h0N`. Re-audit found no
remaining behavior differences.

Proof:
- `python configure.py --non-matching && ninja` linked `mario.elf` and
  produced `mario.dol` with `MoveBG/MapObjPlane.cpp` sourced.
- `python configure.py && ninja` restored the normal matching config and
  passed `build/GMSJ01/mario.dol: OK`.

Reviewed residue:
- `makeMountain()` reads the BMP width/height fields, fills `mHeightMap` from
  `unk118 + 0x436`, and calls `calcNrm(x, z)` over the full grid. Remaining
  drift is frame/register and constant-load order.
- `depress(float, float, float)` performs the same world-to-grid conversion,
  four bilinear height writes, the same twelve `calcNrm` calls (including the
  duplicate `(x + 1, z + 2)` refresh), and the same nine `updateCheckData`
  calls. Remaining drift is indexed-addressing form and register coloring.
- `calcNrm(int, int)` has the same bounds guard, wrapped neighbor samples,
  four cross-product normals, zero-or-inv-sqrt normalization behavior, summed
  normal store, and final scale by `0.25f`. Remaining drift is helper-boundary
  debt (`dot`/`scale` calls in target vs inlined normalize/add in source),
  stack size, and FPR/register allocation.
- `updateCheckData(int, int)` builds the same four vertices at
  `height + 2.0f` and writes the same two collision triangles. Remaining drift
  is stack-slot and FPR coloring only.
- `draw()` emits the same triangle-strip positions, normals, and texture
  coordinates to FIFO. Remaining drift is frame size, FPR coloring, and
  `getTexPos`/FIFO macro source shape.
