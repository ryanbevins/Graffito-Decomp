# NPC/NpcSave audit

Verdict: equivalent  
Status: certified  
Time: 2026-06-13 4:53pm MNL

Unit: `mario/NPC/NpcSave`  
Source: `src/NPC/NpcSave.cpp`  
Classification: `Object(Equivalent, "NPC/NpcSave.cpp")`

## Verdict

Promoted to `Equivalent` after fixing static ownership for
`TBaseNPC::mPtrSaveNormal`. The target owns `mPtrSaveNormal`,
`TBaseNPC::mAngleYDiffWhenTaken`, and `gpCurrentNpc` in `NpcBase.o` in that
order; rebuilt source previously defined `mPtrSaveNormal` in `NpcSave.o`,
which caused the source-link duplicate with original `NpcBase.o`.

`src/NPC/NpcBase.cpp` now defines:

- `TBaseNPC::mPtrSaveNormal`
- `TBaseNPC::mAngleYDiffWhenTaken`
- `gpCurrentNpc`

and `src/NPC/NpcSave.cpp` only references `mPtrSaveNormal`, matching target
ownership.

Remaining `NpcSave` diffs are codegen/data-owner class:

- `TNpcParams::TNpcParams()` allocates and assigns the same far-clip, normal
  save params, 29 individual save entries, sunflower event init, manager
  static resources, and aliasing cases. The 32.2% text score is pointer-table
  and induction-variable shape, not visible behavior drift.
- Aggregate `.data` differs due local/vtable owner labels from the same params
  and save-name data.

Build proof:

- `python configure.py --non-matching && ninja` passed with `NpcSave` linked
  from source.
- `python configure.py && ninja` passed and verified `build/GMSJ01/mario.dol:
  OK`.

## Reverification

2026-06-13 8:21pm MNL

- Current overview still has only `TNpcParams::TNpcParams()` as nonmatching
  text; the save constructors and rodata remain exact. The extra param vtable /
  dummy labels remain ownership debt, not behavior drift.
- The current tick's `python configure.py --non-matching && ninja` proof linked
  all `Equivalent` rows from source, then `python configure.py && ninja`
  restored the matching config with `build/GMSJ01/mario.dol: OK`.
