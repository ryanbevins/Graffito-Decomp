# mario/Strategic/liveactor

Verdict: equivalent
Date: 2026-06-14 3:47am MNL

Reason: source-link validation passed after re-auditing the fresh
`TLiveActor::control()` implementation. One real semantic mismatch was fixed
before promotion: `TSpineBase<TLiveActor>::isIdle()` now treats nonpositive stack
size as idle (`size() <= 0`), matching target `ble` behavior and the existing
`TSolidStack::pop()` empty-stack convention.

Audit details:
- `TLiveActor::control()` now performs the target `unk90` auxiliary-controller
  dispatch when `unk90+4 != 0` and either no spine exists or the spine is idle.
  Remaining drift is register choice, vtable/table load order, and bool
  materialization around the same `current == nullptr && size <= 0` condition.
- `initAnmSound()` and the inlined copy inside `init()` perform the same
  NPC-vs-normal sound allocation, constructor calls, random byte initialization,
  assignment to `mAnmSound`, and `initAnmSound(nullptr, 1, 0.0f)` call; residual
  drift is stack frame/slot layout and local constant labels.
- `requestShadow()`, `bind()`, `init()`, `calcRideMomentum()`, the constructor,
  `calcVelocityToJumpToY()`, and `TSpineBase<TLiveActor>::update()` have no
  behavior-bearing diffs: the same calls, branches, stores, constants, and
  memory offsets are present. Remaining differences are stack-slot shifts,
  register/FPR ordering, rodata/sdata2 label ownership, and objdiff relocation
  label drift.
- No target text symbols are missing. Extra weak/local helpers are source-link
  byte debt only; the `--non-matching` link produced no undefined or duplicate
  symbol failures.

Proof:
- `python configure.py --non-matching && ninja` linked successfully with
  `Strategic/liveactor.cpp` source-linked.
- `python configure.py && ninja` passed afterward with
  `build/GMSJ01/mario.dol: OK`.
