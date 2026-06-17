# Player/MarioUpper audit

Verdict: `equivalent`

Checked: 2026-06-14 6:33pm MNL in AUDIT safety-net mode.

## Result

Reverified existing `Object(Equivalent, "Player/MarioUpper.cpp")`.

The old source-link blockers are gone:

- `TYoshi::onYoshi()` is no longer emitted as a duplicate owner from this TU.
- `TWaterGun::isEmitting()` owner routing no longer blocks source-linking.

Current nonmatching functions remain behavior-equivalent:

- `TMario::stateMachineUpper()` preserves the pump-state switch, pump-enable
  checks, model-frame zeroing, wait timer decrement/reset, sweat emission,
  `checkPumping()` transitions, and failure state stores. Residue is stack
  frame/slot size and local constant-label drift; the displayed
  `__sinit_MarioUpper_cpp` call label is an objdiff naming artifact for the
  target `checkPumping()` relocation.
- `TMario::checkPumpEnable()` preserves FLUDD possession/action/Yoshi,
  dirty-limit, pump-state, nozzle/water-gun, action-flag, and failure-store
  semantics. Residue is stack-frame/local-slot layout and constant-label drift.

The remaining missing `@2246` row is local constant/data-owner byte debt, not a
source undefined reference or behavior blocker.

## Verification

- `python tools/decomp-diff.py -u mario/Player/MarioUpper -s missing` reports
  only the local constant/data-owner row `@2246`.
- This tick's `python configure.py --non-matching && ninja` linked all current
  `Equivalent` objects successfully.
- This tick's `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.
