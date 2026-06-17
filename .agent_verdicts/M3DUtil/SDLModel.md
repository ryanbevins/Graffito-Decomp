# M3DUtil/SDLModel audit

Verdict: equivalent
Date: 2026-06-14 11:12am MNL
Unit: `mario/M3DUtil/SDLModel`

Promoted to `Object(Equivalent, "M3DUtil/SDLModel.cpp")`.

Fix before certification:
- Corrected `SDLModel::entryModelDataSDL()` bump-matrix allocation. The target
  keeps `matsWithBumpMtxs` fixed while the inner `k < param_3` loop fills all
  view entries for a material, then advances the material slot once after the
  loop. Source previously advanced the slot inside the view loop, which
  diverged when `param_3 > 1`.

Reviewed functions:
- `SDLModel::viewCalcSimple()` matches the draw-matrix buffer swap, camera
  matrix concatenation over all draw matrices, and `DCStoreRange` flush.
  Remaining diffs are frame/register/offset spelling.
- `SDLModel::entry()` matches the disabled path that clears bit 0 and calls
  `J3DModel::entry()`, plus the SDL path that marks the model active, searches
  existing draw-buffer tokens by opaque/translucent buffers, inserts into an
  existing token chain when found, or creates and registers a new token.
- `SDLModel::entryModelDataSDL()` matches model-data ownership, flag setup,
  joint/envelope/draw/nrm/shape/material/display-list allocation, bump-matrix
  counting, bump pointer allocation, corrected per-view bump matrix allocation,
  model-data bump flagging, and vertex-buffer creation.
- `SDLModelData::entrySDLModels()`, `recursiveEntry()`, `entryNode()`, and
  `entrySameMat()` match the material traversal, shape-flag skip, active-model
  chain selection, same-material packet aggregation, draw-buffer choice, flag
  clearing, and token reset behavior. Remaining visible residue is
  inline-boundary/helper-label drift (`J3DMatPacket` labels in objdiff),
  recursion unrolling, and stack/register allocation.

Proof:
- `python configure.py --non-matching && ninja` links with `SDLModel.o` built
  from source.
- `python configure.py && ninja` restores the normal matching config and passes
  `build/GMSJ01/mario.dol: OK`.

Residual debt:
- Source still emits extra weak/list/helper owners and local rodata labels, but
  no missing target symbols remain and source-link proof succeeds.
