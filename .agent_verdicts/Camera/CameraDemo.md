verdict: equivalent
date: 2026-06-13 5:44pm MNL
unit: mario/Camera/CameraDemo

Certified `Object(Equivalent, "Camera/CameraDemo.cpp")`.

The previous blocker was the duplicate `TYoshi::onYoshi()` owner. Current
focused diff shows no `TYoshi::onYoshi()` extra and no missing `.text` rows.
The 2026-06-13 2:29am MNL behavior review still applies: camera demo routines
perform the same demo update calls, optional offset copy, at/eye/up rotation
math, final matrix updates, gate-demo map-tool lookup, and frame
countdown/reset behavior. Remaining drift is codegen/static-owner debt,
including frame/register layout, local constants, helper ownership, and the
extra static-init/helper owners that do not block source linking.

Proof:
- `python configure.py --non-matching && ninja` passed with
  `CameraDemo.cpp` source-linked.
- `python configure.py && ninja` then passed with `build/GMSJ01/mario.dol: OK`.

verdict: needs_impl
date: 2026-06-13 2:29am MNL
unit: mario/Camera/CameraDemo

Reason:
- No missing target symbols in the objdiff overview, and the reviewed camera
  demo routines are behaviorally aligned: same demo update calls, optional
  offset copy, at/eye/up rotation math, final matrix updates, gate-demo map-tool
  lookup, and frame countdown/reset behavior.
- Temporary `Equivalent` promotion failed the required
  `python configure.py --non-matching && ninja` proof because
  `CameraDemo.o` emits a duplicate `TYoshi::onYoshi()`, already defined by
  `MarioAction.o`.

Blocker:
- Fix weak/inline ownership for `TYoshi::onYoshi()` before promoting.
