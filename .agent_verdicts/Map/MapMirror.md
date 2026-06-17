verdict: equivalent
date: 2026-06-15 10:48am MNL
unit: mario/Map/MapMirror

Reason:
- Certified `Object(Equivalent, "Map/MapMirror.cpp")`.
- Reviewed all non-exact functions. Near-exact rows are stack-frame/register/
  local-label drift with the same loads, stores, calls, branch conditions, and
  constants. `TMirrorCamera` constructor differs mainly by target inlining the
  `JDrama::TCamera` base construction while source calls the same base ctor.
- `TMirrorModelManager::perform()` is behavior-identical. Target calls local
  `TVec3<float>::set<float>` and `dot()` helper boundaries where source inlines
  the same component stores/dot products; both paths preserve mirror selection,
  illegal-ground guard, reflected eye/target/up math, `C_MTXLookAt`,
  light/effect-matrix setup, material tex-mtx update, and `MActor::entry()`.
- Remaining missing local rows are byte debt: `TVec3<float>::set<float>` has no
  undefined reference in the rebuilt object, and the `@1490`/`@2111`/
  `@2167`-`@2170` rodata rows are local zero/MActor string labels not required
  by source-linking.

Proof:
- `python configure.py --non-matching && ninja` linked with MapMirror sourced.
- `python configure.py && ninja` passed `build/GMSJ01/mario.dol: OK`.
