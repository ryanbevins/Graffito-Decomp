# NPC/NpcManager

Verdict: equivalent
Date: 2026-06-14 12:11pm MNL
Unit: `mario/NPC/NpcManager`

Source-link proof:

- `python configure.py --non-matching && ninja` linked with `NpcManager.o`
  sourced.
- `python configure.py && ninja` restored matching config and passed
  `build/GMSJ01/mario.dol: OK`.

Behavior review:

- `TNPCManager::load(JSUMemoryInputStream&)` matches the 52B target body:
  base `TEnemyManager::load(stream)` then `unk3C = 250.0f`.
- `TNPCManager::perform` is equivalent: when `flags & 0x200`, it ORs
  `0x01000000` into every managed actor's live flags, then calls
  `TEnemyManager::perform`.
- `TNPCManager::clipEnemies` is equivalent after the `gpCamera->unk54 == 0xd`
  fix: far-clip source, Dolpic camera widening, other-fast-cube handling, and
  frustum live-flag updates match target behavior.
- `TNPCManager::makePartsModelData_` is equivalent: 12 model slots, two model
  names per slot, pollution flag remapping, `snprintf("%s/%s", folder, name)`,
  resource guard, optional BMT material table, and optional pollution texture
  replacement. Raw target asm confirms the pollution branch uses
  `cRealPollutionTexName` then `cDummyPollutionTexName`; objdiff's displayed
  names are displaced by rodata ownership drift.
- Constructor and generated create/load/destructor variants are behaviorally
  aligned; remaining diffs are frame/register/loop unroll, static-entry rodata
  owner labels, vtable-base address spelling, and harmless extra weak owners.
