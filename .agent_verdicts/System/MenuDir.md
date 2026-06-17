# System/MenuDir audit

Verdict: equivalent
Date: 2026-06-14 11:40am MNL

Promoted `mario/System/MenuDir` to `Object(Equivalent, ...)` after fixing two
real behavior mismatches found during the audit:

- `TMenuDirector::direct()` now loads the default wave only when flag `0x30007`
  was not already set, matching the target `bne skip` branch after
  `TFlagManager::getBool()`.
- The wave passed to `loadWave()` and `checkWaveOnAram()` is now
  `MS_WAVE_DEFAULT` (`0x100`), matching the target immediate.
- `TMenuDirector::rsetup()` now returns `1` when `new J2DSetScreen(...)`
  leaves `unk3C == nullptr`, matching the target failure return.

Build proof:
- `python configure.py --non-matching && ninja` linked with `MenuDir.o`
  sourced.
- `python configure.py && ninja` restored the matching config and passed
  `build/GMSJ01/mario.dol: OK`.

Behavior review:
- `TMenuDirector::setFixedStageValue()` is exact except stack-frame/stack-slot
  placement for the local conversion table.
- `TMenuDirector::direct()` now has matching first-start flag/wave behavior,
  matching menu-state transitions, matching menu text generation for all stage
  groups/movie groups/default groups, matching fade/show/hide calls, matching
  next-area writes, and matching return-state selection. Remaining diffs are
  stack shape, string-label offsets, range-check spelling, and `TGameSequence`
  temporary/copy shape.
- `TMenuDirector::rsetup()` now has matching failure returns, resource loads,
  view-list insertion sequence, pane lookups, text buffer/message setup,
  stage/display/screen setup, `TOrthoProj` values, and final `unk40->show()`.
  Remaining diffs are frame/iterator temporary layout, constructor inlining
  boundaries, source-owned weak helper labels, and rodata label offsets.
- `setup()`, `setupThreadFunc()`, destructor, constructor, static init, vtable,
  and adjustor are behavior-identical or byte-identical. The target-local
  `TVec3<float>::set`, missing ctor labels, and extra weak/base owners are
  source-emission/byte debt only; the source-link proof shows they are not
  unresolved behavioral dependencies.
