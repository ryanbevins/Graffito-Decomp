# GC2D/MovieRumble

Verdict: `Equivalent` reverified.

## 2026-06-13 1:29pm MNL

- Proof earlier this tick, before any source/config changes:
  - `python configure.py --non-matching && ninja` linked `mario.elf` and `mario.dol`.
  - `python configure.py && ninja` linked and checksum passed.
- Reviewed `TMovieRumble::init(const char*)`, `perform(unsigned long, JDrama::TGraphics*)`, and `checkRumbleOff()`.
- `init` matches behavior: builds `/subtitle/rnbl/%s`, rewrites the extension to `.bcr`, allocates/attaches `Koga::ToolData`, initializes `unk18` from `dataExists`, reads the current rumble row, and clears `unk28`.
- `perform` matches behavior: bit-0 gated movement, `unk28` selects stop/read-next versus start-at-frame behavior.
- `checkRumbleOff` matches behavior: guarded by valid rumble index and frame threshold, stops rumble, increments row index, inlines the same `readCurInfo` sequence (`start_frame`, `end_frame`, `type` / `RumbleType::getIndex`), and clears `unk28`.
- Residual drift is codegen/data-owner only: frame size, GPR coloring inside the inlined `readCurInfo`, source-owned string/vtable labels, and dummy rodata/sdata ownership.
