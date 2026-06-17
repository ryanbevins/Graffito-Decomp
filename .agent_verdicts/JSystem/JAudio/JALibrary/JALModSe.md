# mario/JSystem/JAudio/JALibrary/JALModSe

Verdict: equivalent

Updated 2026-06-14 12:32am MNL in AUDIT mode.

`JSystem/JAudio/JALibrary/JALModSe.cpp` is behavior-equivalent and links from
source under `--non-matching`.

Functional fix made during audit:

- `JALSystem::appendGrpMember(ModType, u32 group_id, u32 member_id)` now
  appends `member_id` to the found group and registers `member_id` with
  `spFManager->addUseFlag(member_id, type)`. The prior source incorrectly used
  the mod type as the member id and registered the group id.

Behavior review:

- `processModDistVolume(...)`: same registration checks for
  `JALSeModVolDist` and `JALSeModVolDGrp`, same `calc`/`calcGrp` value
  propagation, same `1.0f` fallback. Remaining diff is register/template-label
  ownership.
- `append(...)`: same parameter object construction, switch dispatch, concrete
  modifier construction, group modifier construction, and use-flag registration
  for non-group modifiers. Remaining diff is frame size and weak/template owner
  labels.
- `appendGrpMember(...)`: after the fix, same switch key, group lookup key,
  appended member id, and use-flag arguments. Remaining diff is stack/register
  coloring and redundant null-check layout.
- `TFlagManager::TFlagManager()`, `TFlagManager::{addUseFlag,isRegistered}`,
  `JALLinkD<JALSeModDataGrpMemb,u32>` ctor, and the four emitted
  `JALListGrp::searchGroup` instances are behavior-equivalent; remaining diffs
  are stack size, branch layout, loop codegen, and weak/template ownership.
- No missing target symbols. Extra text symbols are weak/template destructor
  and link-owner drift; the source-link proof resolves them.

Proof:

- `python configure.py --non-matching && ninja` linked successfully with this
  TU from source.
- `python configure.py && ninja` passed afterward with
  `build/GMSJ01/mario.dol: OK`.
