# Animal/boid

Verdict: equivalent
Date: 2026-06-14 7:34pm MNL

Promoted `Animal/boid.cpp` to `Object(Equivalent, ...)`.

Proof:

- `python tools/decomp-diff.py -u mario/Animal/boid -s missing` reports no rows.
- `python configure.py --non-matching && ninja` linked the DOL with
  `Animal/boid.o` sourced.
- `python configure.py && ninja` restored the normal matching configuration and
  passed with `build/GMSJ01/mario.dol: OK`.

Strict review:

- `JGeometry::TVec3<float>::div(float)` is present, 48B, and byte-identical.
- `TBoidLeader::setGraph` preserves the null/dummy-graph gate, tracer
  allocation, graph setup, nearest-node set, graph-goal copy, and flag write;
  residue is branch layout and register shuffling.
- `TBoidLeader::perform` preserves the perform-mask gate, graph-goal chase,
  random next-node move, normalized drift, and `calcBoids()` call. The apparent
  pretty-diff label noise at the tail is a real relocation to
  `calcBoids__11TBoidLeaderFv`.
- `calcGoalForce`, `calcForces`, and `calcBoids` perform the same boid force,
  goal, graph, neighbor, yaw/pitch, repel, matrix, and velocity operations.
  Remaining differences are helper-boundary choices, stack/register/FPR
  allocation, branch layout, redundant normalize checks, and `.sdata2` label
  ordering.
- Data drift is behavior-neutral: `TBoidLeader::__vtable` and named constants
  are exact; source also emits unused `JDrama::TViewObj` weak
  vtable/destructor rows and a different local-constant order.
