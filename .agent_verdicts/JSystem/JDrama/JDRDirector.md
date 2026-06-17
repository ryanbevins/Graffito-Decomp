Verdict: equivalent

Date: 2026-06-13 6:32pm MNL

Reason: `JSystem/JDrama/JDRDirector.cpp` links from source and the remaining
diffs are byte/codegen debt, not behavior differences.

Behavior review:
- `JDrama::TDirector::direct()` now matches the target-visible flag behavior:
  it constructs `JDrama::TGraphics`, writes `graphics.unk0 = 1`, calls
  `unk10->testPerform(3, &graphics)`, writes `graphics.unk0 = 0`, then calls
  `unk14->testPerform(8, &graphics)`. Remaining drift is the current
  constructor-depth extra zero store to `TGraphics::unkFE`; no current source
  reads that field.
- `JDrama::TDirector::JSGFindObject(const char*, JStage::TEObject) const` has
  the same `search(name)`, `getType()`, and camera/actor cast return behavior.
  The diff is register coloring of `this` vs `name`.
- Extra local helper owners
  `TViewObjPtrListT<JDrama::TViewObj, JDrama::TViewObj>::searchF(...)` and the
  8-byte `JGadget::TList<void*>::iterator` constructor are unreferenced
  helper-owner byte debt; there are no missing symbols.

Proof:
- `python configure.py --non-matching && ninja` linked cleanly with
  `JDRDirector` from source.
- `python configure.py && ninja` restored the matching build and passed
  `build/GMSJ01/mario.dol: OK`.

Prior blocker resolved: the old red verdict came from `direct()` writing
`graphics.unk2`; implementation tick 670 changed it to `graphics.unk0`, matching
the target stores.

2026-06-14 5:58pm MNL safety-net recheck: verdict remains `equivalent`.
Current overview has no missing target symbols. Fresh full diffs for `direct`
and `JSGFindObject` show the same perform flag sequence, search/key lookup,
type switch, and camera/actor cast returns. The only visible `direct` drift is
the extra source-side zero store to `TGraphics::unkFE`; current source has no
direct `JDrama::TGraphics::unkFE` reads, so this remains initialization byte
debt rather than a behavior difference. This tick's source-link and normal
proof builds covered the current object.
