# mario/JSystem/J3D/J3DGraphAnimator/J3DCluster

Verdict: equivalent
Status: source_link_proven
Time: 2026-06-14 8:02am MNL

## Proof

- Re-audited the stale `needs_impl` verdict under the current source-link rule.
  The only missing target symbol is the 4B empty weak
  `J3DAnmVtxColor::getColor(unsigned char, unsigned short, _GXColor*) const`.
  `build/binutils/powerpc-eabi-nm -u` shows the rebuilt object does not
  reference it, so it is byte debt, not a source-link blocker.
- Data sections are byte-identical and there are no extra symbols.
- Reviewed the remaining text diffs:
  `J3DDeformData::deform()` is frame-size residue; `J3DSkinDeform::initMtxIndexArray()`
  is a 100.0% fuzzy row with equivalent label/order residue; the large
  `J3DDeformer::deform(..., float*)` loop has the same deformable gate,
  key-start accumulation, weight normalization, position/normal accumulation,
  normal averaging, angle selection, and interpolation behavior; and
  `J3DSkinDeform::deform()` has the same position/normal transform paths,
  buffer swaps, matrix selection, paired-single matrix multiplies, cache
  flushes, and current-buffer updates.
- Promoted `JSystem/J3D/J3DGraphAnimator/J3DCluster.cpp` to
  `Object(Equivalent, ...)`.
- `python configure.py --non-matching && ninja` linked successfully with the
  TU source-linked.
- Follow-up normal `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.

## Remaining Byte Debt

- Missing 4B empty weak `J3DAnmVtxColor::getColor(...)`.
- Stack frame, saved-register/FPR, paired-single temporary, and objdiff
  symbol-label residue in the deformation loops.
