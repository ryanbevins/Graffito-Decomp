# Enemy/enemyMario Audit

Verdict: needs_impl  
Date: 2026-06-13 4:00am MNL

Not promoted.

Reason:

- `python tools/decomp-diff.py -u mario/Enemy/enemyMario` shows missing target
  text for matrix/path helpers, including
  `TMatrix34<SMatrix34C<float>>::TMatrix34()`,
  `TRotation3<TMatrix44<SMatrix44C<float>>>::TRotation3()`, and
  `TPathNode::getPoint() const`.
- The TU is missing a large amount of target data: dirty texture names,
  many animation/model filename strings, `marioAnimeFiles`,
  record filename tables, `names$3274`, `bmdFileNames$3287`, and many local
  string/constant rows.
- With broad missing data and helper text, this remains `needs_impl`.
