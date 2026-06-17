# System/MovieDirector audit

Verdict: equivalent

Checked 2026-06-14 11:50am MNL during AUDIT sweep.

Unit: `mario/System/MovieDirector`

Proof:
- Promoted `System/MovieDirector.cpp` to `Object(Equivalent, ...)`.
- `python configure.py --non-matching && ninja` linked cleanly with this TU
  sourced.
- Restored normal config and `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.

Behavior fixes made during audit:
- `rsetup()` movie range gates now use signed compares like target.
- The optional end-save `TCardSave` is constructed with the default name and
  `true` boolean argument, matching target `li r5, 1`.
- THP render placement now centers with
  `((u16)renderSize - videoSize) / 2` for both axes, matching the target
  truncation and arithmetic order.
- `direct()` transition to `STATE_SAVE_TO_TITLE` now uses
  `startWipe(14, 0.3f, 0.0f)` like target.

Audit notes:
- `direct`, `decideNextMode`, `setup`, and the destructor differ only by
  stack-frame size, register allocation, constructor/helper ownership, branch
  layout, and labels after the fixes above.
- `rsetup` now constructs the same subtitle/end-save archives, view lists, THP
  render/subtitle/rumble/card-save objects, render rect, ortho projection,
  screen assignments, THP player setup, audio-track selection, and render
  bounds. Remaining diffs are list-iterator temporary layout, constructor
  inlining boundaries, stack copies versus direct stores, and weak-symbol/data
  ownership.
- Missing `TVec3<float>::set`, ctor labels, movie table labels, and extra
  JDrama/JSU/MSound weak owners are symbol-accounting residue; they do not
  block the from-source `--non-matching` link.
