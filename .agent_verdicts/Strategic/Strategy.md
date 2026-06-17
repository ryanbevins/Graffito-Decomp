# Strategic/Strategy audit

Verdict: equivalent
Date: 2026-06-13 7:56am MNL

Reason: all functions are either byte-matching or behaviorally aligned, and
`python configure.py --non-matching && ninja` linked the TU from source.

Fresh recheck: full pass repeated on 2026-06-13 7:56am MNL. No missing target
text symbols were present; all text extras are template/weak helper owners.
Raw asm resolves the constructor's objdiff vtable-label drift back to the
expected `TNameRef` -> `TViewObj` -> `TStrategy` setup.

Function review:
- `TStrategy::TStrategy(const char*)`: base `TViewObj` construction, vtable
  setup, `unk50` zeroing, and all 16 `unk10` null stores match behaviorally.
  Remaining drift is a target constant-false unroll guard and return-register
  scheduling.
- `TStrategy::load(JSUMemoryInputStream&)`: base load, `TObjHitCheck`
  allocation, count read, per-entry memory stream setup, `genObject`, temporary
  `unk10[15]` registration, child load, slot assignment by `unk20`, stream
  destruction, and `gpStrategy` store match. Remaining drift is stack size and
  helper/vtable owner labeling for the local stream setup.
- `perform`, `searchF`, `loadAfter`, destructor, and `TIdxGroupObj` methods
  byte-match.

Notes:
- Source emits extra JDrama/JSU template and weak owners, but the required
  `--non-matching` source-link proof passed.

2026-06-13 11:41am MNL recheck: verdict remains `equivalent`. Current overview
still has no missing target rows. Full current diffs for `TStrategy::load` and
`TStrategy::TStrategy(const char*)` preserve the same base load, object
generation, child load, slot assignment, `gpStrategy` store, vtable/final field
state, and 16 null slot stores; residue is frame size, helper/vtable owner
labels, and one constant-false unroll guard. Shared proof passed with
`python configure.py --non-matching && ninja`, then normal `python configure.py
&& ninja` verified `build/GMSJ01/mario.dol: OK`.
