# mario/JSystem/J2D/J2DWindow

Verdict: equivalent
Time: 2026-06-13 6:30am MNL

## Verdict
equivalent

## Date
2026-06-12 5:26am MNL

## Reason
All target symbols are present and the two nonmatching emitted functions are
codegen-only:

- `J2DWindow::resize(int, int)`: instruction stream and tree-iteration behavior
  are identical; only the target frame is larger (`0xe0` vs `0xb8`) and saved
  register offsets follow from that frame delta.
- `J2DWindow::draw_private(const JUTRect&, const JUTRect&, float (*)[3][4])`:
  drawing calls, branch structure, constants, and memory offsets are identical.
  Residue is a callee-saved register-coloring choice (`r29` vs `r25`) for a
  texture-coordinate value in two late draw calls, plus shifted local branch
  labels.

`python configure.py --non-matching && ninja` linked cleanly after promoting the
TU.

2026-06-13 6:30am MNL recheck: overview still has no missing target rows, and
`python configure.py --non-matching && ninja` linked from source.

2026-06-13 10:00am MNL recheck: full `--no-collapse` diffs still show only
codegen-class residue. `draw_private` preserves the same GX setup, texture
presence gates, corner/edge draw sequence, TEV reset, and final vtx-desc reset;
the late difference is the known `r29`/`r25` register choice for a
texture-coordinate argument. `resize` is stack-frame/saved-register offset drift
only. Proof rerun passed: `python configure.py --non-matching && ninja`, then
normal `python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.

2026-06-13 1:21pm MNL recheck: verdict remains `equivalent`. Full current
diffs still preserve the same bounds guards, matrix concat/load, border/content
draw sequence, texture-null fallback, TEV/vtx reset, and resize propagation to
visible child panes. Residue is branch target address drift, the same late
GPR-coloring swap for texture-coordinate arguments, and frame-size/save-slot
offsets. Proof rerun passed with `python configure.py --non-matching &&
ninja`, then normal `python configure.py && ninja` verified
`build/GMSJ01/mario.dol: OK`.
