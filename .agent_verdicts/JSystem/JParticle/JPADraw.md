Verdict: equivalent
Date: 2026-06-14 7:29am MNL
TU: mario/JSystem/JParticle/JPADraw

Certification:
- Promoted `JSystem/JParticle/JPADraw.cpp` to `Object(Equivalent, ...)`.
- `python configure.py --non-matching && ninja` linked successfully with
  JPADraw source-linked.
- Follow-up `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.

Strict audit:
- No missing or extra symbols.
- `.data`, `.sdata2`, and `.ctors` entries are byte-identical in the current
  `decomp-diff` overview.
- Remaining text diffs are codegen-class only: stack-frame size and scratch-slot
  offsets in `initialize`, `initParticle`, `initChild`, clipboard setup, draw
  loops, z-draw loops, and `loadYBBMtx`; register coloring around the
  `GXSetAlphaCompare` alpha byte argument; static-init stack/register
  scheduling around `cb` construction and global registration; and one
  `cmplwi r0, 0` vs `cmpwi r0, 0` compare on a byte-loaded texture-animation
  flag in `setParticleClipBoard()`.
- Raw target asm confirms the draw/z-draw clipboard calls are to the intended
  particle/child setup routines; decomp-diff labels some backwards calls with
  the wrong nearby JPADraw symbol name, but the call displacement and source
  behavior are correct.

Implementation update:
- Removed target-absent empty `JPADraw` API bodies/declarations:
  `loadTexture(u8,GXTexMapID)`, `getIndTextureID()`,
  `getIndSubTextureID()`, and `getSecondTextureID()`.
- Fixed a real behavior mismatch in `setChildClipBoard()`: the opening type
  switch now uses `mDrawCtx.mSweepShape->getType()`. Target asm loads
  `mDrawCtx + 0x0c` and then byte `0x44`; the previous source used
  `mBaseShape->getType()`.
- Removed stale TODO comments that pointed at reconstructing the
  target-absent `JPADraw::loadTexture` API.

Current proof:
- `python configure.py && ninja` passed at 2026-06-14 7:21am MNL.
- `python tools/decomp-diff.py -u mario/JSystem/JParticle/JPADraw -s extra`
  reports no symbols.
- `python tools/decomp-diff.py -u mario/JSystem/JParticle/JPADraw -s missing`
  reports no symbols.

Remaining review notes for AUDIT:
- `initialize`, `initParticle`, `initChild`, `setChildClipBoard`,
  `drawParticle`, `drawChild`, `zDrawParticle`, `zDrawChild`, and
  `loadYBBMtx` now show matching operations with frame/stack-slot size,
  register/FPR coloring, local label, or signed-vs-unsigned zero-compare
  residue.
- `setParticleClipBoard()` has one visible `cmplwi r0,0` vs `cmpwi r0,0`
  row on a byte-loaded value; no branch target or data-flow difference found.
- `__sinit_JPADraw_cpp` residue is stack/register scheduling around
  `cb` construction and global registration; no missing constructor or data
  owner found in the implementation pass.
