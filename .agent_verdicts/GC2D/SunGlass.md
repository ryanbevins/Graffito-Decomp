Verdict: equivalent
Date: 2026-06-13 9:12am MNL
Unit: mario/GC2D/SunGlass
Source: src/GC2D/SunGlass.cpp

Reason:
- Reviewed all six nonmatching functions. `TSunShine::loadAfter`,
  `TSunShine::perform`, `TSunGlass::load`, `TSunGlass::loadAfter`,
  `TSunGlass::perform`, and `TSunGlass::startFade` preserve the same calls,
  branch conditions, field offsets, arithmetic, emitter creation path, draw
  dispatch, fade timer update, and alpha stores.
- Remaining text diffs are codegen-class: stack-frame size/local-slot offsets,
  FPR allocation, and equivalent local constant-label ownership.
- No target symbols are missing. Source emits extra weak/base destructor and
  `JDrama::TViewObj` vtable material, but the target-owned vtables and objects
  match and the source-link validation accepts the TU.

Verification:
- `python configure.py --non-matching && ninja`
- `python configure.py && ninja`
- 2026-06-13 9:12am MNL recheck: full current diffs remain codegen-only
  stack/FPR/constant-label residue. Source-link proof passed and the normal
  matching build verified `build/GMSJ01/mario.dol: OK`.
- 2026-06-13 12:52pm MNL recheck: current overview still has no missing target
  symbols. Re-read all six nonmatching diffs; the load/loadAfter/perform and
  fade paths still preserve the same director/flag reads, emitter creation,
  draw dispatch, alpha interpolation, timer update, and stores. Residue remains
  stack-frame size, local-slot offsets, FPR allocation, and local constant-label
  ownership. Reused this tick's successful source-link and normal DOL proof
  batch.

Offending functions: none.
