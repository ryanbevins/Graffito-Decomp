Verdict: equivalent
Time: 2026-06-13 6:31am MNL
Unit: mario/JSystem/J3D/J3DGraphLoader/J3DMaterialFactory_v21
Source: src/JSystem/J3D/J3DGraphLoader/J3DMaterialFactory_v21.cpp
commit_reviewed: f1ecd2d5

Reason:
- Promoted `JSystem/J3D/J3DGraphLoader/J3DMaterialFactory_v21.cpp` to
  `Object(Equivalent, ...)` and proved it with
  `python configure.py --non-matching && ninja`.
- `J3DMaterialFactory_v21::create` has identical call targets by relocation
  record and matching branch/setup/store behavior. Remaining diffs are stack
  temporary slot offsets and local/weak helper label ownership.
- `J3DMaterialFactory_v21::newNBTScale` differs only by stack frame/temporary
  offsets while copying the same default or indexed NBT scale data.
- Remaining `.sdata2`/rodata and extra text residue comes from local weak
  template/helper ownership; no target behavior-bearing symbol is missing, and
  the source-link build succeeds.
- 2026-06-13 6:31am MNL recheck: overview still has no missing target rows,
  and `python configure.py --non-matching && ninja` linked from source.

2026-06-13 11:07am MNL recheck:
- Verdict remains `equivalent`.
- Current overview still has no missing target symbols. Re-read both current
  nonmatching functions: `create` and `newNBTScale`.
- `create` still preserves the same factory call sequence, virtual setter slots,
  loop bounds, material table offsets, and data copies. The displayed call-label
  drift is helper/relocation ownership noise on otherwise matching branch
  instructions.
- `newNBTScale` still selects either the indexed NBT-scale record or the same
  default record; all residue is frame/temporary offset drift.
