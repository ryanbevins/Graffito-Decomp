Verdict: equivalent
Date: 2026-06-14 3:16am MNL
TU: mario/Player/MarioSound

Reason:
- Re-audited after the implementation pass fixed the Yoshi jump-charge release
  velocity field in `TMario::soundMovement()`.
- Fixed one remaining source-shape issue before promotion: `MARIO_START_VOICE`
  now routes through `TMario::startVoice()`, matching the target's inlined
  helper shape and redundant Yoshi guard at voice sites. This lifted
  `soundMovement()` from 71.5% to 84.6%.
- Remaining nonmatching text diffs are behavioral-equivalent codegen debt:
  stack frame size, saved-register placement, bool materialization/retests,
  compare-tree shape, source order/scheduling, and helper call-boundary drift
  such as `soundTorocco()` calling `JGeometry::TUtil<float>::sqrt` in target
  versus the same length math inlined in source.
- No missing symbols remain. Extra `JSUList` destructor/static symbols are
  weak/header ownership debt; the rebuilt object has no unexpected unresolved
  references.

Proof:
- `python configure.py --non-matching && ninja` linked with `MarioSound`
  source-linked.
- Final normal `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.
