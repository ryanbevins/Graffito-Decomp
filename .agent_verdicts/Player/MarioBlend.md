# mario/Player/MarioBlend

Verdict: equivalent
Date: 2026-06-13 6:06am MNL

Reason:
- Re-verified during the AUDIT sweep. `updateOut`, `updateIn`,
  `changeMtxCalcSIAnmBQAnmTransform`, vtable rows, and data rows match
  exactly. The source-owned extra `M3UModel::setMtxCalc` and anonymous rodata
  rows are owner/data drift.
- Full `--no-collapse` diff for `M3UModelMario::updateInMotion()` shows
  identical behavior: same loop bounds/stride, frame-control update, joint
  lookup, `0xff` mtx-calc clear branch, optional transform frame writes,
  basic-quaternion slot stores, fallback current-mtx-calc load, and final joint
  mtx-calc store. Remaining mismatches are stack-frame/save-slot size and
  branch-label address drift.
- Proof: `python configure.py --non-matching && ninja` linked from source,
  then normal `python configure.py && ninja` passed and verified
  `mario.dol: OK`.

2026-06-13 10:11am MNL recheck:
- Current overview still has no missing target symbols. `updateInMotion` keeps
  the same loop bounds/stride, frame-control update, joint lookup, `0xff`
  mtx-calc clear branch, optional transform-frame writes, BQ slot stores,
  fallback current-mtx-calc load, and final joint mtx-calc store.
- Remaining residue is stack-frame/save-slot size and branch-label address
  drift.

Offending functions: none.
