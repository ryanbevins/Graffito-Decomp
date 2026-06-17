# MoveBG/MapObjRicco

Verdict: equivalent
Date: 2026-06-14 10:28am MNL

Reason: re-read every nonmatching text diff after the weak-owner fix. The
remaining differences are codegen/data-owner debt, not behavioral changes.
`MapObjRicco.o` now owns the target weak `TLiveActor::getMActor() const` body,
and `python configure.py --non-matching && ninja` links successfully with Ricco
sourced. Normal `python configure.py && ninja` also passes with
`build/GMSJ01/mario.dol: OK`.

Fixed this audit:
- Recovered target static tunables from `.sdata` for cranes, watermill, and
  fruit launcher.
- `TFruitLauncher::loadAfter` now starts BCK `"riccoswitch"`, not `"@1490"`.
- `TFruitLauncher::fireObj` now calls `J3DModel::calc()`, uses three
  20/40/60/80 fruit-selection attempts, writes launch velocity at
  `mVelocity`, and clears `mLiveFlag`.
- `TSurfGesoObj::initMapObj` now colors `GX_TEVREG1`.
- `TRiccoWatermill::control` now plays the top-arrival sound whenever the
  timer expires, while guarding only the one-time throw with `unk144`.
- `TCraneUpDown::control` writes its rotation matrix into the model's node
  matrix before transforming the cargo.

Remaining byte/codegen debt:
- `TFruitLauncher::fireObj`: frame/FPR lifetime drift, common-store vs
  per-branch toggle lowering, repeated fruit-selection result register moves,
  and velocity random-expression scheduling. Calls, fruit IDs, thresholds,
  position/velocity writes, live-flag clear, demo camera, sound, and
  `killByTimer` semantics match.
- `TRiccoWatermill::control`: frame/FPR drift and switch-body layout/register
  order. The watermill/submarine state transitions, timer gates, sound calls,
  collision toggles, throw target, and flag stores match behaviorally.
- `TCraneUpDown::control`: switch case body order differs, but states 0/1/2/3
  perform the same timer and state stores; matrix/position update and sound
  gate are behaviorally identical.
- Other nonmatching text functions are stack/register/string-label or helper
  boundary residue.
- Data/rodata drift is anonymous constant/string/JALList owner accounting; the
  vtables and recovered static gameplay tunables are present.

Audit evidence:
- `python tools/decomp-diff.py -u mario/MoveBG/MapObjRicco`
- `python tools/decomp-diff.py -u mario/MoveBG/MapObjRicco --search "getMActor"`
- `python tools/decomp-diff.py -u mario/MoveBG/MapObjRicco -d "<symbol>"`
  for all nonmatching text functions.
- `python configure.py --non-matching && ninja` passed after the owner fix and
  again after promotion.
- `python configure.py && ninja` passed with `build/GMSJ01/mario.dol: OK`.
