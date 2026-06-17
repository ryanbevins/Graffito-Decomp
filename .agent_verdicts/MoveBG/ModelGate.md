verdict: equivalent
date: 2026-06-13 10:33pm MNL
unit: MoveBG/ModelGate
commit: 57ecb8d7

Certified `MoveBG/ModelGate.cpp` as behaviorally Equivalent.

Proof:

- `Object(Equivalent, "MoveBG/ModelGate.cpp")` passed
  `python configure.py --non-matching && ninja`, linking `ModelGate.o` from
  source.
- Normal `python configure.py && ninja` then passed with
  `build/GMSJ01/mario.dol: OK`.

Review:

- `receiveMessage()` is behavior-identical. The diff keeps the same sender
  actor-type/message gates, local-position transform, open-progress update,
  random particle emission, and return values; residue is stack-slot and
  constant-label numbering.
- `screenBlur()` is behavior-identical. The target and source perform the same
  Mario-relative vector normalize/rotate, local gate bounds and alpha ramp,
  camera-angle gate, lerp, blur-factor conversion, and `TAfterEffect` stores;
  residue is stack/FPR/local-copy scheduling.
- `perform()` is behavior-identical. Raw target asm matches the source order:
  THP texture offset updates, `MActor::perform`, gate-distance/opening logic,
  local Mario bounds, jump/receive-message handoff, push-away request, open-state
  switch, four wind particles, two sound gates, TEV stage updates, frame 5 set,
  optional `screenBlur()`, and final `THitActor::perform`. Residue is stack
  frame/register coloring, `sqrt` inline/helper choice, TEV helper ownership,
  and objdiff label alignment around extra local helper bodies.
- `loadAfter()` is behavior-identical after the implementation fix. The target
  and source call `initHitActor(0x080000C0, 5, 0x80000000, 300, 400, 300, 400)`,
  choose the gate index by name, create the model actor, patch THP textures,
  initialize animation/model-control fields, insert into the map group list,
  compute matrices/hold offset/angle, load the same particles, and set open
  flags from the same `TFlagManager` IDs. Residue is frame/register layout,
  `snprintf` argument register shape, THP half-size store scheduling, iterator
  temporary layout, and data-label shape.
- Data drift is byte debt: the `.data` prefix bytes match; target
  `@1431/@1411/@1210` are represented by source `dummy1431/dummy1411/dummy1210`;
  `.sdata2` contains the same constants in different order; extra weak/helper
  bodies are owner/codegen debt and the source-link proof shows no undefined
  behavior-bearing symbol is missing.
