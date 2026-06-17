# Audit verdict: equivalent

Date: 2026-06-14 5:47am MNL
Mode: AUDIT
Unit: `mario/Map/MapEventMare`

Verdict: equivalent

Proof:
- `python configure.py --non-matching && ninja` linked successfully with
  `Map/MapEventMare.cpp` source-linked.
- Follow-up normal `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.

Reason:
- The implementation follow-up fixed the previous real blocker in
  `TMareEventDepressWall::rising()`: sound gate/start now precede particle
  `0x15b` emission/configuration.
- Rechecked the remaining nonmatching rows after the implementation fixes.
  `TMareEventBumpyWall::load()` differs by compare-tree order but maps the same
  building-index ranges to the same bump directions.
- Bumpy-wall `control()` and bump functions perform the same state dispatch,
  joint translation, pre/post collision movement, rumble/camera/sound side
  effects, warp setup, and final `kill()` behavior; residue is stack/temp
  layout and branch shape.
- `TMareEventDepressWall::initCommon()`, `depressing()`, `rising()`,
  `perform()`, `TMareEventWallRock::load()`, and `TMareWallRock`
  setup/movement/appear functions are behavior-aligned. Remaining drift is
  codegen-class: iterator temporaries, `calcMap()` call-boundary/inlining,
  vector temp layout, sqrt expression shape, frame size, register coloring, and
  local data/constant label ownership.
- Missing/extra local constants and weak helper/data symbols do not create
  undefined references in the source-linked build.
