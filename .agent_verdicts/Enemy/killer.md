# Enemy/killer

Verdict: equivalent
Status: equivalent
Time: 2026-06-15 11:55am MNL

## Verdict

Certified functionally equivalent and promoted to
`Object(Equivalent, "Enemy/killer.cpp")`.

## Reason

Audit found and fixed three real functional blockers before promotion:
- `TFlyEnemyParams` and `TKillerParams` constructors now call
  `TParams::load(mPrmPath)`, matching the two inlined target loads in
  `TKillerManager::load`.
- `TKiller::init` now re-fetches `getSaveParam()` for `mKillerParams`, guards
  skin-deform allocation on `J3DModel::mSkinDeform`, performs the target
  instance-zero joint scan, and initializes `unk188` to `0.0f`.
- `TKiller::attackToMario` now sends `HIT_MESSAGE_UNKA` when the killer is
  already in `TNerveKillerExplosion`.

Remaining non-exact rows are codegen/data-label/ownership residue, not
behavioral: stack/register drift, local const-pool rows `@1431`, `@1411`, and
`@1210`, source-only weak/helper rows, data/rodata label drift, and the no-op
`TMtx34f` ctor call boundary in `TKillerManager::createEnemyInstance`.

Proofs:
- `python configure.py --non-matching && ninja` linked the from-source object.
- `python configure.py && ninja` restored normal config and passed
  `build/GMSJ01/mario.dol: OK`.
