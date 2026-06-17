# JSystem/JDrama/JDRFrmGXSet audit

Verdict: equivalent
Date: 2026-06-13 4:15pm MNL

Re-verified the existing `Equivalent` certification during the audit fallback
sweep.

- `python tools/decomp-diff.py -u mario/JSystem/JDrama/JDRFrmGXSet` still
  shows no missing target rows.
- `TFrmGXSet::perform` preserves the perform-bit guard, current framebuffer
  selection, render-mode struct copy, seven `unk64` display flag transfers into
  `TGraphics::unkFC`, FB clamp, clear color, and clear-Z stores.
- Remaining drift is frame size (`0x130` target vs `0x90` source) and
  register/base-pointer coloring around repeated `TDisplay` accesses.
- Source-link proof is covered by this tick's successful
  `python configure.py --non-matching && ninja`; normal config was restored
  with `python configure.py && ninja` and verified `build/GMSJ01/mario.dol: OK`.

Verdict: equivalent
Date: 2026-06-13 9:10am MNL

Reason: the only nonmatching function is behaviorally aligned, and
`python configure.py --non-matching && ninja` linked the TU from source.

Function review:
- `JDrama::TFrmGXSet::perform(unsigned long, JDrama::TGraphics*)`: the perform
  flag guard, frame-buffer/render-mode copies, seven display flag transfers,
  FB clamp, clear color, and clear-Z stores match the target. Remaining drift
  is stack frame size and volatile-register coloring.
- `JDrama::TFrmGXSet::~TFrmGXSet()`: byte-matches.

Notes:
- Source emits extra `JDrama::TViewObj` weak data/text, but the required
  `--non-matching` source-link proof passed.
- Reverified this pass against the full current diff: the source still follows
  the target's perform-flag guard, copy/store sequence, and GX state writes.
  The residue remains stack-frame shape plus volatile/GPR coloring, not a
  behavioral difference.
- 2026-06-13 9:10am MNL proof: `python configure.py --non-matching && ninja`
  linked from source, then `python configure.py && ninja` restored the matching
  config and verified `build/GMSJ01/mario.dol: OK`.

Offending functions: none.
