# mario/MoveBG/WoodBarrel

Verdict: equivalent
Date: 2026-06-13 6:19am MNL

Reason:
- All data, rodata, sdata, and ctor rows match, and no target symbol is
  missing.
- Most owned functions match exactly. The remaining nonmatching functions
  (`appear`, `appeared`, `kill`, `hold`, and `put`) preserve the same calls,
  predicates, constants, stores, flag updates, sound/particle/water paths, and
  returns. Their residual diffs are stack-frame/local-slot size and local-label
  ownership only.
- Extra text is source-owned weak helper/destructor ownership (`TMapObjGeneral`,
  `THitActor`, `TTakeActor`, `JDrama::TActor`, and `JSUList` helpers).
- Source-link proof passed under `python configure.py --non-matching && ninja`,
  then normal `python configure.py && ninja` restored the matching config and
  verified `build/GMSJ01/mario.dol: OK`.

2026-06-13 10:11am MNL recheck:
- Current overview still has no missing target symbols. Full diffs for
  `appear`, `appeared`, `kill`, `hold`, and `put` still preserve the same
  state transitions, sound/particle/water requests, hip-drop radius update,
  ground-height flag updates, holder/ground actor predicates, and flag stores.
- Remaining residue is frame/local-slot size and local-label ownership.

2026-06-14 3:28pm MNL recheck:
- Current source is still `Object(Equivalent, "MoveBG/WoodBarrel.cpp")`.
- `python tools/decomp-diff.py -u mario/MoveBG/WoodBarrel` still reports no
  missing symbols; all data/rodata/sdata/sdata2 rows match exactly.
- Re-read full no-collapse diffs for `appear`, `appeared`, `kill`, `hold`, and
  `put`. The only current differences are stack-frame/local-slot offsets and
  label ownership (`@NNN`/local static names). Calls, predicates, flag
  stores, holder notification, water emit request, hip-drop damage-height
  update, and ground/airborne live-flag transitions are unchanged.
- This tick's `python configure.py --non-matching && ninja` proof after the
  MathUtil promotion also source-linked the current WoodBarrel object; normal
  `python configure.py && ninja` then passed `build/GMSJ01/mario.dol: OK`.

Offending functions: none.
