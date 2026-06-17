# MoveBG/MapObjWave audit

Verdict: equivalent
Checked: 2026-06-14 7:39pm MNL
Unit: `mario/MoveBG/MapObjWave`

Promoted `MoveBG/MapObjWave.cpp` to `Object(Equivalent, ...)`.

Proof:

- `python tools/decomp-diff.py -u mario/MoveBG/MapObjWave` reports no missing
  target rows.
- `python configure.py --non-matching && ninja` linked the DOL with
  `MapObjWave.o` sourced.
- `python configure.py && ninja` restored the normal matching configuration and
  passed with `build/GMSJ01/mario.dol: OK`.

Fix before certification:

- `TMapObjWave::draw()` now names the texture-coordinate products before adding
  `unk6C`/`unk70`. This removes the behavior-bearing `fmadds` contraction for
  texture coordinates and emits target-equivalent separate multiply/add
  rounding. Fuzzy score drops because FPR/frame allocation changes, but the
  previous audit blocker is gone.

Strict review:

- `load`, `perform`, `updateHeightAndAlpha`, `draw`, and `getHeight` preserve
  the same constants, area switch cases, water/background predicates, cube
  height adjustment, wave equations, texture-coordinate values, and return
  cases.
- `initDraw`, ctor/dtor, `updateTime`, `noWave`, and `getWaveHeight` are exact.
- Remaining drift is codegen/data-owner debt: frame/register/FPR allocation,
  branch layout around `isWaterBg`, local helper/static ownership,
  source-owned weak/list destructor rows, and static-init/data relocation drift.
  The target's startup `sColor` default is overwritten by the constructor before
  `draw()` can use the TU-global color.

Verdict: not_equivalent
Checked: 2026-06-13 1:33am MNL
Unit: `mario/MoveBG/MapObjWave`

## Result

Do not promote yet. During audit, `TMapObjWave::initDraw` had real GX
state mismatches against the target; those constants were corrected and the
function now byte-matches. The TU still has remaining differences that are not
safe to certify as pure codegen.

Build proof after the `initDraw` fix:

- `python configure.py && ninja` passed.
- `python tools/decomp-diff.py -u mario/MoveBG/MapObjWave -d 'TMapObjWave::initDraw'`
  reports `100.0% match`.

## Blockers

- `TMapObjWave::draw` still differs around texture-coordinate emission. Our
  source emits `fmadds` for `base + coord * scale`, while the target emits
  separate `fmuls` + `fadds`; this may affect exact FP rounding, so it is not
  audit-safe. A temporary-product rewrite was tested and reverted because it
  worsened FPR pressure and stack size.
- `__sinit_MapObjWave_cpp` still has static-list/data relocation drift. A
  `sColor = { 0xff, 0xff, 0xff, 0xff }` initializer was tested and reverted
  because it introduced an extra data symbol and made the constructor
  nonmatching while not fixing the target `__sinit` store.
- `.data` and `.sdata2` remain nonmatching, with many extra weak list
  destructors and the local `isWaterBg(unsigned short)` helper emitted as an
  extra symbol.
