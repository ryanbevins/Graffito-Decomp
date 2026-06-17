# MoveBG/MapObjTrap

Verdict: equivalent
Date: 2026-06-12 2:40pm MNL

Reason: all target text functions are present. The five nonmatching functions
preserve behavior:

- `TLampTrapIron::receiveMessage`: same actor-type predicate, hit-point
  decrement, fire timer store, particle bind, and base fallback; residue is
  frame size/slot placement and label ownership for `mFireTimerMax`.
- `TLampTrapIron::loadAfter` and `TLampTrapSpike::loadAfter`: same allocation,
  hit-actor construction, item-group insertion, flag clearing, and pointer
  store; residue is JGadget/helper owner labels and temporary stack slots.
- `TLampTrapSpike::control`: same switch dispatch, BCK/frame-rate setup,
  timer/state transitions, sound call, base control call, ground-plane guard,
  and Mario attack message; residue is saved-register coloring, stack slots,
  and constant label names.
- `TLampTrapSpikeHit::perform`: same position copy, Y offset, state guard,
  collision iteration, actor-type test, and Mario attack message; residue is
  frame size/slot layout and constant label names.

Data note: objdiff's `.data` / `.sdata` residue is label/ownership drift from
source-emitted standalone hit constructors, JGadget helpers, and infectious
strings. No runtime table or static field needed by this TU is missing.

Proof: `python configure.py --non-matching && ninja` linked from source, then
plain `python configure.py && ninja` passed and verified `mario.dol: OK`.

2026-06-13 8:15am MNL recheck:
- Overview still has no missing target text functions. Nonmatching text remains
  limited to `TLampTrapIron::receiveMessage`, both `loadAfter` methods,
  `TLampTrapSpike::control`, and `TLampTrapSpikeHit::perform`.
- `TLampTrapIron::receiveMessage`: current full diff preserves the same
  actor-type predicate, hit-point decrement, zero-hit fire-timer store,
  particle bind to the model matrix, true return, and base fallback. Residue is
  stack frame/slot size and static-label ownership for `mFireTimerMax`.
- `TLampTrapIron::loadAfter` and `TLampTrapSpike::loadAfter`: current full
  diffs preserve allocation, hit-actor construction, actor type/dimensions,
  owner pointer store, item-group insertion, hit-flag clearing, and owner
  pointer assignment. Displayed helper names differ at JGadget iterator/list
  calls, but the sequence and call boundaries are the same helper-owner/label
  residue already known for this TU.
- `TLampTrapSpike::control`: current full diff preserves the state switch,
  BCK selections, frame/rate setup, timer/state transitions, sound gate/start,
  frame-pass/end checks, base control call, ground-plane guard, and Mario attack
  message. Residue is saved-register coloring, stack slots, and local constant
  labels.
- `TLampTrapSpikeHit::perform`: current full diff preserves the position copy,
  Y offset, state guard, collision iteration, actor-type test, and Mario attack
  message. Residue is stack frame/slot size and local labels.
- Proof refreshed in the same audit sweep: `python configure.py --non-matching
  && ninja` linked from source, and normal `python configure.py && ninja`
  verified `build/GMSJ01/mario.dol: OK`.

2026-06-13 11:44am MNL recheck: verdict remains `equivalent`. Current overview
is unchanged: no missing target text rows, same five nonmatching functions, and
the same source-owned hit-constructor/JGadget/infectious-string owner drift.
The existing full-diff review remains valid: iron/spike hit handling,
loadAfter allocation and group insertion, spike control state transitions, and
hit actor perform collision behavior are aligned. Shared proof from this tick
passed: `python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` verified `build/GMSJ01/mario.dol: OK`.
