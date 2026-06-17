# mario/NPC/NpcTrample

Verdict: equivalent
Date: 2026-06-13 8:39am MNL

Reverified current `Object(Equivalent, ...)` row during the audit sweep.

Reason:
- `python tools/decomp-diff.py -u mario/NPC/NpcTrample` still reports no
  missing or extra symbols.
- Full `--no-collapse` diff for `TNpcTrample::updateTrample(float, float*)`
  shows identical branches, timer stores, save-param loads, sine-table lookup,
  output store, and return behavior. Residue is FPR operand permutation and
  const-label numbering only.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored normal config and verified
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

2026-06-13 12:49pm MNL recheck: verdict remains `equivalent`. Fresh full diff
for `TNpcTrample::updateTrample(float, float*)` still shows the same inactive
pass-through, active countdown/decrement stores, transition into the recover
timer from `mPtrSaveNormal`, sine-table weighted output calculation, and
boolean return. Differences are FPR operand permutation, branch-label offsets,
and local constant labels only. Proof reused from this tick:
`python configure.py --non-matching && ninja`, then normal `python configure.py
&& ninja` with `build/GMSJ01/mario.dol: OK`.

---

Verdict: equivalent
Date: 2026-06-13 4:24am MNL

Reason:
- `python tools/decomp-diff.py -u mario/NPC/NpcTrample` reports no missing or
  extra symbols. `TNpcTrample::startTrample()` and data symbols match exactly.
- `TNpcTrample::updateTrample(float, float*)` is 99.3% and exact-size. The full
  `--no-collapse` diff shows identical branches, loads, stores, calls, and
  constants; the residue is FPR operand permutation and const-label numbering.
- Re-verification of existing `Object(Equivalent, ...)` linked cleanly under
  `python configure.py --non-matching && ninja`.

Offending functions: none.
