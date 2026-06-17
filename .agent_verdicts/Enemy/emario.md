# Enemy/emario

Verdict: equivalent
Date: 2026-06-14 12:40pm MNL

Reason:
- `TEMario::init(TLiveManager*)` behavior bug fixed in `8ebcb723`: target
  toggles `THitActor::unk64` / `HIT_FLAG_NO_COLLISION` around `initHitActor`,
  not `mActorType`.
- `TEMario::perform(unsigned long, JDrama::TGraphics*)` behavior bug fixed in
  `be5425f8`: target collision switch body order is `0x80000001` notify actor
  first, then `0x400000bc` damage Enemy Mario, with `flags & 1` retained as the
  later draw-copy guard.
- This audit fixed one more behavior bug: `TEMario::load(JSUMemoryInputStream&)`
  calls the `TEnemyMario` vtable slot at `0xc0`, which is
  `TEnemyMario::initValues()`, after the basket lookup. Source previously
  called the early `loadAfter()` slot at `0x18`.
- Remaining diffs are behavior-neutral: stack/frame/register allocation,
  string/data-label ownership, source-inlined `TUtil<f32>::sqrt`/`TVec3::sub`
  versus target weak call boundaries, and local `cDirtyFileName` /
  `cDirtyTexName` array-vs-pointer data shape. The extra/missing local data
  owners are not referenced as unresolved symbols and did not block source link.

Proof:
- `python configure.py --non-matching && ninja` linked with `Enemy/emario.cpp`
  sourced.
- `python configure.py && ninja` restored matching config and passed
  `build/GMSJ01/mario.dol: OK`.
