verdict: equivalent
date: 2026-06-15 9:26pm MNL
unit: mario/Enemy/hauntLeg

Certified after the fresh implementation handoff cleared all missing rows and
restored the TEV color tables / slope-matrix behavior.

Reviewed non-exact rows:
- `TNerveHauntLegHaunt::execute`: same jump setup, airborne/landing split,
  target-distance check, `receiveMessage(self, 4)` take path, wander push, and
  angle clamp. Residue is stack frame, vector helper-boundary, register/FPR, and
  clamp/source-shape drift.
- `THauntLeg::getTakingMtx`: same clipped-out identity/translation copy into
  base TR matrix and same normal `getAnmMtx(0) + 0x60` return. Residue is
  `identity33` vs `PSMTXIdentity` label/call-boundary and FPR placement.
- `THauntLeg::calcRootMatrix`: same eaten guard, current-leg global store,
  scale slot writes, slope and fallback basis construction, tilt concat,
  translation writes, haunted-proxy position update, and empty collision loop.
  Residue is frame/FPR/register allocation, `stw` vector copy vs `stfs` field
  stores for equivalent float values, matrix temp layout, and empty-loop
  lowering.
- `THauntLeg::init`: same actor flags, walker mode, joint callback, haunted
  object allocation/vtable, group insertion, hit actor init, and owner store.
  Residue is register allocation, local string labels, and list-helper call
  label drift.
- `THauntLegManager::initSetEnemies`: same graph lookup, random graph-node
  placement, y + 5.0, airborne/reset, material loop, two-TEV-color setup, and
  eight-entry color cycling. Residue is frame/data-label/register drift.
- `HauntLegCallback`: same `when == 0` / current-leg / haunt-nerve guards,
  short-angle conversion, `JMASSin`/`JMASCos`, Z-rotation matrix construction,
  and two `PSMTXConcat` calls. Residue is singleton/static label drift and
  register/local-matrix layout.

Proof:
- `python configure.py --non-matching && ninja` linked with
  `Enemy/hauntLeg.cpp` sourced.
- Plain `python configure.py && ninja` passed `build/GMSJ01/mario.dol: OK`.
