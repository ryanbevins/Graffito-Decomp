Verdict: equivalent
Time: 2026-06-13 6:33am MNL
Unit: mario/Enemy/BossHanachanNerve
Source: src/Enemy/BossHanachanNerve.cpp
commit_reviewed: 853fe5a0

Reason:
- Promoted `Enemy/BossHanachanNerve.cpp` to `Object(Equivalent, ...)` and
  proved it with `python configure.py --non-matching && ninja`.
- All behavior-bearing functions are either byte-matching or differ only in
  stack-frame/local-layout residue:
  `TNerveBossHanachanSnort::execute`,
  `TNerveBossHanachanDamage::execute`,
  `TNerveBossHanachanDown::execute`, and
  `TNerveBossHanachanTumble::execute`.
- The visible calls, branches, constants, stores, singleton pushes, and return
  values match the target in those four functions.
- The remaining object/data residue is weak-owner/vtable ordering noise:
  source emits extra weak `TNerveBase<TLiveActor>` and `JSUList<...>`
  artifacts, while the target references several `JALList` statics externally.
  The required source-link build succeeds, so this is not a missing-symbol
  blocker.
- 2026-06-13 6:33am MNL recheck: overview still has no missing target rows,
  and `python configure.py --non-matching && ninja` linked from source.

2026-06-13 10:11am MNL recheck:
- Current overview still has no missing target symbols. Full diffs for
  `Snort`, `Damage`, `Down`, and `Tumble` still preserve the same animation
  setup, slip/death/get-up/tumble checks, BGM/tempo/balloon calls, singleton
  initialization, nerve stack pushes, and return values.
- Remaining residue is stack-frame/local-layout drift, singleton/vtable label
  attribution, and source-owned weak/JSUList artifacts.
