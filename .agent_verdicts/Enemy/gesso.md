# Enemy/gesso Audit

Verdict: equivalent  
Date: 2026-06-13 7:12pm MNL

Certification:

- Promoted `Enemy/gesso.cpp` to `Object(Equivalent, ...)`.
- Proof build passed: `python configure.py --non-matching && ninja` linked
  `mario.elf` and rebuilt `mario.dol` with `gesso` from source.
- Normal matching build then passed: `python configure.py && ninja` reported
  `build/GMSJ01/mario.dol: OK`.
- Current overview has no missing target symbols. Extra text/ctor symbols are
  weak/local helper ownership debt (`theNerve()`, `getSightDirection()`,
  `polluteBehavior()`, `TPathNode`, list iterators, attachment/model helpers),
  not undefined source-link blockers.

Implementation fix:

- `TGesso::polluteBehavior()` now returns while
  `mPollutionTimer <= unk1E8->mSLPollutionInterval.get()`, so the pollution
  path proceeds only when `timer > interval`.
- Focused diffs now show the target `ble` interval gate in both inlined copies:
  `TNerveGessoStay::execute()` and `TGesso::walkBehavior(int, float)`.
- Temporary source-link proof passed with
  `Object(Equivalent, "Enemy/gesso.cpp")` and
  `python configure.py --non-matching && ninja`; the row was restored to
  `NonMatching` afterward and normal `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.

Behavior review:

- The previous equality-frame blocker is fixed. Target and source now both
  increment `mPollutionTimer`, require animation end, skip if already in
  `TNerveGessoPollute`, skip while the timer is less-than-or-equal to
  `mSLPollutionInterval`, run `MsIsInSight`, reset the timer, kill the pollute
  object, and push `TNerveGessoPollute`.
- Remaining low-score rows `TGesso::walkBehavior()` and
  `TNerveGessoStay::execute()` still differ mainly because target inlines
  `polluteBehavior()` and `getSightDirection()` while current source emits
  helper owners. The reviewed behavior matches after the interval fix.
- `TGessoManager::initSetEnemies()`, `GessoBodyCallback()`, `TGesso::bind()`,
  `TGesso::setPolluteGoal()`, `TGessoPolluteObj::set()`,
  `TGessoPolluteObj::rebirth()`, `TNerveGessoFreeze::execute()`, and
  `TNerveGessoFall::execute()` remain as previously reviewed: same state
  transitions, velocities, animation IDs, pollution/stamp calls, flag writes,
  and manager/model setup, with residual stack slots, helper-owner labels,
  inline boundaries, and register/FPR coloring.
- Non-text drift is data/label ownership: BAS table/model-entry rows and
  infectious/static helper data differ by symbol ownership/offset accounting,
  not missing runtime data.

Additional audit spot-checks:

- `TGessoManager::initSetEnemies()` allocates `TGessoPolluteModelManager` and
  calls `init()` on the first enemy; target/source differ by constructor
  inlining and stack size only.
- `GessoBodyCallback()` builds the same rotation and scale matrices and
  concatenates them in the same order; residue is stack layout/FPR allocation.
- `TGesso::behaveToFindMario()` source emits a `TPathNode(THitActor*)` call
  where target inlines the constructor stores; raw rebuilt relocations confirm
  objdiff's `genRandomItem()` label there is a misresolve.
