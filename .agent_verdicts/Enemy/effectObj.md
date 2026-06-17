# Enemy/effectObj audit

Verdict: equivalent
Date: 2026-06-13 3:04pm MNL

`Enemy/effectObj.cpp` links from source and is behavior-equivalent after the
fresh implementation fixes.

Behavior fixes landed before certification:
- `TEffectBombColumWater::generate()` now copies incoming scale directly into
  `mScaling` and emits particle `0x1D4` with group argument `2`; this row is
  now byte-exact.
- `TEffectColumSand::generate()` and `TEffectExplosion::generate()` now use
  `param_2.x` for the uniform particle emitter scale, matching target loads
  from offset `0`.
- `TEffectModel::reset()` now materializes the random rotation multiply before
  adding `mn`, matching the target's separate `fmuls`/`fadds` behavior instead
  of a contracted `fmadds`.

Remaining nonmatching rows are codegen/data-owner debt only:
- Reset-family rows differ by stack/frame size, saved-register placement,
  local rodata labels, and model-name base register coloring, but call the same
  `reset`, animation setters, frame controllers, and flag stores.
- `TEffectColumWater::generate()` differs by target out-of-line
  `TVec3::scale(1.3f)` versus source-inlined multiplies; the stored scaled
  vector and `0x89`/`0x8A` particle emissions are identical.
- `TEffectObjManager::perform()`, `TEffectObjBase::perform()`,
  `moveObject()`, `behaveToWater()`, and the two fountain `emitEffect()` rows
  differ only by iterator/frame/register/temp layout with the same calls,
  constants, stores, and branch conditions.
- `.data`/extra rows are helper/vtable/static-owner drift; no target symbols
  are missing.

Proof:
- `python configure.py && ninja` passed before promotion.
- `python configure.py --non-matching && ninja` linked `effectObj` from source.
- `python configure.py && ninja` restored the normal config and verified
  `build/GMSJ01/mario.dol: OK`.
