# mario/GC2D/Option

Verdict: equivalent  
Status: certified  
Time: 2026-06-14 11:55pm MNL

## Decision

Promoted `GC2D/Option.cpp` to `Object(Equivalent, ...)`.

Required audit fix before promotion:
- `TPaneScalingControl::update()` used the shared `RAD_TO_DEG` macro, whose
  adjusted constant emitted `57.295780f` (`0x42652ee1`). Target uses
  `57.295776f` (`0x42652ee0`). Replaced the expression with
  `progress * TAU * 57.295776f`; `.sdata2` now owns the target literal and
  `TOptionSoundUnit::update()` improved 99.5% -> 99.6%.

Reviewed nonmatching text rows:
- `TOptionControl` setting compare/reset, load/loadSetting, transition, input,
  and write paths perform the same loads, stores, flag writes, pane visibility,
  camera transition checks, and sound-stop behavior.
- `TOptionSoundUnit` state/update/constructor/animation rows match behavior;
  remaining drift is helper-boundary, stack, member-function pointer temporary,
  FPR/register, and local-label debt.
- `TOptionRumbleUnit` state/update/constructor rows match behavior; remaining
  drift is stack/register/local-label and `TPatternAnmControl::set` helper
  boundary debt.

Data notes:
- `.rodata` is byte-identical.
- `.sdata2` still has constant-order drift, but all live literal values now
  match target.
- The common `@1431/@1411/@1210` dummy rows display as missing while source
  owns identical bytes as `dummy1431`/`dummy1411`/`dummy1210`.

Proof:
- `python configure.py --non-matching && ninja` linked with `Option` sourced.
- Restored normal config with `python configure.py && ninja`; passed
  `build/GMSJ01/mario.dol: OK`.

## Re-verification — 2026-06-15 12:23pm MNL

Rechecked the only stale `Equivalent` audit cache after
`System/MSoundMainSide` was certified. Current overview still matches the
original verdict: no behavioral blockers, only codegen/source-only helper
factoring and data-label debt. Current proofs:

- `python configure.py --non-matching && ninja` linked with `Option` sourced.
- `python configure.py && ninja` passed `build/GMSJ01/mario.dol: OK`.
