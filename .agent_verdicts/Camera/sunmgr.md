# Camera/sunmgr Audit

Verdict: equivalent  
Date: 2026-06-13 8:27am MNL

Reverified `Camera/sunmgr.cpp` as `Object(Equivalent, ...)`.

Reviewed functions:
- `TSunMgr::getAddColor() const`: same `unk14` guard, `gpSunModel->unkAC`
  float-to-int conversion, and zero fallback. Residue is stack frame/slot
  size only.
- `TSunMgr::perform(unsigned long, JDrama::TGraphics*)`: same warp-enable,
  perform-bit, graphics-mode, L-button camera, Mario distance, sun-model UV
  window, stage-change, BGM sound-volume, and sound-pitch behavior. Residue is
  stack/register allocation, equivalent zero materialization (`mr` vs `li`),
  and local constant-label ownership.
- `TSunMgr::load(JSUMemoryInputStream&)`: same stream reads, color packing,
  sun/sunset model searches, map/flag guard, `unk15` updates, position-holder
  lookup for `cSunWarpPointName`, and warp-position copy. The field-store and
  stack-load grouping around `unk18/unk1C` is reordered before any external
  call but computes the same fields. Remaining residue is stack/register
  coloring and string/helper label ownership.

Data notes:
- Missing anonymous ctor-string rows correspond to source-named strings
  (`dummyMactorStringValue1`, `SMS_NO_MEMORY_MESSAGE`, `cSunSceneName`,
  `cSunsetSceneName`) and source-owned weak/vtable extras. No target text
  symbols are missing.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` passed afterward and verified
  `build/GMSJ01/mario.dol: OK`.

2026-06-13 11:44am MNL recheck: verdict remains `equivalent`. Current overview
is unchanged: no missing target text rows, same three nonmatching functions,
and the same anonymous ctor-string rows paired with source-named string/vtable
extras. The earlier function review still applies for add-color, perform, and
load behavior. Shared proof from this tick passed:
`python configure.py --non-matching && ninja`, then normal `python configure.py
&& ninja` verified `build/GMSJ01/mario.dol: OK`.
