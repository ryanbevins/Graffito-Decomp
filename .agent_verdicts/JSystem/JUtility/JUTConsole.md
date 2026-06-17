# JSystem/JUtility/JUTConsole Audit

Verdict: equivalent  
Date: 2026-06-13 8:27am MNL

Reverified `JSystem/JUtility/JUTConsole.cpp` as
`Object(Equivalent, ...)`.

Reviewed functions:
- `JUTConsole::doDraw(JUTConsole::EConsoleType) const`: same visibility/font
  guards, active/direct console handling, ortho setup, fill-box color choice,
  font color selection, line iteration, font draw path, and direct-print path.
  Residue is stack-frame size, saved-register/FPR coloring, temporary color
  stack placement, and one unused post-draw conversion spill.
- `JUTConsole::print(const char*)`: same output guard, newline/tab/ordinary
  character handling, line wrap, line attribute updates, scroll/head/tail
  index maintenance, and final NUL store. Residue is equivalent branch layout
  and register choice around the wrap-to-zero path.
- `JUTConsoleManager::removeConsole(JUTConsole*)`: same active-console
  replacement, warning/report console clearing, and link-list removal.
  Residue is stack iterator placement and helper label drift around the tiny
  console accessor functions.
- `JUTConsoleManager::setDirectConsole(JUTConsole*)`: same old direct-console
  reinsertion, active-console fallback, new direct-console removal, warning/
  report clearing, link-list removal, and final direct-console store. Residue
  is stack iterator placement and helper label drift.

Extras:
- Source-owned `J2DOrthoGraph` / link-list destructors and varargs wrapper
  helpers link cleanly and do not correspond to missing target text.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` passed afterward and verified
  `build/GMSJ01/mario.dol: OK`.

2026-06-13 12:54pm MNL recheck:
- Current overview still has no missing target symbols.
- Re-read all four nonmatching diffs. `doDraw` still has the same
  visibility/font guards, ortho setup, fill-box and font color choices,
  line iteration, font draw path, and direct-print path; `print` still has the
  same output guard, newline/tab/ordinary-character handling, wrapping,
  scroll/head/tail updates, and final NUL store; the manager methods still
  perform the same active/direct-console replacement, report/warning clearing,
  list removal/reinsertion, and direct-console store.
- Residue remains frame size, stack color/iterator placement, saved register
  coloring, equivalent wrap-to-zero branch layout, and helper-label ownership.
  Reused this tick's successful source-link and normal DOL proof batch.
