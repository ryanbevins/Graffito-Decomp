# JSystem/JDrama/JDREfbCtrl Audit

Verdict: equivalent
Date: 2026-06-13 4:14pm MNL

Re-verified the existing `Equivalent` certification during the audit fallback
sweep.

- `python tools/decomp-diff.py -u mario/JSystem/JDrama/JDREfbCtrl` still
  shows no missing target rows.
- `TEfbCtrl::perform` keeps the same `GXSetColorUpdate`,
  `GXSetAlphaUpdate`, `GXSetZMode`, and display-rect copy semantics; residue
  is saved-register/frame layout.
- `TEfbCtrlDisp::perform` keeps the same pixel-format setup, inherited EFB
  state setup, copy guard, clear-color copy, and `IssueGXCopyDisp` arguments;
  residue is stack-slot and argument evaluation order.
- `TEfbCtrlTex::setTexAttb` still writes the same `GXGetTexObjAll` outputs to
  `mImagePtr`, `mTexFmt`, `mWidth`, and `mHeight`; target homes width/height
  through stack words, while current source stores them directly.
- Source-link proof is covered by this tick's successful
  `python configure.py --non-matching && ninja`; normal config was restored
  with `python configure.py && ninja` and verified `build/GMSJ01/mario.dol: OK`.

Verdict: equivalent
Date: 2026-06-13 9:10am MNL
Unit: mario/JSystem/JDrama/JDREfbCtrl
Source: src/JSystem/JDrama/JDREfbCtrl.cpp

Reason:
- `TEfbCtrlTex::perform(unsigned long, JDrama::TGraphics*)` is byte-exact after
  the source-rectangle and `GXBool` clear-flag fixes from prior ticks.
- Re-reviewed the remaining nonmatching text:
  `TEfbCtrl::perform` differs only by stack frame size and saved-register
  coloring; the GX update calls and display-rect copy are identical.
  `TEfbCtrlDisp::perform` preserves the pixel-format setup, inherited EFB
  state update, copy guard, clear-color copy, clear-Z/FBClamp/flags arguments,
  and display copy call; residue is stack-slot/register/argument-evaluation
  order only. `TEfbCtrlTex::setTexAttb` writes the same GX texture object
  outputs to the same fields; residue is a widened temporary/stack-home shape.
- No missing symbols. Extra rows are weak/base destructor and `TViewObj` vtable
  ownership drift, not runtime behavior gaps.
- Proof: `python configure.py --non-matching && ninja` linked from source, then
  plain `python configure.py && ninja` passed and verified `mario.dol: OK`.
- 2026-06-13 9:10am MNL recheck: full current diffs still show no behavior
  drift; overview has no missing target rows, source-link proof passed, and
  the normal matching build verified `build/GMSJ01/mario.dol: OK`.

Offending functions: none.
