# JSystem/JAudio/JAInterface/JAISound Audit

Verdict: equivalent
Checked: 2026-06-14 7:42pm MNL
Unit: `mario/JSystem/JAudio/JAInterface/JAISound`

Safety-net recheck after `MoveBG/MapObjWave` certification. Current overview
has no missing target symbols, exact `.data`/`.sdata2`, and only the same four
nonmatching text rows: `setDistancePanCommon`, `setPositionDopplarCommon`,
`setSeDistancePitch`, and `setSeDistanceFxmix`. Existing reviews below still
classify these as behavior-equivalent distance/audio-parameter codegen drift,
and the source extra rows remain empty virtual/helper owner drift. This tick's
successful source-link and normal proof builds cover the current object.

## 2026-06-12 9:44pm MNL - equivalent

Verdict: `equivalent`.

Promoted `JSystem/JAudio/JAInterface/JAISound.cpp` from `NonMatching` to
`Equivalent`.

Reason:
- No missing target symbols.
- Reviewed all four nonmatching text functions:
  `JAISound::setDistancePanCommon()`,
  `JAISound::setPositionDopplarCommon(unsigned long)`,
  `JAISound::setSeDistancePitch(unsigned char)`, and
  `JAISound::setSeDistanceFxmix(unsigned char)`.
- The pan function preserves the one-camera pan formula, clamp branches, and
  multi-camera fallback; residue is return-FPR and branch-target codegen.
- The dopplar function preserves camera/source deltas, two `sqrtf` lengths, the
  dopplar denominator, and 0.1/2.0 clamp; residue is scheduling/FPR naming and
  constant-label ownership.
- The pitch and fxmix distance functions preserve sw-bit gates, random pitch
  perturbation, distance scaling, clamps, and inlined SE move-parameter writes.
  Raw asm confirmed the apparent `setSeDistanceFxmix` call-label mismatch in
  `decomp-diff.py` is symbol-owner drift; target calls
  `setSeInterFxmix__8JAISoundFUcfUlUc`.
- Extra empty 4-byte virtual/helper stubs and two standalone helper bodies are
  source-side owner drift, not missing runtime behavior.

Proof:
- `python configure.py --non-matching && ninja` linked with `JAISound` from
  source.
- `python configure.py && ninja` passed and verified `mario.dol: OK`.

## 2026-06-13 9:24am MNL - reverified

Verdict remains `equivalent`.

Full diffs for `setDistancePanCommon`, `setPositionDopplarCommon`,
`setSeDistancePitch`, and `setSeDistanceFxmix` still show identical behavior:
same pan branches and formulas, same doppler distance math and clamps, same
random pitch/distance scaling, and same fxmix clamp/interpolation. Raw
relocations confirm the noisy `setSeDistanceFxmix` labels are inline/owner
drift: target inlines `checkSwBit(4)` through `getSoundSwBit`, while source
calls the matching wrapper, and both final calls target `setSeInterFxmix`.
Shared proof passed: `python configure.py --non-matching && ninja`, then
`python configure.py && ninja` verified `mario.dol: OK`.

## 2026-06-13 6:55pm MNL - reverified

Verdict remains `equivalent`.

Secondary safety-net pass after `M3DUtil/MActorData` certification. Current
overview still has no missing target symbols; only the same four nonmatching
text functions remain:

- `setDistancePanCommon()`: same one-camera pan formula, small-vector 0.5
  fallback, max-distance clamps, side-specific pan branches, and multi-camera
  fallback. Residue is FPR/result register placement and branch layout.
- `setPositionDopplarCommon(unsigned long)`: same camera/source delta math,
  two length calculations, doppler denominator, and 0.1/2.0 clamp. Residue is
  scheduling/FPR naming and constant-label ownership.
- `setSeDistancePitch(unsigned char)`: same sw-bit gates, random perturbation,
  distance pitch scaling, player-count pitch addition, and move-parameter
  writes. Residue is stack-slot/FPR operand order.
- `setSeDistanceFxmix(unsigned char)`: same default/distance fxmix calculation,
  0x7f clamp, normalization, and SE interpolation behavior. Displayed callee
  names remain misleading owner-label drift from the existing note, not a
  behavior difference.

Proof refreshed on current tree:
- `python configure.py --non-matching && ninja`
- `python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`
