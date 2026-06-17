# Player/MarioEffect Audit

Verdict: `equivalent`

Last rechecked: 2026-06-15 7:52am MNL.

Safety-net recheck: current overview still has no missing rows. The same four
non-exact functions remain behavior-reviewed; residual differences are
stack/register/FPR shape, infectious string/data owner drift, and helper-owner
extras. Today's full `python configure.py --non-matching && ninja` proof linked
this object from source, and normal `python configure.py && ninja` passed
`build/GMSJ01/mario.dol: OK`.

## 2026-06-13 6:36pm MNL - reverified

Verdict remains `equivalent`.

Current overview still has no missing symbols. The nonmatching rows are the same
four behavior-reviewed functions plus rodata/data/sdata2 and helper-owner byte
debt. The prior review remains valid: `perform()` preserves the waterboost
state machine and per-flag actor processing, the splash helpers preserve slot
selection/matrix/animation/visibility/active-slot writes, and `init()` preserves
actor/model/animation setup and conductor registration. The full
`--non-matching` proof earlier in this tick linked this object from source.

## 2026-06-13 9:29am MNL - refreshed

Verdict remains `equivalent`.

Re-read full `--no-collapse` diffs for `perform`,
`setJumpIntoWaterEffectSmall`, `setJumpIntoWaterEffect`, and `init`. `perform`
is byte-equivalent modulo frame/rodata ownership. The two splash helpers still
preserve the same gates, slot selection, matrix/scale setup, animation/rate
writes, shape-packet visibility writes, and active-slot writes. `init` preserves
the two jump actors, waterboost actor/model setup, animation setup, frame-rate
writes, and conductor registration. Residue is stack/register coloring and
infectious-string label ownership.

## 2026-06-12 9:48pm MNL - equivalent

Verdict: `equivalent`.

Promoted `Player/MarioEffect.cpp` from `NonMatching` to `Equivalent`.

Reason:
- No missing target symbols.
- Reviewed all four nonmatching text functions:
  `TMarioEffect::perform(unsigned long, JDrama::TGraphics*)`,
  `TMarioEffect::setJumpIntoWaterEffectSmall()`,
  `TMarioEffect::setJumpIntoWaterEffect()`, and
  `TMarioEffect::init(TMario*)`.
- `perform` preserves the water-boost state machine, BCK/BTK transitions,
  particle matrix binding, model-matrix copy, per-flag actor perform calls, and
  two-slot water-entry animation retirement.
- The two water-entry helpers preserve actor-slot selection, Mario matrix copy,
  small/full scale setup, animation/rate setup, matrix copy, shape packet
  hide/show flags, and active-slot writes.
- `init` preserves Mario pointer/slot init, jump/boost animation data and actor
  allocation, model resource loads with `0x10040000`, model setup, boost BCK/BTK
  setup, frame-rate writes, and conductor registration.
- Remaining residue is codegen-class only: stack frame/slot layout, saved GPR/FPR
  coloring, pointer-local shape, infectious-string label offsets, and weak/
  include-owner extras.

Proof:
- `python configure.py --non-matching && ninja` linked with `MarioEffect` from
  source.
- `python configure.py && ninja` passed and verified `mario.dol: OK`.
