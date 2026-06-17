# Enemy/conductor audit

## Verdict: equivalent
Status: equivalent
Time: 2026-06-13 9:20pm MNL

Unit: `mario/Enemy/conductor`
Source: `src/Enemy/conductor.cpp`
Classification: `Object(Equivalent, "Enemy/conductor.cpp")`

## Verdict

Certified `Enemy/conductor` as behaviorally equivalent after recovering the
`gpConductor` small-data owner in `conductor.cpp`.

## Review

- `perform`, `genEnemyFromPollution`, `killEnemiesWithin`,
  `makeOneEnemyAppear`, `makeEnemyAppear`, `init`, and `isBossDefeated` all
  preserve the target operations and control-flow semantics. Remaining text
  diffs are iterator temporary stack slots, frame size, register allocation,
  local label ownership, and equivalent switch lowering (`isBossDefeated`
  still treats map 3 as the boss-gesso case and all other maps as the
  hinokuri/default case).
- The target-visible rodata and vtable entries are exact. Residual data drift
  is owner/static debt from source-only helper/vtable emission, not a runtime
  behavior difference for the source-linked TU.
- The previous linker blocker is gone: `TConductor* gpConductor;` now owns the
  target `.sbss` global referenced by source-linked dependents.

## Validation

- `python configure.py --non-matching && ninja` linked successfully with
  `Enemy/conductor` from source.
- `python configure.py && ninja` passed with `build/GMSJ01/mario.dol: OK`.

Verdict: needs_impl
Status: needs_impl
Time: 2026-06-13 5:35am MNL

Unit: `mario/Enemy/conductor`
Source: `src/Enemy/conductor.cpp`
Classification: `Object(NonMatching, "Enemy/conductor.cpp")`

## Verdict

Do not promote yet. The function bodies reviewed in this sweep are
behavior-aligned: `perform`, `genEnemyFromPollution`, `killEnemiesWithin`,
`makeOneEnemyAppear`, `makeEnemyAppear`, `init`, and `isBossDefeated` only show
stack/register, helper-owner, rodata-label, or equivalent branch-layout
residue.

Temporary promotion to `Object(Equivalent, "Enemy/conductor.cpp")` failed the
source-link proof because replacing the target object leaves `gpConductor`
undefined. The linker reported references from many sourced objects, including
`SDLModel.o`, `effectObj.o`, `liveactor.o`, `enemy.o`, `MarioEffect.o`,
`MarioAutodemo.o`, enemy/boss TUs, `MapObjLib.o`, `smallEnemy.o`, `NpcBase.o`,
`NpcInitPrg.o`, `NpcManager.o`, and `generator.o`, then stopped after too many
errors.

Leave this TU red until the `gpConductor` data owner is recovered, then
re-audit and source-link again.
