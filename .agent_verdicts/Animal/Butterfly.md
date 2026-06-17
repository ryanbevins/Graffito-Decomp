# Animal/Butterfly

verdict: equivalent
date: 2026-06-14 3:22am MNL
unit: Animal/Butterfly

Reverified during tick 709 as the secondary AUDIT safety net. Current overview
still has no missing target symbols and the same four nonmatching text
functions (`createRealoidActor`, `load`, `receiveMessage`, `init`). The
existing behavior review below still holds; remaining diffs are frame/layout,
branch/register choices, iterator temporary labels, and weak/static ownership.
The tick's `python configure.py --non-matching && ninja` source-link proof
included `Animal/Butterfly`, and the final normal build passed `mario.dol: OK`.

## 2026-06-13 10:36pm MNL

Reverified during tick 691. The current overview still has the same four
nonmatching text functions and data-owner drift. The existing behavior review
below still holds: no missing target symbols and no structural behavior gap.
The `--non-matching` proof run for this tick linked the whole source-linked set
successfully, including `Animal/Butterfly`.

## 2026-06-13 9:29am MNL - refreshed

Verdict remains `equivalent`.

Re-read full `--no-collapse` diffs for `createRealoidActor`, `load`,
`receiveMessage`, and `TButterfly::init`. Residue is frame layout, branch layout,
loop index register choice, stack placement for default vectors, and static-data
ownership. Raw objdump confirmed `TButterfly::init` calls the same JGadget
iterator constructors and `insert` helper; the pretty diff's destructor label is
symbol-label drift.

Verdict: equivalent
Date: 2026-06-12 11:16pm MNL

Promoted `Animal/Butterfly.cpp` to `Object(Equivalent, ...)` after
source-link proof.

Reviewed functions:
- `TButterfloid::createRealoidActor(MActor*)`: same `TButterfly` allocation,
  `TRealoidActor` construction, vtable install, `mFloid` back-pointer store,
  and null-allocation return behavior. Residue is frame layout.
- `TButterfloid::load(JSUMemoryInputStream&)`: same model-name load, event ID
  read, coin/event/1-up object creation switch, boid-leader parameter writes,
  Mario repel actor/position setup, repel range/force/flag writes, all-actor
  BCK setup, and actor init loop. Residue is stack size, loop index register
  choice, and default-vector construction layout.
- `TButterfly::receiveMessage(THitActor*, unsigned long)`: same handling for
  take/release/death messages, particle emit, dead-count tracking, final reward
  object appear/replacement, position/velocity writes, and live-flag clear.
  Residue is branch layout and register/frame selection.
- `TButterfly::init()`: same hit actor setup, hit-flag changes, name-ref group
  lookup, and child insertion. Residue is iterator temporary layout/labels.

Data:
- `.data` mismatch comes from source-owned weak/static data (`TRealoidActor`
  and nerve/vtable/string helpers) that the target object imports from
  elsewhere. Source-link validation accepts the ownership drift; Butterfly's
  own strings, vtables, and rodata rows match.

Validation:
- `python configure.py --non-matching && ninja` linked successfully with
  `Butterfly` from source.
- `python configure.py && ninja` passed and reported `mario.dol: OK`.
