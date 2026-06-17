# JSystem/JDrama/JDRCamera

Verdict: equivalent
Date: 2026-06-13 9:55am MNL

Reason:
- All accessors, thunks, destructors, and camera vtables are byte-matching.
- `TLookAtCamera::perform`, `TOrthoProj::load`, and `TOrthoProj::perform`
  differ only by stack frame size/slot offsets with identical calls, flags,
  constants, field offsets, and store order.
- `TPolarCamera::perform` keeps the same runtime behavior: `flags & 0x14`
  gate, perspective setup, near/far mirror stores, the three trig rotation
  stages from `-unk40`, `-unk3C`, and `unk38`, final view-matrix copy, and
  `flags & 0x10` `GXSetProjection` call. The large residue is known
  matrix-construction spelling/helper ownership: target stores matrix
  intermediates directly through the local `SMatrix34C<float>::set` helper,
  while current source uses clearer `TRotation3f`/`TPosition3f` temporaries
  and emits an extra weak `identity33`.
- Data residue is constant-order/weak-owner drift (`0`, `1`, degree-radian
  constant; extra weak `TViewObj` vtable/destructor), not missing runtime data.

2026-06-13 9:55am MNL recheck:
- Current overview still has no missing target symbols.
- Full `--no-collapse` diffs were re-read for `TPolarCamera::perform`,
  `SMatrix34C<float>::set`, `TLookAtCamera::perform`, `TOrthoProj::load`, and
  `TOrthoProj::perform`. Raw target asm confirms the noisy calls in
  `TPolarCamera::perform` are to the local
  `SMatrix34C<float>::set(float,...)` helper, not to camera accessors.
- Remaining residue is frame size/slot layout, FPR/register coloring, and
  matrix-helper owner/source-shape drift only; calls, stores, constants, branch
  predicates, final `PSMTXCopy`, and `GXSetProjection` behavior still match.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` verified `build/GMSJ01/mario.dol: OK`.

2026-06-14 10:45pm MNL safety-net recheck:
- Current overview still has no missing target symbols; the only text residue
  remains `TPolarCamera::perform`, local `SMatrix34C<float>::set`,
  `TLookAtCamera::perform`, `TOrthoProj::load`, and `TOrthoProj::perform`.
- Re-read the full current diffs for those camera functions. `TLookAtCamera`
  and `TOrthoProj` are frame-only. `TPolarCamera` still has the same
  `param_1 & 0x14` gate, perspective call, near/far stores, three trig matrix
  stages, final `PSMTXCopy`, and conditional `GXSetProjection`; remaining drift
  is matrix helper/source-shape, frame, FPR/register, and local data-label debt.
- The Kazekun audit tick's `python configure.py --non-matching && ninja`
  source-link proof also rebuilt and linked this existing `Equivalent` row.
