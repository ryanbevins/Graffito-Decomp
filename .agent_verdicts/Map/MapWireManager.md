# Map/MapWireManager audit

Verdict: equivalent
Status: equivalent
Checked: 2026-06-15 8:13am MNL

Certified `Map/MapWireManager.cpp` as source-linkable and behaviorally
equivalent.

Proof:
- `python configure.py --non-matching && ninja` linked `mario.dol` with
  `Object(Equivalent, "Map/MapWireManager.cpp")`.
- Plain `python configure.py && ninja` then passed `build/GMSJ01/mario.dol: OK`.

Non-exact rows reviewed:
- `TMapWireManager::load()` is frame/stack-slot drift and sdata label naming;
  stream reads, color byte stores, array allocation, cube-wire count, and wire
  init loop match behavior.
- `TMapWireManager::loadAfter()` calls base `loadAfter`, allocates the actor
  manager, constructs/initHitActor's the helper actor, searches the item group,
  inserts the helper actor into the children list, stores the manager, and
  increments `unk1C`. Remaining drift is constructor/helper-boundary, stack,
  register, and label naming residue.
- `JGadget::TList_pointer<THitActor*>::insert(...)` is stack-frame/slot drift
  around the same `TList_pointer_void::insert` call and return iterator copy.
- `TMapWireManager::perform()` has the same three flag-gated passes
  (`doActorToWire`/`move`, hit-flag and taken-mtx sync, upper/lower draw, DB
  entry). Drift is frame/register and misleading symbol labels.
- `TMapWireActorManager::doActorToWire()` has the same cube-wire selection,
  held-object invalidation, Mario take-message loop, wire-change dirty flag,
  release/rebind transitions, and foot-point updates. Drift is frame/register
  only.
- `TMapWireActor::getPosInWire()` computes the same flattened start/end points,
  perpendicular foot, total wire length, partial length, and ratio. Drift is
  helper-boundary/vector temporary stack shape; target and source perform the
  same arithmetic and calls.

No missing text symbols remain; the extra weak/local rows do not create
undefined references and the `--non-matching` source-link build proved the TU
links from source.
