# MoveBG/MapObjWater

Verdict: equivalent
Date: 2026-06-12 2:46pm MNL

Reason: all target text functions are present. The remaining text diffs are
behavior-equivalent:

- `TMapObjWaterFilter::perform`: same `unk44` and director-mode early exits,
  simple-demo/camera-mode guard, water-height guard, transform/scale/view matrix
  construction, model base matrix copy, and actor perform call. Residue is stack
  frame/temporary placement, equivalent branch layout for the early exits, and
  label-display noise.
- `TMapObjWaterFilter::TMapObjWaterFilter` and
  `TMapObjSeaIndirect::TMapObjSeaIndirect`: same base construction, vtable
  stores, placement defaults, actor defaults, derived vtable stores, and
  `unk44 = nullptr`; residue is helper-owner label attribution for the base
  constructors.

Data note: `.data` residue is from source-owned weak/base helper labels and
infectious-string helpers; runtime vtables and constants are present.

Proof: `python configure.py --non-matching && ninja` linked from source, then
plain `python configure.py && ninja` passed and verified `mario.dol: OK`.

2026-06-13 8:13am MNL recheck:
- Overview still has no missing target functions. Nonmatching text remains
  limited to `TMapObjWaterFilter::perform`, `TMapObjWaterFilter` ctor, and
  `TMapObjSeaIndirect` ctor.
- `TMapObjWaterFilter::perform`: current full diff preserves the same
  `unk44` and director-mode early exits, simple-demo / camera-mode guard,
  water-height guard, transform-info setup, translate/scale/view-matrix
  operations, model base-matrix copy, and final `MActor::perform` call. The
  branch layout around the director and water-height guards differs, but the
  branch conditions and destinations preserve the same return/continue
  behavior. Remaining drift is stack frame/temporary placement and label noise.
- `TMapObjWaterFilter` / `TMapObjSeaIndirect` constructors: current full diffs
  preserve base actor construction, vtable stores, placement defaults, actor
  defaults, derived vtable stores, and `unk44 = nullptr`. The displayed call
  names are helper-owner attribution noise; the constructor stores line up.
- Proof refreshed in the same audit sweep: `python configure.py --non-matching
  && ninja` linked from source, and normal `python configure.py && ninja`
  verified `build/GMSJ01/mario.dol: OK`.

2026-06-13 11:43am MNL recheck: verdict remains `equivalent`. Current overview
is unchanged: no missing target rows, same three nonmatching text functions, and
the same source-owned base/destructor/infectious-string extras. The prior
full-diff classification remains valid: water-filter perform and both
constructors preserve branch conditions, calls, vtable stores, placement
defaults, and field stores. Shared proof from this tick passed:
`python configure.py --non-matching && ninja`, then normal `python configure.py
&& ninja` verified `build/GMSJ01/mario.dol: OK`.
