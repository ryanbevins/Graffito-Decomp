## Enemy/areacylinder

Verdict: equivalent
Audited: 2026-06-13 7:25am MNL

Reason:
- `TAreaCylinderManager::getCylinderContains` and `contain` have matching
  list traversal, end-iterator checks, vertical range guards, X/Z distance
  square math, radius-square compare, and return behavior. Diffs are stack slot
  and frame-size residue.
- `TAreaCylinder::load` has the same stream read order, radius/height scaling,
  ignored path/name records, conductor lookup, missing-manager allocation,
  manager registration, list insertion, and final angle conversion. Diffs are
  stack frame size, local temp/register placement, and weak/symbol-order
  residue; object-level disassembly confirms the source build still calls the
  list `insert` helper at the insertion site.
- Extra weak `TList`/`TViewObj` helpers are not target-owned symbols but do not
  change runtime behavior.

Verification:
- 2026-06-12 12:41pm MNL: promoted `Enemy/areacylinder.cpp` to `Equivalent`;
  `python configure.py --non-matching && ninja` linked successfully.
- 2026-06-13 7:25am MNL: reverified current diffs for
  `getCylinderContains`, `contain`, and `TAreaCylinder::load`. Behavior remains
  identical. Rechecked raw source-object relocations: the suspicious
  `decomp-diff.py` call label inside `load` is still the list `insert` helper,
  with only helper symbol-order/owner drift against the target. Source link
  proof passed in the same notes-refresh batch.

2026-06-13 10:49am MNL recheck: verdict remains `equivalent`. Re-read the
current diffs for `getCylinderContains`, `contain`, and `TAreaCylinder::load`.
The manager list traversal, vertical/radius containment tests, stream read
order, radius/height scaling, ignored path/name records, conductor lookup,
missing-manager allocation, manager registration, list insertion, and final
angle conversion are unchanged. Raw target asm confirms the scary pretty label
at the insertion site is the
`TList<TAreaCylinder*>::insert(...)` helper, not a destructor call. Proof
refreshed with `python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
