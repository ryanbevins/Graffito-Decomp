Verdict: equivalent
Time: 2026-06-13 6:30am MNL
Unit: mario/MoveBG/MapObjOption
Source: src/MoveBG/MapObjOption.cpp
commit_reviewed: df388a32

Reason:
- Promoted `MoveBG/MapObjOption.cpp` to `Object(Equivalent, ...)` and proved
  it with `python configure.py --non-matching && ninja`.
- The four nonmatching functions are behavior-equivalent:
  `TFileLoadBlock::initMapObj`, `TFileLoadBlock::loadAfter`,
  `TFileLoadBlock::receiveMessage`, and `TFileLoadBlock::touchPlayer`.
- Remaining text diffs are rodata base-offset relabeling and stack-frame
  residue. The actual comparisons, particle resource load, name searches,
  inlined block-push behavior, rumble call, particle emits, timer stores, and
  return values match the target.
- Remaining object/data residue comes from extra weak helpers, infectious
  strings, and vtable/data ownership/layout differences; the source-link build
  succeeds and no target behavior-bearing symbol is missing.
- 2026-06-13 6:30am MNL recheck: overview still has no missing target rows,
  and `python configure.py --non-matching && ninja` linked from source.
- 2026-06-13 10:00am MNL recheck: full `--no-collapse` diffs for
  `initMapObj`, `loadAfter`, `receiveMessage`, and `touchPlayer` still show no
  structural behavior drift. Name comparisons, particle resource load, partner
  block lookup, head/message activation gates, BCK start, card selection,
  rumble call, two particle emits, and timer propagation match target behavior.
  Remaining residue is rodata/SDA owner labels, stack frame size, and extra weak
  helper ownership. Proof rerun passed:
  `python configure.py --non-matching && ninja`, then normal
  `python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
