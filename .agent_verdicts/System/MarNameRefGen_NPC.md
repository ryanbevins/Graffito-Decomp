# mario/System/MarNameRefGen_NPC

Verdict: equivalent
Checked: 2026-06-14 5:16pm MNL
Unit: `mario/System/MarNameRefGen_NPC`

## Result

Safety-net recheck confirms the current `Object(Equivalent, ...)` verdict.
Current overview has all `.text`, `.rodata`, and `.sdata2` target rows
byte-identical and no missing target symbols.

The previous blocker is fixed: `TMarNameRefGen::getNameRef_NPC(const char*)`
passes the shared `"?"` string to
`TMareJellyFishManager::TMareJellyFishManager(const char*)`, matching the target
constructor argument.

Remaining drift is `.data` plus extra helper/static-owner rows
(`JDrama::TViewObj` dtor/vtable, draw-buffer stubs, infectious strings, and
small `.sdata`), with no behavior-bearing references left unresolved.

## Proof

- This tick's `python configure.py --non-matching && ninja` linked all current
  `Equivalent` objects, including `MarNameRefGen_NPC`, from source.
- This tick's `python configure.py && ninja` restored the normal config and
  passed `build/GMSJ01/mario.dol: OK`.
