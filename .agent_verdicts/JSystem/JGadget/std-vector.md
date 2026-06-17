# mario/JSystem/JGadget/std-vector

Verdict: equivalent
Date: 2026-06-13 7:10am MNL

Reason:
- Re-verified during the AUDIT sweep. `python tools/decomp-diff.py -u
  mario/JSystem/JGadget/std-vector` reports no missing target symbols.
- `TVector<void*, TAllocator<void*>>::InsertRaw` follows the same behavior for
  zero-count insertions, in-capacity hole creation, tail copy, growth-size
  calculation, allocation failure return, front/tail uninitialized copies,
  old-buffer deletion, and final begin/end/capacity stores. Refreshed full
  diff residue is saved-GPR coloring and label offsets from extra wrapper
  emissions.
- `TVector_pointer_void::reserve` preserves the same capacity guard,
  allocation, failure return, uninitialized copy, destroy-old-elements path,
  end/begin/capacity stores, and old-buffer deletion. The only structural-looking
  delta is target `mr. r31, r3` versus source `cmplwi r3, 0; addi r31, r3, 0`,
  which is codegen-class and behavior-identical.
- Extra public wrapper/helper emissions are not target-owned symbols; no target
  behavior depends on their presence, and source-link validation succeeds.
- Recheck at 7:10am: the full current diffs still show only register coloring,
  branch-label drift, and null-test lowering. `python configure.py
  --non-matching && ninja` linked from source, then `python configure.py &&
  ninja` passed with `mario.dol: OK`.

Offending functions: none.

2026-06-13 10:43am MNL recheck: verdict remains `equivalent`. Re-read the
current diffs for `TVector<void*, TAllocator<void*>>::InsertRaw` and
`TVector_pointer_void::reserve`. InsertRaw still performs the same zero-count
return, capacity check, in-place/tail copies, growth allocation, old-buffer
delete, and begin/end/capacity updates; reserve still has the same capacity
guard, allocation/failure return, uninitialized copy, old-element teardown,
pointer updates, and old-buffer delete. The remaining drift is saved-GPR
coloring, branch-label layout, and `mr.` versus `cmplwi` null-test lowering.
Proof refreshed with `python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
