# Enemy/riccohook audit

Verdict: equivalent  
Status: certified  
Time: 2026-06-13 4:47pm MNL

Unit: `mario/Enemy/riccohook`  
Source: `src/Enemy/riccohook.cpp`  
Classification: `Object(Equivalent, "Enemy/riccohook.cpp")`

## Verdict

Promoted to `Equivalent`. The old source-link blocker was a header declaration
error: `TRiccoHookManager` declared a `perform(u32, JDrama::TGraphics*)`
override that does not exist in the original symbol list. The target manager
vtable slot points at `TEnemyManager::perform`, so removing the bogus override
declaration makes `TRiccoHookManager::__vtable` byte-match and removes the
undefined `TRiccoHookManager::perform` reference.

Remaining nonmatching text is `TRiccoHook::init(TLiveManager*)`. It is
behavior-equivalent: same spine init, nerve singleton init, hit flag, `THookTake`
allocation/setup, hit actor radii, NameRef group insertion, graph reset, march
speed/turn speed stores, and live-flag side effect. Diffs are stack-frame/slot
drift, static/local label ownership, and helper-owner attribution around
JGadget iterator helpers.

Build proof:

- `python configure.py --non-matching && ninja` passed with `riccohook` linked
  from source.
- `python configure.py && ninja` passed and verified `build/GMSJ01/mario.dol:
  OK`.
