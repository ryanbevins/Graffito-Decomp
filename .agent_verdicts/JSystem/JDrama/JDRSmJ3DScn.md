Verdict: equivalent
Date: 2026-06-13 2:44pm MNL
Unit: mario/JSystem/JDrama/JDRSmJ3DScn
Source: src/JSystem/JDrama/JDRSmJ3DScn.cpp

Revalidated during the AUDIT secondary safety sweep. Current overview still has
no missing target rows. `objdump -drC` on target and rebuilt objects confirms
decomp-diff's misleading inherited-helper labels are ownership/name drift: both
objects call `TViewObjPtrListT<TViewObj, TViewObj>::perform` and
`TViewObjPtrListT<TViewObj, TViewObj>::loadSuper`.

Current full diff classification:
- `TSmJ3DScn::perform`: same optional child-list perform for flags `0x1/0x2`,
  optional light-map perform, view matrix copy into `j3dSys`, `drawInit`, draw
  buffer `frameInit` loop, draw-buffer installation, child-list perform with
  `param_1 | 0x204`, draw-mode stores, and both draw calls. Residue is frame
  size and helper-owner label drift only.
- `TSmJ3DScn::loadSuper`: same parent `loadSuper`, `TLightMap` allocation,
  store to `mLightMap`, and virtual `load` call. Target spells base
  construction as ctor calls; source inlines the equivalent `TNameRef`,
  `TViewObj`, `TFlagT`, and `TLightMap` field setup.

Proof reused from this tick: `python configure.py --non-matching && ninja`
linked with this row from source, then `python configure.py && ninja` restored
normal config and verified `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

Verdict: equivalent
Date: 2026-06-13 9:10am MNL
Unit: mario/JSystem/JDrama/JDRSmJ3DScn
Source: src/JSystem/JDrama/JDRSmJ3DScn.cpp

Reason:
- Reviewed both nonmatching functions. `perform` preserves the child-list
  perform call for flags `0x1/0x2`, light-map perform, view-matrix copy,
  `J3DSys::drawInit`, draw-buffer frame-init loop, draw-buffer installation,
  child-list perform with `0x204`, draw-mode updates, and both draw calls.
- `loadSuper` preserves the parent `loadSuper`, `TLightMap` allocation and
  construction, store into `mLightMap`, and virtual `load` call. The remaining
  constructor-shape differences inline the same `TNameRef`/`TViewObj`/flag
  setup and default fields.
- No target symbols are missing. Extra template/base emissions are not
  target-owned and source-link validation accepts the TU.

Verification:
- `python configure.py --non-matching && ninja`
- `python configure.py && ninja`

2026-06-13 9:10am MNL recheck: full current diffs still show only codegen and
helper-owner residue; overview has no missing target rows, source-link proof
passed, and the normal matching build verified `build/GMSJ01/mario.dol: OK`.

Offending functions: none.
