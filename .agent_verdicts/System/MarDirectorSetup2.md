Verdict: equivalent
Time: 2026-06-13 6:30am MNL
Unit: mario/System/MarDirectorSetup2
Source: src/System/MarDirectorSetup2.cpp

Reason:
- `TMarDirector::setup2()` and `TMarDirector::~TMarDirector()` were reviewed
  with `--no-collapse`; remaining text differences are codegen-class only:
  stack/frame size, stack slot placement, saved-register coloring, local-label
  offsets, and equivalent argument-register setup for
  `MSound::setCameraInfo`.
- The previous structural setup gap is fixed: the stage-event loop writes the
  found `TMapObjBase` event index to `unk134` before storing the object pointer
  into `TStageEventInfo::unk28`, and the pause/talk windows receive the gamepad
  pointer stores.
- Source-link validation initially exposed duplicate `TYoshi::onYoshi()`
  ownership from the full `MarioMain.hpp` include. The TU now uses a minimal
  local `TMario` declaration for `setGamePad`/`gpMarioOriginal`, removing the
  duplicate while preserving the required base-layout pointer adjustment.
- Anonymous vector rodata coverage is intact through the existing TU-local
  dummy owner; `decomp-diff -s missing` reports no missing symbols.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` passed and verified `mario.dol: OK`.
- 2026-06-13 6:30am MNL recheck: overview still has no missing target rows,
  and `python configure.py --non-matching && ninja` linked from source.
- 2026-06-13 10:00am MNL recheck: full `--no-collapse` diffs for
  `~TMarDirector` and `setup2` remain behavior-equivalent. The destructor keeps
  the same sound exit, archive unmount checks, THP shutdown gate, draw-sync
  callback clears, globals clears, vector dtor, base dtors, and optional delete.
  `setup2` preserves event-object wiring, gamepad/UI lookups and stores, fader
  setup, draw-sync callback registration, `MSound::setCameraInfo`, stage init,
  draw-done wait, and buried-building init. Residue is frame/register coloring,
  local string labels, argument-register order, and no-op helper owner drift.
  Proof rerun passed: `python configure.py --non-matching && ninja`, then
  normal `python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
