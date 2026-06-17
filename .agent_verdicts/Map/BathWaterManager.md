# Map/BathWaterManager Audit

Verdict: needs_impl  
Date: 2026-06-13 4:00am MNL

Not promoted.

Reason:

- `python tools/decomp-diff.py -u mario/Map/BathWaterManager` shows missing
  target text for several vector/box/random/matrix helpers, including
  `JGeometry::TVec3<float>::set<float>(...)`,
  `JGeometry::TBox<TVec3>::operator=`,
  `JMath::TRandom_fast_::get_ufloat_1()`, `TVec3::setMax`,
  `TVec3::setMin`, `SMatrix44C<float>::SMatrix44C()`, and
  `TRotation3<...>::setLookDir(...)`.
- Local constructor/data symbols such as `.ctors` `@1900`, `@1996`-`@1999`
  are also missing, and the rebuilt object emits many target-absent helpers.
- The TU is not symbol-complete and remains `NonMatching`.
