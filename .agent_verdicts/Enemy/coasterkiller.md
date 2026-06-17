# Enemy/coasterkiller audit

Verdict: `equivalent`

Checked 2026-06-14 10:59am MNL in AUDIT mode.

Promoted `Enemy/coasterkiller.cpp` to `Object(Equivalent, ...)` after the
implementation pass fixed the previous blockers:

- `TCoasterEnemyParams` owns the two mutable speed defaults in `.sdata`
  (`20.0f`, `16.0f`). Objdiff still reports target local labels `@2842/@2843`
  as missing and named source symbols as extra, but section contents and source
  behavior match.
- `TCoasterKiller::perform` gates the particle/sound block on `(param_1 & 2)`
  and `!LIVE_FLAG_DEAD`, matching the target guard.
- `TCoasterKiller::setDeadAnm` now scales the generated `TEffectExplosion`
  actor, not the killer.

Current remaining diffs are byte/codegen debt:

- `loadAfter`: target has a larger frame; operations match.
- `load`: parameter construction and `TParams::load` order match, with
  register/stack drift and a redundant same-value `unk38` store in source.
- `setDeadAnm`: label/register spelling only after the generated-effect fix.
- `perform`: target calls `JGeometry::TUtil<float>::sqrt`, source inlines the
  same sqrt math before the sound gate; singleton guard labels differ.
- `bind`: stack slots differ; rebuilt relocation is to
  `TVec3<float>::sub`, despite objdiff's misleading symbol-label display.
- `moveCoaster`: large quaternion/vector expression-shape drift, but same graph
  target normalization, speed select, velocity scale, steering, optional tilt,
  and final quaternion normalization.
- Data/sdata mismatch is local-label/extra weak-owner debt; all target vtables
  and behavior-bearing data are present.

Proof:

- `python configure.py --non-matching && ninja` linked successfully with
  `coasterkiller.o` sourced.
- `python configure.py && ninja` restored the normal matching config and passed
  `build/GMSJ01/mario.dol: OK`.
