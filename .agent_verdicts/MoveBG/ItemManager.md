# MoveBG/ItemManager audit

Verdict: equivalent  
Date: 2026-06-13 6:45am MNL
Status: reverified current source-link proof

Proof: `python configure.py --non-matching && ninja` links cleanly with
`MoveBG/ItemManager.cpp` built from source. All target `.text` functions,
`TItemManager` vtable, and `gpItemManager` are present. A follow-up plain
`python configure.py && ninja` restored the matching configuration and verified
`build/GMSJ01/mario.dol: OK`.

Reason: Non-100% function diffs
are codegen-class stack-frame/slot, local label ownership, and `mr`/`addi`
operand-encoding residue.

Reviewed functions:
- `TItemManager::newAndRegisterCoin(unsigned long)`: same blue/empty/red coin
  selection, object creation, `gpItemManager->unk78` path, and `unk134` store;
  residue is stack placement and local string/static labels.
- `TItemManager::makeShineAppearWithDemoOffset(...)`: same name lookup,
  position offset adds, `appearWithDemo`, and return; residue is `addi r4,r30,0`
  vs `mr r4,r30`.
- `TItemManager::makeShineAppearWithDemo(...)`: same name lookup, absolute
  position stores, `appearWithDemo`, and return; residue is `addi` vs `mr`.
- `TItemManager::makeShineAppearWithTime(...)`: same name lookup, absolute
  position stores, `appearWithTime`, and return; residue is stack placement and
  `mr`/`addi`.
- `TItemManager::resetNozzleBoxesModel(int)`: same object scan, nozzle-box type
  and stage checks, live-flag particle/sound path, emitter scale writes, and
  `makeModelValid`.

Notes: source emits extra unused weak/base owners and a target-absent
4-byte `makeShineAppearWithTimeOffset` stub. No source references to that helper
were found, and the from-source link succeeds.

Reverified: 2026-06-13 10:54am MNL — still equivalent. Re-read the exact
`newAndRegisterCoin(unsigned long)` diff plus the shine helpers and
`resetNozzleBoxesModel`. Coin selection/creation, shine lookup and position
stores, `appearWithDemo`/`appearWithTime` calls, nozzle-box scan, particle/sound
path, and `makeModelValid` behavior still match. Remaining drift is
stack/temp placement, local string/data owner labels, and `mr` versus `addi`
encoding. Proof passed again with `python configure.py --non-matching && ninja`,
then plain `python configure.py && ninja` restored the matching config and
verified `build/GMSJ01/mario.dol: OK`.
