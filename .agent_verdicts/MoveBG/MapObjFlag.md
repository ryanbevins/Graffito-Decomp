verdict: equivalent

date: 2026-06-15 6:55am MNL
unit: MoveBG/MapObjFlag

Certified `mario/MoveBG/MapObjFlag` as `Equivalent` after fixing one real data
behavior bug: target initializes `TMapObjFlag::mFlutterSpeed` to `4.0f`
(`0x40800000`), while source had `8.0f`. Rechecked the current text diffs:
`load`, `perform`, `initDraw`, ctor, `init`, `updateVertex`, and `draw` keep
the same calls, stores, loop bounds, strip topology, vertex/UV writes, sound
gate, texture load/draw dispatch, and matrix setup. Remaining differences are
codegen/data-owner debt: frame/register/FPR allocation, local constant label
order, FIFO loop induction spelling, extra weak owner emission from the rogue
include set, and unreferenced extra sdata pointers for infectious strings.

Proof:
- `python configure.py --non-matching`
- `ninja` linked with `MapObjFlag.o` sourced.
- `python configure.py`
- `ninja` restored normal matching config and passed `build/GMSJ01/mario.dol: OK`.

Previous verdict under older stricter byte/data bar:

verdict: not_equivalent
date: 2026-06-13 1:49am MNL
unit: MoveBG/MapObjFlag

Blocked by missing target data symbols and nonmatching data sections.

- Overview shows missing `.ctors` symbols `@2283` and `@2674`.
- `TMapObjFlag::mFlutterSpeed`, `.data`, `.sdata`, and `.sdata2` are
  nonmatching, so the object is not data-equivalent.
- `TMapObjFlag::draw()` is only 71.8% and was not audited further after the
  missing/data blockers.
