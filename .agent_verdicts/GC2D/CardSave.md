# GC2D/CardSave audit

Verdict: equivalent
Time: 2026-06-13 6:30am MNL

Status: equivalent
Time: 2026-06-13 5:13am MNL

Unit: `mario/GC2D/CardSave`
Source: `src/GC2D/CardSave.cpp`
Promoted: `Object(Equivalent, "GC2D/CardSave.cpp")`

## Verdict

Promoted after a full behavior review and source-link proof.

Build proof:
- `python configure.py --non-matching && ninja` linked `build/GMSJ01/mario.dol`
  with the sourced `CardSave` object.
- `python configure.py && ninja` restored the matching config and reported
  `build/GMSJ01/mario.dol: OK`.

## Structural Fix Applied

`TCardSave::execMovement_`, `PROGRESS_UNK13`: corrected the bookmark timestamp
predicate. Target behavior is:
- empty bookmark slot: transition to `PROGRESS_UNK16` and write the bookmark;
- existing slot with matching timestamp: transition to `PROGRESS_UNK16`;
- existing slot with differing timestamp: transition to `PROGRESS_UNK2C`.

The old source sent matching timestamps to `PROGRESS_UNK2C` and differing
timestamps to `PROGRESS_UNK16`.

## Remaining Nonmatches

All remaining reviewed deltas are codegen-class:
- `execMovement_`: repeated `TExPane` movement and stream-construction blocks
  differ by register allocation, stack-slot placement, and branch layout after
  the predicate fix; call sequences, field offsets, state writes, and constants
  are behavior-equivalent.
- `perform`: target inlines the orthographic draw helper; source emits an extra
  helper copy, but the inlined path performs the same viewport/scissor setup,
  screen draw, destructor sequence, and `GXSetScissor` restore.
- `waitFor*`, `drawMessage*`, `waitForChoice*`, `waitForStop`, `initData`,
  `load`: remaining differences are stack-slot drift, register coloring,
  harmless return extension (`extsb` vs already signed value), branch-layout
  inversions with equivalent destinations, and constant/label drift.
- Data nonmatches are the jump table/relocation table and extra emitted helper
  symbols/vtables; no missing target symbols remain.

2026-06-13 6:30am MNL recheck: overview still has no missing target rows, and
`python configure.py --non-matching && ninja` linked from source.

2026-06-13 11:31am MNL recheck: overview and unresolved-symbol check still
show no missing target behavior; remaining extras are emitted helpers/vtables
and data ownership. `python configure.py --non-matching && ninja` linked from
source, then `python configure.py && ninja` restored the normal config and
verified `build/GMSJ01/mario.dol: OK`.
