# Camera/sunmodel audit

Verdict: Equivalent

Reviewed: 2026-06-13 8:27am MNL

Unit: `mario/Camera/sunmodel`

## Evidence

- `python tools/decomp-diff.py -u mario/Camera/sunmodel` reports no missing
  target text symbols.
- `TSunModel::TSunModel` and `TSunModel::getZBufValue` are byte-identical.
- `CLBScreenFPosToSPos` differs only in stack-frame layout and local label
  ownership.
- `TSunModel::calcDispRatioAndScreenPos_` preserves the visible-count loop,
  2D-position calculation, inner/outer sun position generation, and 17-entry
  screen-position conversion loop. Residue is stack/register layout and helper
  label drift.
- `TSunModel::perform` preserves the same indoor/screen-window gate, brightness
  chase paths, camera-to-sun vector normalization, map-static-object update,
  frame update, material animator update, model entry, and view-calc behavior.
  Remaining differences are stack/register layout, bool temporary layout, and an
  inlined camera-position copy in place of the target's `TVec3::set` helper.
- `TSunModel::load` preserves the same BMD/BTK load path, material animator
  setup, material-indirect data copies, color/frame initialization, mirror
  object creation, transform copy, and insertion into the sun scene list. The
  two material word copies use opposite temporary load order but store the same
  source words to the same destination fields.
- `python configure.py --non-matching && ninja` linked successfully with the
  object as `Equivalent`.
- `python configure.py && ninja` passed the normal build and verified
  `build/GMSJ01/mario.dol: OK`.

## Residual risk

- Several local rodata names and weak/helper destructors still drift, so this is
  certified as functionally equivalent only, not byte-matching.

2026-06-13 11:44am MNL recheck: verdict remains `equivalent`. Current overview
is unchanged: no missing target text rows, same nonmatching sun-model methods,
and the same missing anonymous rodata names paired with source-named
string/vtable extras. The existing review remains valid for perform,
display-ratio/screen-position conversion, load, constructor, and helper
behavior. Shared proof from this tick passed: `python configure.py
--non-matching && ninja`, then normal `python configure.py && ninja` verified
`build/GMSJ01/mario.dol: OK`.
