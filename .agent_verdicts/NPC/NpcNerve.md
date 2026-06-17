# mario/NPC/NpcNerve

Verdict: equivalent
Date: 2026-06-13 6:19am MNL

Reason:
- `TNerveNPCTalk::execute` keeps the same `gpMarDirector->isTalkModeNow()` /
  `unk124 == 4` predicate, calls, and returns; residue is register/frame
  coloring.
- `TNerveNPCRecoverAfter::execute` differs by stack-frame size only.
- `TNerveNPCWaitMarioApproach::execute` keeps the same transition to
  `TNerveNPCTurnToMario`, actor-type guard behavior, sunflower block, Monte ME
  path, turn-to-first-state path, and time-zero wait call. Residue is inlined
  `execCommonWaitApproach` codegen drift.
- `TNerveNPCTurnToMario::execute` keeps the same early transition back to
  wait, common wait/approach behavior, `SMS_GoRotate`, Mario delta vector, yaw
  quadrant handling, delta normalization, and wait/step calls. Residue is stack
  temporaries, register allocation, and branchy absolute-value lowering.
- `TNerveNPCGraphWander::execute` keeps the same initial random frame counter,
  walk call, goal-distance computation, pending-path pop, one-way graph-node
  detection, frame-counter update, 50/100 unit thresholds, graph-wait push,
  next-node advance, U-turn push, and `0x200000` live-flag set. Residue is
  stack/register/branch-shape drift plus local helper-label ownership.
- Source-link proof passed under `python configure.py --non-matching && ninja`,
  then normal `python configure.py && ninja` restored the matching config and
  verified `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

2026-06-13 10:45am MNL recheck: verdict remains `equivalent`. Re-read the
current diffs for all five nonmatching nerves. `Talk` and `RecoverAfter` keep
the same director/talk and recovery predicates/calls. `WaitMarioApproach` and
`TurnToMario` still share the same sink/down/sunflower/Monte guards, turn-to
first-state paths, Mario rotation/vector math, wait/step calls, and return
values; residue is frame/register/branch-shape and abs-angle lowering. For
`GraphWander`, raw target asm confirms the scary pretty labels are actually
`getCurGraphIndex__12TGraphTracerCFv` and `getGraph__12TGraphTracerCFv`, not
wrong destructor/nerve calls. The function still preserves random frame setup,
walk, goal-distance math, pending-path pop, one-way graph-node detection,
frame-counter update, 50/100 distance thresholds, graph-wait push,
shortest-next-node advance, U-turn push, and `0x200000` live-flag store. Proof
refreshed with `python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
