# mario/NPC/NpcBase

Verdict: equivalent
Status: equivalent
Time: 2026-06-15 3:58am MNL

## Verdict

## Reason
Do not promote yet. Tick 798 implemented the concrete
`TSpineEnemy::isReachedToGoal() const` blocker: the source now owns the body in
`NpcBase.cpp`, routes non-owner TUs through an out-of-line declaration, computes
distance to the active/fallback graph point, and `@3386` now matches.

The rebuilt object still has missing target ctor/data rows (`@3935`, `@3936`).
The next implementation pass should review the remaining low-score `perform`,
`moveObject`, and `receiveMessage` diffs for real structure before promotion.

Tick 798 also fixed one real `moveObject` bug: the balloon update gate is
outside director states 1-4, and held-message routing swaps `0xE004F` and
`0xE0051` around skipped `0xE0050`.

Tick 798 fixed a real `receiveMessage` bug as well: default/unhandled paths now
return `FALSE` like the target instead of `TRUE`.

Tick 798 fixed an early `perform` gate too: the target tests
`LIVE_FLAG_UNK200` (`0x200`) where source had `0x400`. The TU still needs
implementation review because `perform` is missing a later sight-test block and
other branch structure.

Tick 798 restored the camera `MsIsInSight` gate in `perform`, moving that
function 61.7% -> 67.1% and matching `@3705`-`@3707`. `perform` still has later
branch-shape differences.

Tick 798 also fixed two `perform` live-flag masks: skipped update clears
`0x60000`, and the `unk1DC` countdown reset clears `0x10000000`.

Tick 798 fixed the late model lock/unlock path in `perform`: it is gated by
`flags & 0x200`, clears `0x1000000`, and uses `mAllDLLockDist`.

Tick 798 restored the main `flags & 2` animation/matrix-effect branch in
`perform`, moving it 67.2% -> 82.0%. Remaining `perform` residue is smaller
branch/register/source-shape debt.

Tick 798 corrected `receiveMessage` so non-waterball `message == 0x0E` senders
return `FALSE`.

Tick 798 cleaned up the `receiveMessage` particle scale locals, moving it
62.0% -> 73.1%.

Tick 798 fixed another concrete `moveObject` cluster: the ride/sink gates use
`0x400000` and `0x1000000`, bind/walk-clamp uses `0x10`, held yaw comes from
`mHolder->mRotation.y * 182.04445f`, and upward walk velocity clamps to `5.0f`.
This moved `moveObject` 82.7% -> 89.1% and resolved the missing `@3935` /
`@3936` data rows.

Tick 798 then reshaped the `TNpcBalloon` director-mode guard to the recurring
NPC two-stage local-bool pattern and made the held message ladder skip exactly
`0xE0050`; `moveObject` moved 89.1% -> 91.2%.

Tick 798 fixed `getAnmOffDist_()`'s default value: target returns
`gpCamera->mFar` unless a dont-calc-anim nerve selects a per-NPC wait distance,
using `mSLDanceAnmOffDist` only as the active-off max candidate. The helper
moved 81.3% -> 84.7%.

Tick 798 rewrote `isNeedNeckStraight()` to the target's single-result structure
without changing the predicate table; it moved 64.0% -> 78.9%.

Tick 798 rewrote `receiveMessage()` to a default-false shared-result path,
moving it 73.1% -> 76.4%. Behavior remains aligned; remaining differences are
source-shape/codegen residue.

`TBaseNPC::getFocalPoint()` and `getCursorPos()` are also 0% because the source
uses return-by-value temporary/copy shape instead of direct sret stores, but the
remaining broader structural diffs are enough to keep this `NonMatching`.

## Implementation follow-up — 2026-06-15 3:47am MNL

Ready for AUDIT recheck by implementation judgment. This pass found and fixed
two remaining concrete behavior gaps:

- `TBaseNPC::perform()` now restores the target state-4 turn-animation rate
  clamp after `walkAnmRateChange_()`: `SMSGetAnmFrameRate() * mTurnSpeed *
  mTurnAnmRate`, clamped to `mTurnAnmMinRate`/`mTurnAnmMaxRate`, then
  `MActor::setFrameRate(rate, 0)`. Score moved 82.0% -> 84.4%.
- `TBaseNPC` constructor now skips post-init allocation only for actor
  `0x0400001C`, still allocates `TNpcAnmRequest` for lock actors
  `0x0400000F/0x04000014`, and includes normal Mare M/W in trample
  eligibility. Score moved 53.0% -> 63.3%.

Verification: `python configure.py && ninja` passed, `git diff --check` passed,
and `python tools/decomp-diff.py -u mario/NPC/NpcBase -s missing` reports no
missing symbols. Remaining known residue is codegen/source-shape: constructor
vector default-ctor calls, source-only `TNpcTrample::TNpcTrample()`, stack
frame size, helper-owner labels, and sret vector return shape.

## Certification — 2026-06-15 3:58am MNL

Certified `Equivalent`. Rechecked every nonmatching function after the
implementation follow-up. No missing target symbols remain, and the remaining
diffs are behavior-neutral: stack/register frame shape, compare lowering,
branch-layout differences, helper-owner/inline choices, source-only weak helper
emissions, sret `Vec` return shape for `getFocalPoint()`/`getCursorPos()`, and
jump-table/data relocation label drift.

Behavioral review highlights:
- `perform()` preserves the special dummy-connected actors, camera sight gate,
  state-4 turn-animation frame-rate clamp, live-flag masks, animation/matrix
  effect branch, and model lock/unlock behavior.
- `moveObject()` preserves trample/mad transition checks, balloon director-mode
  blocking and held-message routing, forbid counters, sand-bomb behavior,
  ride/sink flags, held transform/yaw update, upward velocity clamp, and
  final position/rotation integration.
- `receiveMessage()` preserves the default-false result, take/trample/object
  messages, waterball/kino sender gate, particle emission/cooldown, and hit
  object kind selection.
- Constructor semantics match target gates: actor `0x0400001C` exits after
  base field init, lock actors still allocate `TNpcAnmRequest`, and trample
  allocation covers normal/special Monte/Mare plus the `0x04000016..17` range.

Proof:
- `python configure.py --non-matching && ninja` linked with `NpcBase` sourced.
- `python configure.py && ninja` restored matching config and passed
  `build/GMSJ01/mario.dol: OK`.
