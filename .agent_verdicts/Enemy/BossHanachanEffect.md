# Enemy/BossHanachanEffect Audit

Verdict: equivalent  
Date: 2026-06-14 2:53am MNL

`mario/Enemy/BossHanachanEffect` is certified functionally equivalent and
`configure.py` now marks `Enemy/BossHanachanEffect.cpp` as `Equivalent`.

Build proof:
- `python configure.py && ninja` passed before promotion.
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` passed afterward with `mario.dol: OK`.

Fix made during audit:
- `TWaterHitActor::unk68` is a signed 16-bit water-hit counter, not `s32`.
  Evidence: target `TWaterHitActor::receiveMessage()` and
  `onWaterHitCounter()` store it with `sth`; `TBossHanachan::emitParticle_()`
  reads it with `lha`.

Reviewed nonmatching functions:
- `TBossHanachan::emitCamShake_()` has stack/FPR/register drift, local
  constant label drift, and helper ownership drift around
  `CLBCalcRatio<float>`. Camera shake, rumble, distance-ratio clamping,
  frame checks, and BGM-stop behavior are aligned.
- `TBossHanachan::emitOneTimeSandPillar_(...)` is codegen-class only:
  stack-frame size and local label drift. Position averaging, actor-matrix
  writes, animation selections, shake, and sound gate/call are aligned.
- `TBossHanachan::emitParticle_()` now has the correct signed halfword
  water-hit guard. Remaining deltas are codegen-class: stack/register slots,
  `extsh.` vs `cmpwi` after an `lha`, local constant/sdata drift, and
  probability comparison lowering (`bge` vs `cror` + `beq`). Particle IDs,
  matrix/position bindings, loop bounds, state checks, and water-height tests
  are aligned.

Residual symbol drift:
- `sEmitSandFrameFoot` and several infectious weak/static symbols still show
  missing/extra or data-section drift in objdiff, but the values are present
  and used equivalently. No behavioral target symbol is absent from the
  source-link build.
