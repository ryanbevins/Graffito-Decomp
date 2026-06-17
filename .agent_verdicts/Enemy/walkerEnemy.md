Verdict: equivalent
Time: 2026-06-13 6:33am MNL
Unit: Enemy/walkerEnemy

Audited all nonmatching text symbols and promoted the TU to
`Object(Equivalent, ...)` in `configure.py`.

- No missing target symbols in overview.
- `TNerveWalkerTraceMario::execute`, `TNerveWalkerEscape::execute`,
  `TNerveWalkerAttack::execute`, `TWalkerEnemy::behaveToFindMario`,
  `reset`, and related `setGoalPathMario()` sites match behavior. The noisy
  residue is `TPathNode` construction emitted out-of-line in source versus
  inlined stores in target, plus singleton label/name drift.
- `moveObject`, `init`, `isReachedToGoalXZ`, `isResignationAttack`, and
  `TWalkerEnemyParams` match behavior; remaining diffs are stack/register
  allocation, helper ownership, and constant label placement.
- `.data`/`.sdata2` aggregate rows are noisy, but the raw target `.data` is
  vtables already represented by matching per-symbol rows, and the remaining
  constants are used equivalently by the audited functions.
- Proof: `python configure.py --non-matching && ninja` linked
  `build/GMSJ01/mario.dol` from source, then plain `python configure.py &&
  ninja` passed and verified `build/GMSJ01/mario.dol: OK`.
- 2026-06-13 6:33am MNL recheck: overview still has no missing target rows,
  and `python configure.py --non-matching && ninja` linked from source.

2026-06-13 11:10am MNL recheck:
- Verdict remains `equivalent`.
- Current overview still has no missing target symbols. Re-read all ten
  nonmatching text functions: the three walker nerves,
  `isReachedToGoalXZ`, `isResignationAttack`, `behaveToFindMario`, `reset`,
  `moveObject`, `init`, and `TWalkerEnemyParams`.
- Behavior still matches: graph/path target selection, Mario-trace goal copies,
  escape/attack state transitions, jump setup, random wait interval selection,
  stack/path-node push-pop behavior, and parameter construction use the same
  calls, stores, constants, branch predicates, and object offsets.
- Remaining differences are codegen-class: local `TPathNode` construction vs.
  inlined stores, stack frame/slot layout, induction-variable/register coloring,
  singleton/static label ownership, and TParam/vtable label attribution. The
  11:08am proof batch linked from source with
  `python configure.py --non-matching && ninja`, then normal
  `python configure.py && ninja` restored the matching build.
