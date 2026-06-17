verdict: equivalent
date: 2026-06-15 5:02pm MNL
tu: mario/Enemy/elecNokonoko
source: src/Enemy/elecNokonoko.cpp

Audit result:
- Certified `Equivalent` after fixing one real behavior mismatch in
  `TNerveElecNokonokoCollect::execute()`: when the collect animation frame is
  greater than 32, target calls the carapace virtual at vtable offset `0xe4`,
  which is `TElecCarapace::kill()`. The old source called `shoot()` at
  `0x144`; source now calls `kill()`.
- `python tools/decomp-diff.py -u mario/Enemy/elecNokonoko -s missing` reports
  no missing rows.
- Remaining non-exact functions are behavior-equivalent codegen debt:
  stack/register placement, branch layout, helper inlining/call-boundary
  differences (`TVec3::sub`, `TUtil::sqrt`, list iterators, `TPathNode` copies),
  random expression shape, and local data-label/constant-pool ordering.
- Non-exact data rows are byte/layout debt, not wrong contents:
  `dennoko_bastable` has the same BAS path/null table, `entry$2916` has the same
  model load entry, and `@4318` is the same `30.0f` constant.

Proof:
- `python configure.py --non-matching && ninja` linked with
  `Enemy/elecNokonoko.cpp` sourced.
- `python configure.py && ninja` passed with `build/GMSJ01/mario.dol: OK`.
