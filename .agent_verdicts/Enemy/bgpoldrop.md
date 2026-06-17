# mario/Enemy/bgpoldrop

Verdict: equivalent  
Date: 2026-06-13 11:57am MNL

`Enemy/bgpoldrop.cpp` is functionally identical and links from source.

- `TBGPolDrop::launch()` and `TBGPolDrop::move()` are byte-identical.
- `TBGPolDrop::perform()` has the same active-state gates, movement call,
  matrix setup, rotation/source velocity handling, particle binding, scale
  updates, animation calls, and pollution stamping behavior. Its 100.0%
  nonmatching display is relocation/local-owner drift.
- The constructor's residue is label/stack-slot ownership over the same base
  construction and field initialization.
- Source still carries an explicit `char trash[0x10]` stack/frame-size helper
  in `move()`, but the current audit directive treats this as byte-matching
  debt, not a behavioral blocker; the real operations are present and exact.
- Proof: temporary `Object(Equivalent, "Enemy/bgpoldrop.cpp")` promotion passed
  `python configure.py --non-matching && ninja`.
