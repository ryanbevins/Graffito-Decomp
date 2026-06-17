# mario/Player/MarioMain

Verdict: equivalent
Status: equivalent
Time: 2026-06-14 2:17am MNL

## Verdict

Promoted `Player/MarioMain.cpp` to `Object(Equivalent, ...)`.

## Evidence

- `TMario::drawSyncCallback(unsigned short)` is instruction-identical except
  for local stack slot placement around the `GXPeekARGB` color output.
- `TMario::perform(unsigned long, JDrama::TGraphics*)` no longer has the
  structural `GXSetDstAlpha` argument-width mismatch. The TU-local prototype
  now passes the converted silhouette alpha as a full-width second argument,
  matching the target call ABI; the SDK implementation still keeps its real
  `GXSetDstAlpha(GXBool, u8)` signature.
- Remaining `perform` diffs are codegen-class: frame size, preserved-register
  coloring, stack slots for temporary colors/vectors, and one `cmpwi` versus
  `extsh.` condition test with identical signed-short semantics.
- The overview still reports extra weak JAL list destructor/static symbols
  emitted by the template definitions included through `MSoundBGM.hpp`. The
  original object references those symbols externally; the rebuilt object
  defines weak copies. The from-source link accepted this layout.

## Proof

- `python configure.py --non-matching && ninja`
- `python configure.py && ninja` (`build/GMSJ01/mario.dol: OK`)
