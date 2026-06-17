# Enemy/enemymanager Audit

Verdict: `equivalent`
Date: 2026-06-14 9:08am MNL
Unit: `mario/Enemy/enemymanager`

## Reason

The prior `needs_impl` blocker is resolved. `TEnemyManager::mIsCopyAnmMtx` is
now defined in `enemymanager.cpp` with the target initializer `true`, removing
the missing `.sdata` owner and making the TU source-linkable.

This audit also fixed two behavior mismatches before promotion:
- `TEnemyManager::performShared()` now guards the initial `TTimeRec` timer
  start with `unk30 & 1`, matching target behavior.
- `TEnemyManager::perform()` now marks inactive actors with
  `HIT_FLAG_NO_COLLISION` instead of `LIVE_FLAG_DEAD`, matching the target
  store to hit flags at offset `0x64`.

## Proof

- `python configure.py --non-matching && ninja` linked `mario.elf` and
  produced `mario.dol` with `Enemy/enemymanager.cpp` sourced.
- `python configure.py && ninja` restored the normal matching config and
  passed `build/GMSJ01/mario.dol: OK`.

## Residue

- `copyAnmMtx(TSpineEnemy*)`: same animation index/frame check, root matrix and
  sound update, base matrix scaling, `unk48` matrix concat loop, and optional
  weight-envelope update. Remaining drift is stack/temp/register allocation.
- `copyFromShared()`: same shared model copy path, view matrix swap, draw matrix
  copy, and cache store. Remaining drift is stack-slot placement.
- `performShared(...)`: same timer guards, active-count check, shared animation
  updates, flag/sound/copy phases, per-enemy movement/animation/shadow/light
  calls. Remaining drift is stack/register and nested loop lowering.
- `perform(...)`: same draw-buffer calls, timer guards, clipping, inactive
  hit-flag write, empty no-op loop when movement is not requested, per-enemy
  `testPerform`, and timer end. Remaining drift is loop layout and stack size.
- `createEnemies(int)`: same capacity clamps, negative-count return, instance
  creation, add-to enemy group, and `init(this)` call. Remaining drift is stack
  and JGadget iterator helper ownership.
- Data drift is byte debt from vtable/helper owner placement and infectious
  string/TParam weak extras; the sourced vtable entries are behavior-equivalent
  empty helpers.
