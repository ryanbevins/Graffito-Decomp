# mario/MSound/MSoundScene

Verdict: equivalent
Status: certified_equivalent
Time: 2026-06-14 4:16am MNL

## Reason
Re-audited the stale `not_equivalent` verdict, fixed the real pool-ordering
bug, and promoted `MSound/MSoundScene.cpp` to `Object(Equivalent, ...)`.

Behavior fix made during audit:
- `MSSceneSE::frameLoop()` and `MSSceneSE::sortMaxTrans()` now keep nearer or
  equal candidates ahead of farther candidates. The target computes the new
  candidate distance before the current slot, then replaces when
  `currentDist >= newDist`. The old source replaced when
  `currentDist < newDist`, which selected the opposite pool ordering.
- Nested slot comparisons in `sortMaxTrans()` now use the same `>=` rule.

Remaining reviewed diffs are codegen/ownership only:
- `sortMaxTrans()` now has the same candidate/current distance call order, same
  `cror eq, gt, eq` replacement condition, same recursive demotion behavior,
  and same slot writes. Residue is register/index-width shape and recursive
  helper label drift.
- `frameLoop()` pointer-copy setup writes the same effective addresses despite
  different `this + offset + 4` versus `this + offset` plus store-displacement
  shape. Sector classification, pool insertion, averaging, gate checks, and SE
  start calls are behavior-equivalent.
- `__sinit_MSoundScene_cpp` and extra `JSUList` destructors are static/weak
  ownership byte-debt.

## Proof
- `python configure.py --non-matching && ninja` linked successfully with
  `MSound/MSoundScene.cpp` source-linked.
- `python configure.py && ninja` passed afterward with
  `build/GMSJ01/mario.dol: OK`.
