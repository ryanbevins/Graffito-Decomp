Verdict: equivalent
Time: 2026-06-13 6:31am MNL
Unit: mario/System/MarDirector
Source: src/System/MarDirector.cpp

Reason:
- Reviewed both nonmatching functions. `registerEventWatcher` preserves the
  same `unk80` list insertion via end iterator and inserted watcher pointer;
  remaining residue is stack/local iterator layout and helper-label ownership.
- `TMarDirector::TMarDirector` preserves base construction, vtable writes,
  five `TPerformList` allocations/constructions, member zero/default stores,
  `TVector_pointer` construction and `reserve(100)`, demo-info array
  construction, global `gpMarDirector` assignment, `initLoadParticle`, state
  byte clears, and `OSInitStopwatch`. Source emits redundant zero stores into
  each allocated `TPerformList` list node, but `TSingleNodeLinkList::Initialize_`
  immediately writes the same list fields, so behavior is unchanged.
- No target symbols are missing. Extra weak/helper emissions are not
  target-owned and source-link validation accepts the TU.

Verification:
- `python configure.py --non-matching && ninja`
- `python configure.py && ninja`
- 2026-06-13 6:31am MNL recheck: overview still has no missing target rows,
  and `python configure.py --non-matching && ninja` linked from source.
- 2026-06-13 11:41am MNL recheck: full current diffs for
  `registerEventWatcher` and `TMarDirector::TMarDirector()` still show only
  frame/local iterator layout, helper-owner labels, redundant equivalent
  `TPerformList` zeroing before list initialization, and register coloring.
  Shared proof passed with `python configure.py --non-matching && ninja`, then
  normal `python configure.py && ninja` verified `build/GMSJ01/mario.dol: OK`.
