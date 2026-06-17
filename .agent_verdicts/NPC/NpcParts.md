# mario/NPC/NpcParts

Verdict: equivalent
Status: promoted
Time: 2026-06-14 1:13pm MNL

## Proof
- `python configure.py --non-matching && ninja` passed and linked
  `build/GMSJ01/mario.dol` with `NPC/NpcParts.cpp` from source.
- `python configure.py && ninja` passed afterward; `build/GMSJ01/mario.dol: OK`.

## Review
- Fixed the previous `partsPerform` blocker: the jellyfish effect path now
  looks up `_starglow1` through `J3DModelData::getMaterialName()` (`+0xb4`),
  matching the target instead of the joint-name table.
- Constructor behavior now matches the target field usage:
  `TNpcModelData+0/+4` are joint attachment names, while `+8/+c` are part
  model names. The target null-checks the part model name before `strcmp` and
  model-data lookup; source now does the same.
- Constructor also now calls `parts->unk18->setLightType(1)` for every
  constructed part, matching the target branch where null pollution color skips
  only packet color initialization.
- Remaining text diffs are codegen-class: stack frame sizes, saved-register
  choices, typed array recomputation vs pointer induction, and equivalent
  branch ordering around actor-type cases. Calls, constants, null checks, and
  stores line up behaviorally.
- Remaining data diffs are owner/layout residue for global string pointers and
  infectious helper strings; the source-link proof confirms no unresolved or
  duplicate-symbol issue for this TU.
