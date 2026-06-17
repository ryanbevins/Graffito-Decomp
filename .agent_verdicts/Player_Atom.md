# mario/Player/Atom

Verdict: equivalent  
Date: 2026-06-13 11:57am MNL

`Player/Atom.cpp` is functionally identical and links from source.

- `TMapCollisionBase::setUp()` and `__sinit_Atom_cpp` are byte-identical.
- The target-owned `.ctors` and `.sdata2` rows match.
- The remaining source-side extra `.rodata` and local `.bss` size/layout drift
  come from the static `cDeformedTerrainCenter` storage/initialization shape.
  No text path observes a different operation, call, store, or branch condition.
- Proof: temporary `Object(Equivalent, "Player/Atom.cpp")` promotion passed
  `python configure.py --non-matching && ninja`, then normal
  `python configure.py && ninja` verified `build/GMSJ01/mario.dol: OK`.
