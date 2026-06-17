# Camera/CameraJetCoaster audit

Verdict: `equivalent`

Checked 2026-06-14 11:02am MNL in AUDIT mode.

Promoted `Camera/CameraJetCoaster.cpp` to `Object(Equivalent, ...)` after a
full current review of the only nonmatching function,
`CPolarSubCamera::ctrlJetCoasterCamera_()`.

Behavior reviewed as matching:

- Episode balloon/message state machine: flag count, actor count, countdown,
  stage transition, message IDs, and `drawJetCoasterBalloonMessage_()` inlined
  path all perform the same calls and stores.
- L-button first-person toggle: same input flag, mode flip, sound IDs, gate
  check, and SE call.
- First-person camera path: nozzle/top-position update, X-rotation update,
  torocco matrix column extraction, `CLBRotatePosAndUp` output order,
  normalized forward vector, chain/up offset, rotated side offset, angle
  conversion through `matan`/`CLBRoundf<s16>`, and distance field store match
  behavior.
- Third-person camera path: manual stick angle updates, limit clamps,
  `CLBChaseAngleDecrease` calls, coaster-state word copies, Mario-facing
  forward vector normalization, side-vector cross product, and
  `CLBRotatePosAndUp` call match behavior.
- Shared tail snap/chase path: word copies into camera position, first-person
  snap behavior, third-person chase speeds, and `CLBChaseDecrease` calls match.

Remaining diffs are byte/codegen debt:

- Frame size and saved-register/FPR allocation differ.
- `TRotation3::setRotate`, `TVec3::dot`, and `TVec3::scale` helper ownership
  appears as source-local extras, but call boundaries/operations are present and
  source-link safe.
- `.rodata`/`.sdata2` label ownership and fused multiply/add spelling differ.

Proof:

- `python configure.py --non-matching && ninja` linked successfully with
  `CameraJetCoaster.o` sourced.
- `python configure.py && ninja` restored the normal matching config and passed
  `build/GMSJ01/mario.dol: OK`.
