# Enemy/SleepBossHanachan Audit

Verdict: equivalent  
Date: 2026-06-13 1:59pm MNL

Kept as `Object(Equivalent, "Enemy/SleepBossHanachan.cpp")`.

Proof:

- `python tools/decomp-diff.py -u mario/Enemy/SleepBossHanachan` shows no
  missing target symbols. Current extra text/data rows are helper/vtable/static
  ownership drift only.
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored the matching config and verified
  `build/GMSJ01/mario.dol: OK`.

Behavior review:

- `TNerveSBH_Fall::execute()` performs the same animation-end guard, shine
  spawn with demo strings and fall position, shine/map flags, live flag write,
  mirror actor flag/write, shape hide call, sleep-continue nerve push, and
  boolean return.
- `TSleepBossHanachan::startFall()` sets the same flag-manager bit, stores the
  fall coordinates, switches BCK index, refreshes animation sound, installs
  the fall nerve, and clears the current spine slot.
- All other functions are exact. Remaining drift is stack-frame size, local
  slot offsets, singleton/data symbol names, and extra emitted base/helper
  owners. Raw `.data`/relocation review shows source-only `MtxCalcTypeName`,
  `SMS_NO_MEMORY_MESSAGE`, `TNerveBase<TLiveActor>` vtable, and weak actor/
  manager thunks before the same behavior-bearing nerve, actor, and manager
  vtable entries.
