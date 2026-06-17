# System/GCLogoDir audit

Verdict: equivalent

Checked 2026-06-14 11:44am MNL during AUDIT sweep.

Unit: `mario/System/GCLogoDir`

Proof:
- Promoted `System/GCLogoDir.cpp` to `Object(Equivalent, ...)`.
- Fixed `TGCLogoDir::direct_nlogo()` state 3 to wait on
  `TProgSelect::mHideTextBoxes`; the target reads `lbz 0x16(r3)`, matching
  the header layout.
- `python configure.py --non-matching && ninja` linked cleanly with this TU
  sourced.
- Restored normal config and `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.

Audit notes:
- `direct_nlogo`, `direct_dolby`, `direct`, and the destructor now differ only
  by stack-frame size, register allocation, compare form, labels, and branch
  layout around equivalent state updates and fader calls.
- `setup` constructs the same stage/view object tree, textures, progressive
  selector, render rect, ortho projection, screen assignments, fader color, and
  wipe. Remaining diffs are constructor inlining, iterator temporary layout, and
  weak-symbol ownership.
- The missing `TVec3<float>::set`, ctor labels, rodata/data drift, and extra
  JDrama/JSU/MSound weak owners are symbol-accounting residue; they do not block
  the from-source `--non-matching` link.
