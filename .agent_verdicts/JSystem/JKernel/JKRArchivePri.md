Verdict: equivalent

Date: 2026-06-13 7:10am MNL
Unit: `mario/JSystem/JKernel/JKRArchivePri`
Source: `src/JSystem/JKernel/JKRArchivePri.cpp`
Classification: `Object(Equivalent, "JSystem/JKernel/JKRArchivePri.cpp")`

Reason:
- Re-verified during the audit sweep together with `JKRArchivePub`, because the
  archive vtable/data ownership is split differently between target and source.
  The overview has no missing target text symbols. It reports source-owned
  helper/vtable extras, while `JKRArchivePub` reports the paired missing vtable
  row.
- `JKRArchive::JKRArchive(long, EMountMode)` performs the same base
  construction, vtable store, mount-mode/count stores, heap lookup/current-heap
  fallback, entry-number store, and first-current-volume setup. The only diff
  is stack-frame/save-slot size plus vtable label ownership.
- `findDirectory`, `findTypeResource`, `findFsResource`, and
  `findNameResource` preserve the same archive table walks, CArcName hashing,
  hash/name comparisons, directory/file flag tests, recursive lookups, null
  returns, and returned entry pointers. Full diffs show stack slot drift for
  `CArcName` locals and label noise from source-owned `isSameName` /
  `findResType` helpers; raw target asm confirms the recursive call targets.
- `findTypeResource` and `findFsResource` are byte-identical apart from stack
  local offsets and helper-owner labels. Other functions match exactly.
- Recheck at 7:10am: raw objdump confirms the scary `findDirectory` call label
  in objdiff is only label drift; the rebuilt relocation calls
  `findDirectory__10JKRArchiveCFPCcUl`, matching target asm. No structural
  diffs were found.

Proof:
- `python configure.py --non-matching && ninja` linked
  `build/GMSJ01/mario.dol` from source.
- `python configure.py && ninja` restored the matching config and passed
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

2026-06-13 10:45am MNL recheck: verdict remains `equivalent`. Re-read the
current diffs for the `(long, EMountMode)` constructor plus
`findDirectory`, `findTypeResource`, `findFsResource`, and `findNameResource`.
The constructor still performs the same base construction, vtable store,
mount-mode/count stores, heap/current-heap fallback, entry-number store, and
first-current-volume setup. The lookup helpers still walk the same directory
and file tables, build the same `CArcName` locals, test the same hash/name/type
conditions, recurse through the same directory ids, and return the same null or
entry pointers. The `findDirectory`/`findFsResource` scary labels remain paired
archive-vtable/helper ownership drift, not wrong calls. Proof refreshed with
`python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.

2026-06-14 5:58pm MNL safety-net recheck: verdict remains `equivalent`.
Current overview has no missing target text symbols. Fresh `--no-collapse`
diffs for the constructor, `findDirectory`, `findTypeResource`,
`findFsResource`, and `findNameResource` show only frame/stack-local and
helper/vtable owner-label drift. Rechecked the rebuilt relocation at the scary
`findDirectory` recursive call; it targets
`findDirectory__10JKRArchiveCFPCcUl`, not `findResType`. This tick's
`--non-matching` and normal proof builds covered the current source-linked
object.
