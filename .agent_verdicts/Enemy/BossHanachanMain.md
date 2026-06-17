# Enemy/BossHanachanMain Audit

Verdict: equivalent  
Date: 2026-06-15 8:37pm MNL

Certified `mario/Enemy/BossHanachanMain` as behaviorally equivalent after
fixing one real blocker in `TBossHanachan::init(TLiveManager*)`: target asm
passes common `mSLBodyAttackRadius.value` (`mParams + 0xb8`) as the
`TSphereLink` constructor's fourth float, not change
`mSLSandSlopeForce.value`.

Reviewed the remaining non-exact functions:

- `execDamage`: same hit-point decrement, death/damage nerve selection,
  hit-flag enabling for head/body/feet, rail-name choice, and sound calls;
  residue is loop-unroll/branch/register shape.
- `goToInitialRecoverGraphNode`, `getBodyMaxRotateZ`,
  `checkFallDecideAndSetup`, `CalcRevisionPosByRotateZ`, manager ctor,
  `clipEnemies`, and `loadAfter`: same calls, offsets, constants, and
  stores; residue is stack/register/FPR/helper-boundary drift.
- `execSlip`, `execWalk`, `bind`, and `throwMario_`: same movement,
  graph-node, ground/wall, throw-vector, message, and clamp behavior; residue
  is vector helper inlining, stack placement, and algebraically equivalent
  fused float code.
- `perform`: same death/shine flow, director-blocked freeze, warning balloon,
  movement/body-history updates, sphere-link and wave/tumble math, sand/ground
  slope handling, Mario collision throws, hit-actor flag/map-collision updates,
  particle/camera/animation/model updates, target arrow, draw, view, and effect
  gates. Remaining drift is frame size, register/FPR coloring, helper
  boundaries, loop unroll shape, and data/rodata label layout.

No missing target rows remain. Extra rows are unreferenced weak/helper/static
owners or static-init/data-label debt, not undefined references.

Proof:

- `python tools/decomp-diff.py -u mario/Enemy/BossHanachanMain -s missing`
  reports no missing rows.
- `python configure.py --non-matching && ninja` linked with
  `BossHanachanMain` sourced.
- Plain `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.
