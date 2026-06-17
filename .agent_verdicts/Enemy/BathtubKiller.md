# Enemy/BathtubKiller audit

Verdict: equivalent  
Status: source_link_proven  
Updated: 2026-06-15 6:37pm MNL

## Audit verdict

`mario/Enemy/BathtubKiller` is behaviorally equivalent and source-linkable.
Flipped `configure.py` to `Object(Equivalent, "Enemy/BathtubKiller.cpp")`.

Proofs:

- `python configure.py --non-matching && ninja` linked cleanly with
  `BathtubKiller` sourced.
- `python configure.py && ninja` passed `build/GMSJ01/mario.dol: OK`.

Remaining debt is byte/codegen only:

- The target's `createEnemyInstance()` calls the 4-byte empty
  `TMatrix34<TSMtx34f>` constructor for `unk220`; the rebuilt path omits it,
  but the weak constructor is just `blr` and has no behavior.
- Large function diffs (`makeQuat`, `makeInitialVelocity`, state nerves,
  `perform`, `bind`) are vector/quaternion helper inlining, stack/FPR/register
  placement, and symbol-label drift. Reviewed calls, stores, branch
  conditions, constants, and state transitions match the target behavior.
- Data drift is rodata/sdata label/base layout plus extra unreferenced weak
  owners; no missing target rows remain.

## Implementation handoff

Current implementation pass cleared the strict missing-symbol blockers:

- `python tools/decomp-diff.py -u mario/Enemy/BathtubKiller -s missing` now
  reports no rows.
- `JGeometry::TVec3<float>::set<int>(int, int, int)` is present and 100%.
  The break/explosion inline paths now route zero velocity construction through
  a small return-by-value helper, preserving the target local `set<int>` owner.
- `TBathtubKillerParams` `PARAM_INIT` defaults now match the target constructor
  values, restoring the formerly missing `.sdata2` constants `@5698`, `@5722`,
  `@5724`, `@5726`, and `@5731`.

## Proof

Temporary `Object(Equivalent, "Enemy/BathtubKiller.cpp")` with
`python configure.py --non-matching && ninja` linked cleanly. Restored
`NonMatching`; normal `python configure.py && ninja` passed
`build/GMSJ01/mario.dol: OK`.

## Audit notes

Remaining diffs are codegen/source-shape residue, not known missing behavior:
extra weak/base helper owners, caller stack/copy residue around the restored
zero-vector helper, parameter constructor label/register drift, and broad
pre-existing BathtubKiller function mismatches that need behavioral review by
AUDIT before promotion.
