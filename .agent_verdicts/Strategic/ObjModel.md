# Audit verdict: equivalent

Verdict: equivalent
Time: 2026-06-13 6:29am MNL

Date: 2026-06-12 11:57pm MNL

`mario/Strategic/ObjModel` is functionally equivalent.

Reviewed functions:

- `TMActorKeeper::TMActorKeeper(TLiveManager*)`: stack-frame size only.
- `TMActorKeeper::createMActorFromAllBmd(unsigned long)`: same model-data
  count walk, per-index actor creation, and loop bounds; residue is stack
  frame and register choice around the linked-list traversal.
- `TMActorKeeper::createMActor(const char*, unsigned long)`: target/source
  preserve the same lookup-by-key/name, create-and-keep fallback, second lookup,
  actor-model-data-index store, nth-data traversal, `SDLModel` allocation,
  `MActor` allocation, `setModel`, actor-array store, and actor-count
  increment. The low score is inlining/helper-owner shape around
  `createAndKeepData`, `loadModelData`, `registerDataAndJoinNewNode`, and
  `createMActorFromNthData`.
- `TMActorKeeper::getMActor(const char*) const`: codegen-only frame and label
  drift; behavior matches for null keeper fallback, lookup, index scan, and
  null return.
- `TModelDataKeeper::getDataByName(const char*) const`: codegen-only frame and
  helper-owner drift; behavior matches the lookup and nth-data traversal.
- `TModelDataKeeper::createAndKeepData(const char*, unsigned long)`:
  codegen-only stack-frame and local string-buffer slot drift; behavior matches
  path formatting, resource load, `SDLModelData` allocation, node registration,
  and new-node allocation.

Proof:

- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` passed with `build/GMSJ01/mario.dol: OK`.

2026-06-13 6:29am MNL recheck: overview still has no missing target rows, and
`python configure.py --non-matching && ninja` linked from source.

2026-06-13 9:57am MNL recheck:
- Current overview still has no missing target symbols.
- Re-read full diffs for all nonmatching rows:
  `TMActorKeeper::TMActorKeeper(TLiveManager*)`,
  `createMActorFromAllBmd`, `createMActor`, `getMActor`,
  `TModelDataKeeper::getDataByName`, and `createAndKeepData`.
- `createMActor` still looks structural in the pretty diff, but raw source and
  target object review shows the same behavior: lookup by key/name,
  load/register missing model data, re-lookup index, store the model-data index,
  fetch nth data, then create/register the actor. Target inlines the
  `createAndRegister` allocation/setModel body where current source emits the
  helper call; this is helper-owner/source-shape drift, not behavior drift.
- Other rows remain frame/register/label residue only. Proof batch passed:
  `python configure.py --non-matching && ninja`, then `python configure.py &&
  ninja` with `mario.dol: OK`.

2026-06-15 7:18am MNL safety-net recheck:
- Current overview still has no missing target rows. Extra rows remain
  unreferenced helper-owner byte debt (`TModelDataKeeper::getIndex`,
  `TModelDataNode` ctor, `isSameName`) and do not block source-linking.
- Re-read source plus current overview for all nonmatching functions. The
  behavioral sequence is still the same as the prior certification:
  model-data lookup, load/register-on-miss, index re-lookup, nth-data fetch,
  SDLModel/MActor allocation, `setModel`, actor-array store, and count bump.
- No downgrade. The `--non-matching` proof earlier in this tick linked all
  current Equivalent rows, including `ObjModel`, and normal config was restored
  with `build/GMSJ01/mario.dol: OK`.
