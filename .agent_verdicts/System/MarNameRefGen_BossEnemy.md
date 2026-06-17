# System/MarNameRefGen_BossEnemy Audit

Verdict: equivalent
Date: 2026-06-13 12:23pm MNL

`mario/System/MarNameRefGen_BossEnemy` is functionally identical and links from
source as `Object(Equivalent, "System/MarNameRefGen_BossEnemy.cpp")`.

Evidence:
- `python configure.py --non-matching && ninja` linked successfully after the
  temporary promotion.
- The only missing target text symbol in overview is
  `JGeometry::TVec3<float>::set<float>(float, float, float)` (16B). The target
  calls it once while constructing `TSleepBossHanachan`; current source inlines
  the same three zero stores to the vector. No source-linked object needs an
  undefined reference to this helper, so it is weak/helper ownership byte debt.
- `TMarNameRefGen::getNameRef_BossEnemy(const char*) const` preserves the same
  ordered name comparisons, allocation sizes, constructor calls, manager
  selector arguments, and null fallback. Remaining text drift is stack-frame
  size, string-label offsets, saved-register coloring, and the `TVec3::set`
  call boundary described above.
- `__sinit_MarNameRefGen_BossEnemy_cpp` has the same JSUPtrList
  initialization and global-object registration pattern; objdiff labels differ
  because the source owns extra JSUList destructor helpers and local labels.

Offending functions: none.
