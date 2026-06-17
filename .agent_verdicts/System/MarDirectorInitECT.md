verdict: equivalent
date: 2026-06-13 11:26pm MNL
unit: System/MarDirectorInitECT

Certified behavior-equivalent after the implementation pass fixed the
`TMarDirector::initECDisp()` full-screen orthographic projection extents.

Reviewed current overview:
- No missing target text functions.
- Residual target-missing items are four 4-byte local rodata constants; rebuilt
  code uses equivalent local constant/data owners.
- Extra text symbols are helper-owner debt for JGadget iterator/list helpers,
  JDrama ctor/dtor helpers, `TFlagT<unsigned short>::set`, and `TNameRef`
  construction. The rebuilt object does not leave undefined references.

Reviewed nonmatching text:
- `setupPerformList_console()` preserves the same `Group 2D` lookup,
  `TEmitterViewObj` allocation/insert, and three `unk30->push_back` calls;
  residue is stack slots, register coloring, and local JGadget helper ownership.
- `initECDisp(...)` preserves the same stage display setup, searches, optional
  lens glow/flare creation, optional lens render pass pushes, 3D/2D group pushes,
  and `stageDisp` reuse. The formerly wrong `ortho2`/`ortho3` width/height
  behavior is fixed: target and rebuild both store render width to
  `TOrthoProj::mField[2]`/offset `0x38` and render height to
  `mField[3]`/offset `0x3c`; call/evaluation order and FPR coloring still drift.
- `initECTMir(...)` preserves the same mirror EFB lookup, flag/vfilter stores,
  mirror camera texture attribute copy, texture width/height query, source rect
  setup, and return; residue is stack frame/slot layout.
- `initECTGft(...)` preserves the same bath-water fallback, graffiti group/draw
  init lookup, EFB texture/viewport/ortho setup, loop over pollution joint
  models, TIMG image pointer/size/format stores, source rect setup, and perform
  list pushes. The apparent width/height register swap in the loop still stores
  image width to `0x38` and image height to `0x3c`.

Proof:
- `python configure.py --non-matching && ninja` linked successfully with
  `System/MarDirectorInitECT.o` from source.
- `python configure.py && ninja` restored the normal matching config and passed
  with `build/GMSJ01/mario.dol: OK`.
