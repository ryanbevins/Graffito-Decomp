# mario/Enemy/tamaNoko

Verdict: equivalent
Status: equivalent
Time: 2026-06-15 4:31pm MNL

## Verdict

Certified `Equivalent`. All remaining text and data drift is codegen/helper
boundary or label/layout debt; no reviewed diff changes behavior.

## Evidence

- Missing target local data rows `@4983` and `@4337` are now present and exact.
  `@4983` is the flower part name `"タマノコフラワー"`; `@4337` is the
  `0.8f` land-effect particle scale.
- `TTamaNoko::landEffect()` now applies `mScaling * 0.8f` to all four emitted
  land particles, matching the target behavior; score moved `64.0% -> 94.6%`.
- `python tools/decomp-diff.py -u mario/Enemy/tamaNoko -s missing` reports no
  missing rows.
- Reviewed remaining non-exact rows:
  `TNerveTamaNokoWait/HitWater/Sink/Thrown/Attack::execute`,
  `TTamaNoko::isReachedToGoal`, `getGravityY`, `setAfterDeadEffect`,
  `landEffect`, `requestShadow`, `calcRootMatrix`, `receiveMessage`,
  `walkBehavior`, `TTamaNokoManager::initSetEnemies/load`, and
  `TTamaNokoFlower::perform`.
- Remaining text drift is stack-frame/local placement, register/FPR coloring,
  equivalent compare/branch orientation, target-inlined vs source-owned helper
  boundaries (`TPathNode`, `TVec3::scale`, `TRotation3::identity33`, nerve
  singletons), and objdiff label drift. The same calls, loads, stores,
  constants, loop bounds, state transitions, particles, sounds, and flag writes
  are present.
- Remaining data drift is rodata/data/sdata label/layout debt from extra weak
  owners and infectious/static data; required strings, vtables, BAS table
  entries, save-parameter defaults, and local constants are present.
- Promoted to `Object(Equivalent, "Enemy/tamaNoko.cpp")`; proof:
  `python configure.py --non-matching && ninja` links from source. Plain
  `python configure.py && ninja` also passes `build/GMSJ01/mario.dol: OK`.

## Remaining review items

- Byte-matching debt only. `TNerveTamaNokoThrown::execute` still scores
  `75.9%`, but reviewed diff shows
  the same behavior: parameter fetch, Mario throw angle/power trig, velocity
  assignment, `mPosition.y += 2.0f`, airborne flag, and land completion path.
  Remaining mismatch is trig/table load order, stack frame, and register
  placement.
- `TTamaNokoFlower::perform` still scores `94.0%`; raw asm and source agree on
  talk-mode gating, one-shot flower spawn, five-object radial placement,
  normalized outward velocity, animation/sound update, viewCalc/entry, and
  actor perform. Remaining mismatch is stack/local placement and matrix/vector
  temporary shape.
