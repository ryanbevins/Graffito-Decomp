## Enemy/enemytable

Verdict: equivalent
Audited: 2026-06-13 7:25am MNL

Reason:
- The only nonmatching function, `TStageEnemyInfoTable::getMatchedInfo`, keeps
  the same behavior: first pass sums matching weights, zero total returns
  `nullptr`, random selection scales `rand()` by `1.0f / (RAND_MAX + 1)` and
  `weightSum`, then the second pass subtracts matching weights and returns the
  first entry where the running value is no longer positive.
- Diffs are codegen-class: stack frame size, saved-register choices for
  `weightSum`/`this`/cached begin-end fields, and local stack offsets.
- Extra JSU/TVector weak destructor emissions are not target-owned behavior.

Verification:
- 2026-06-12 12:45pm MNL: promoted `Enemy/enemytable.cpp` to `Equivalent`;
  `python configure.py --non-matching && ninja` linked successfully.
- 2026-06-13 7:25am MNL: reverified current `getMatchedInfo` diff. The
  two-pass matching-weight sum and random weighted selection remain
  behavior-identical; residue is frame size, saved-register coloring, and
  begin/end field caching shape. Source link proof passed in the same
  notes-refresh batch.

2026-06-13 10:49am MNL recheck: verdict remains `equivalent`. Re-read the
current `getMatchedInfo` diff. The two-pass matching-weight sum, zero-total
null return, random scale by total weight, subtracting second pass, and first
non-positive weighted match return are unchanged. Drift is stack frame size,
saved-register choices, and begin/end field caching shape. Proof refreshed with
`python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
