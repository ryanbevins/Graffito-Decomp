# Camera/lensglow audit

Verdict: `equivalent`

Checked 2026-06-14 11:05am MNL in AUDIT mode.

Certified after fixing the previous source-link blocker. `lensglow.cpp` now
declares `cSunVolumeName` and `cSunsetVolumeName` as `extern`, matching
`sunmodel.cpp` and `lensflare.cpp`; `System/MarNameRefGen_Map.cpp` remains the
source owner for those globals.

Behavior reviewed as matching:

- Constructor: same base/J3DFrameCtrl initialization, sun/sunset volume pointer
  select, resource path construction for `glow.bmd`, `glow.btk`, and
  `glow.brk`, model/animation load, material-anm setup loop, animator entry,
  frame-controller setup, and initial scale/offset fields.
- `perform`: same indoor/sun-position visibility test, alpha target/chase,
  scale target/chase, visible-centroid calculation, matrix setup/copy/calc,
  material Tev color alpha update, animation frame update, model entry, and
  view calc gating.

Remaining diffs are byte/codegen/data-label debt:

- Frame size/register/FPR allocation and loop induction shape differ.
- Objdiff now reports local rodata labels `@1490`, `@1632`, and `@1633` as
  missing because source uses the shared extern sun-volume globals; the runtime
  strings and pointers are supplied by `MarNameRefGen_Map`.
- Extra `J3DFrameCtrl`/`TViewObj` weak/vtable owners and infectious strings are
  unreferenced/source-link safe.

Proof:

- `python configure.py --non-matching && ninja` linked successfully with
  `lensglow.o` sourced.
- `python configure.py && ninja` restored the normal matching config and passed
  `build/GMSJ01/mario.dol: OK`.
