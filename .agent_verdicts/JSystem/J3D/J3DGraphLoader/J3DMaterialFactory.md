Verdict: equivalent
Time: 2026-06-13 6:31am MNL
Unit: mario/JSystem/J3D/J3DGraphLoader/J3DMaterialFactory
Source: src/JSystem/J3D/J3DGraphLoader/J3DMaterialFactory.cpp
commit_reviewed: bab503c3

Reason:
- Promoted `JSystem/J3D/J3DGraphLoader/J3DMaterialFactory.cpp` to
  `Object(Equivalent, ...)` and proved it with
  `python configure.py --non-matching && ninja`.
- No target symbols are missing. The behavior-bearing function calls in
  `J3DMaterialFactory::create` match the target relocation sequence after the
  expected offset drift from extra weak/helper ownership.
- Remaining function diffs are stack-frame/local-temp placement in `create`,
  `newIndTexOrder`, `newIndTevStage`, and `newNBTScale`. Branches, table/index
  math, default constants, virtual dispatch slots, and returns match target
  behavior.
- The small `.sdata2` residue is an extra leading constant referenced by
  source-owned weak/helper code (`calcColorChanID`/helper ownership); the target
  constants remain present and the source-link build succeeds.
- 2026-06-13 6:31am MNL recheck: overview still has no missing target rows,
  and `python configure.py --non-matching && ninja` linked from source.

2026-06-13 11:07am MNL recheck:
- Verdict remains `equivalent`.
- Current overview still has no missing target symbols. Re-read all current
  nonmatching functions: `create`, `newIndTexOrder`, `newIndTevStage`, and
  `newNBTScale`.
- `create` still performs the same table reads, factory calls, virtual material
  setter calls, loop bounds, and copies. The suspicious pretty labels around
  `newTevSwapModeTable`, `newAmbColor`, and `newIndTevStage` are relocation /
  owner-label drift on matching `bl` instructions, not changed behavior.
- The three small constructors/copy helpers differ only by stack frame and
  temporary-slot placement while copying the same default or indexed data.
- Remaining `.sdata2` residue is source-owned helper constant placement.
