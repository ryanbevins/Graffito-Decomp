# mario/Animal/AnimalManager

Verdict: equivalent  
Date: 2026-06-13 12:02pm MNL

`Animal/AnimalManager.cpp` is functionally identical and links from source.

- `TMewManager::createModelData()` calls the same model-data virtual with an
  equivalent static entry table; the target/source entry symbol names differ.
- `TMewManager::loadAfter()` and `load()` preserve the same base calls,
  `MSRandPlay::createRandPlayVec`, animal-save allocation, and view-clip field
  setup; residue is frame/register/local-owner drift.
- `TAnimalManagerBase::clipEnemies(...)` performs the same perspective clip
  setup, object loop, Y offset, other-fast-cube test, frustum check, and live
  flag set/clear behavior. Remaining differences are register coloring,
  induction-variable shape, and stack/frame layout.
- Source still carries `volatile u8 _pad[8]` in `clipEnemies()`, but under the
  current audit directive this is byte-matching debt, not a behavioral blocker;
  the real operations are present.
- Proof: temporary `Object(Equivalent, "Animal/AnimalManager.cpp")` promotion
  passed `python configure.py --non-matching && ninja`.
