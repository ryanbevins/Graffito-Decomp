# mario/Enemy/poihana

Verdict: equivalent
Status: equivalent
Time: 2026-06-15 4:05pm MNL

## Verdict

Certified `Equivalent`.

## Reason

All target symbols are present; `python tools/decomp-diff.py -u
mario/Enemy/poihana -s missing` reports no missing rows. The prior
implementation pass cleared the two local-data blockers (`@4324`, `@3390`) and
fixed the real behavior mismatches in sleep randomization, throw rotation, and
the trapped fling equality check.

Reviewed remaining non-exact text rows:

- `TNervePoihanaTrapped::execute`: same trap state transitions, airborne/death
  paths, vector subtraction, zero-vector guard, normalization, random min/max
  intervals, velocity stores, and BCK changes. Remaining drift is stack size,
  local-slot/FPR allocation, and helper-boundary display noise.
- `TPoiHana::walkBehavior`: same wake timer threshold and
  `-500 + int(1000 * MsRandF())` sleep offset; target materializes the integer
  range through stack while source folds the float range constant.
- `TPoiHana::init` and `TPoiHanaManager::load`: same list insertion, collision
  setup values, parameter construction, defaults, and `TParams::load` call.
  Remaining drift is iterator/frame/register and label ownership.
- `TPoiHana::isCollidMove`, `genEventCoin`, `moveObject`,
  `TNervePoihanaThrow::execute`, `TNervePoihanaFreeze::execute`,
  `TNervePoihanaSleep::execute`, and `TPoiHanaManager::initSetEnemies`: same
  calls, stores, constants, branch conditions, and loop bounds; diffs are frame,
  local placement, FPR/GPR coloring, and data-label drift.

Data-section drift is byte debt only: the actor/vtable/BAS table rows match, and
remaining `.rodata`/`.data`/`.sdata`/`.sdata2` differences are label/extra-owner
residue from source-only weak/static ownership.

Proof:

- `python configure.py --non-matching && ninja` linked with
  `Object(Equivalent, "Enemy/poihana.cpp")`.
- Plain `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.
