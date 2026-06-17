Verdict: equivalent
Time: 2026-06-13 6:32am MNL
Unit: mario/Map/Sky
Source: src/Map/Sky.cpp

Reason:
- Re-reviewed the three nonmatching functions. `TSky::TSky(const char*)` and
  `TSky::load(JSUMemoryInputStream&)` preserve the base construction/load,
  MActor creation, optional material-table copy/initDL, and non-Sirena
  `startAllAnim("sky")`; residue is stack size and local symbol/relocation
  label drift.
- `TSky::perform(unsigned long, JDrama::TGraphics*)` preserves the same
  camera inverse/identity setup, translation extraction, Sirena-only Y rotation
  matrix and angle wrap, model base matrix copy, `MActor::perform`, and the GX
  sky-sphere draw state/calls. Residue is stack/register layout, matrix
  temporary placement, arithmetic scheduling, and anonymous constant labels.
- No missing symbols. Named vtables match; the remaining anonymous `.data`
  mismatch and weak/base extras are owner/layout drift.
- Proof: `python configure.py --non-matching && ninja` linked from source, then
  plain `python configure.py && ninja` passed and verified `mario.dol: OK`.
- 2026-06-13 6:32am MNL recheck: overview still has no missing target rows,
  and `python configure.py --non-matching && ninja` linked from source.

Reverified: 2026-06-13 10:52am MNL — still equivalent. Re-read constructor,
`load`, and `perform` diffs. Base construction/load, MActor setup, optional
material table path, non-Sirena animation start, camera inverse/identity setup,
Sirena rotation/angle wrap, base-matrix copy, `MActor::perform`, and GX sphere
draw calls remain behaviorally identical. Remaining drift is stack/register
shape, matrix temporary placement, arithmetic scheduling, and owner/constant
labels. Proof passed again with `python configure.py --non-matching && ninja`,
then plain `python configure.py && ninja` restored the matching config and
verified `build/GMSJ01/mario.dol: OK`.
