# mario/JSystem/JParticle/JPAResourceManager

Verdict: equivalent
Date: 2026-06-13 8:39am MNL

Reverified current `Object(Equivalent, ...)` row during the audit sweep.

Reason:
- `python tools/decomp-diff.py -u mario/JSystem/JParticle/JPAResourceManager`
  still reports no missing target symbols and no data-section mismatches.
- Full diffs for `JPATextureResource::registration(...)` and
  `JPAResourceManager::load(const void*, unsigned short)` show identical
  texture lookup/allocation/table update and emitter registration logic.
  Residue is stack frame/save-slot offsets and branch-label addresses only.
- Extra `JPAEmitterResource` helper bodies are source-owned API drift; target
  call sites inline the same logic and source-link proof is clean.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored normal config and verified
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

2026-06-13 12:52pm MNL recheck: verdict remains `equivalent`. Fresh full diffs
for `JPATextureResource::registration` and
`JPAResourceManager::load(const void*, unsigned short)` still show identical
texture-name lookup, allocation, `JPATexture` construction, table store/count
increment, emitter loader call, user-index store, emitter table insertion, and
count increment. Extra `JPAEmitterResource` helper bodies remain source-owned
API drift. Proof refreshed with `python configure.py --non-matching && ninja`,
then normal `python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.

---

Verdict: equivalent
Date: 2026-06-13 4:42am MNL

Reason:
- Re-verified during the AUDIT sweep. `python tools/decomp-diff.py -u
  mario/JSystem/JParticle/JPAResourceManager` reports no missing target
  symbols and no data-section mismatches.
- `JPATextureResource::JPATextureResource`, `JPAEmitterResource::getByUserIndex`,
  `JPAResourceManager::JPAResourceManager`, and
  `JPAResourceManager::load(const char*, unsigned short)` match exactly.
- `JPATextureResource::registration(const unsigned char*, JKRHeap*)` is
  exact-size and 99.9%. The refreshed full `--no-collapse` diff shows
  identical texture name lookup, allocation, constructor call, table store,
  count increment, and return; only stack frame / save-slot offsets differ.
- `JPAResourceManager::load(const void*, unsigned short)` is exact-size and
  99.8%. The refreshed full diff shows identical loader call and inlined
  emitter registration logic; only stack frame / save-slot offsets and branch
  label addresses differ.
- The extras are unreferenced out-of-line helper bodies for
  `JPAEmitterResource::JPAEmitterResource(...)` and
  `JPAEmitterResource::registration(...)`; the target call sites inline the same
  logic.
- Promoting `JSystem/JParticle/JPAResourceManager.cpp` to
  `Object(Equivalent, ...)` linked cleanly under
  `python configure.py --non-matching && ninja`; retained as `Equivalent`
  pending the batch source-link proof.

Offending functions: none.
