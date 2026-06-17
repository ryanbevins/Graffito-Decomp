---
unit: mario/MarioUtil/ScreenUtil
source: src/MarioUtil/ScreenUtil.cpp
verdict: equivalent
audited_at: 2026-06-12 4:20pm MNL
---

Verdict: equivalent
Time: 2026-06-13 6:30am MNL

## Summary

`MarioUtil/ScreenUtil.cpp` is functionally equivalent and source-link proven.

All functions are byte-matching except `TAfterEffect::perform`, which is
99.8%. The nonmatching residue is codegen-class only:

- `TAfterEffect::perform` has the same enable/event gates, same mode switch
  semantics, same inlined blur reset body, same call to
  `calcDashBlurValue()`, same interpolation updates, same GX state setup, same
  eight-vertex `GX_TRIANGLEFAN` output, and the same final `unk14 &= ~4` flag
  clear.
- Remaining diffs are register coloring (`r30`/`r31` for the viewport pointer
  and bool materialization) plus local-symbol/constant label attribution.

The object still emits extra standalone `TAfterEffect::setBlurDefaultValue()`,
`JDrama::TViewObj::~TViewObj()`, and the JDrama vtable in our source build.
They are unused by the target behavior, and the source-link proof shows they do
not create duplicate/undefined linker ownership problems.

## Proof

- `python tools/decomp-diff.py -u mario/MarioUtil/ScreenUtil`: no missing
  rows; only the `perform` codegen residue and known extras.
- `python configure.py --non-matching && ninja`: linked from source.
- `python configure.py && ninja`: passed and verified `mario.dol: OK`.
- 2026-06-13 6:30am MNL recheck: overview still has no missing target rows,
  and `python configure.py --non-matching && ninja` linked from source.
- 2026-06-13 9:57am MNL recheck: overview still has no missing target rows.
  Full `--no-collapse` diff for `TAfterEffect::perform` still shows identical
  enable/perform-bit gates, blur reset/default body, interpolation updates, GX
  state setup, texture load, TEV/blend/z/alpha state, eight-vertex FIFO output,
  and final flag clear. The `loadAfter` versus `setBlurDefaultValue` call label
  is source-owner drift around the same reset body; remaining residue is
  register coloring and local label ownership. Proof batch passed:
  `python configure.py --non-matching && ninja`, then `python configure.py &&
  ninja` with `mario.dol: OK`.
