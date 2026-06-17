# Map/PollutionAction audit

Verdict: equivalent  
Date: 2026-06-13 6:45am MNL
Status: reverified current source-link proof

Proof: `python configure.py --non-matching && ninja` links cleanly with
`Map/PollutionAction.cpp` built from source. All target `.text`, static data,
and static-init symbols are present. A follow-up plain `python configure.py &&
ninja` restored the matching configuration and verified
`build/GMSJ01/mario.dol: OK`.

Reason: Non-100% function diffs are codegen-class
stack-frame/slot, register/FPR-coloring, local label ownership, and one
behavior-equivalent redundant branch in the `action()` switch dispatch.

Reviewed functions:
- `TPollutionLayer::action()`: same plane gate, action-mode dispatch, fire /
  thunder / glass-wall paths, random polluted-position search, particle/sound
  calls, timers, spread stamping, and counters. Residue is stack/register/FPR
  coloring plus the target's redundant `bge` in the switch tree.
- `TPollutionLayer::fire()`: same polluted-position search, fire wait counter,
  sound, two emitter spawns, scale writes, ring index wrap, and counter reset.
- `TPollutionLayer::getPollutedPos(float, TVec3*)`: same five random attempts
  and `isPolluted` test; residue is stack frame/slot placement.
- `TPollutionLayer::getPollutedPosNear(float, TVec3*)`: same random near-Mario
  sampling, area/bounds checks, depth lookup, Mario-height rejection, pollution
  byte lookup, and five-attempt fallback.

Notes: `.sdata` bytes match exactly. Source emits extra unused JSUList weak
destructors/smList ownership from rogue MSound includes, but the source-link
build succeeds and behavior-visible symbols are present.

Reverified: 2026-06-13 10:54am MNL — still equivalent. Re-read all four
nonmatching functions. `action()` still preserves the plane gate, mode dispatch,
fire/thunder/glass-wall effects, random polluted-position search, timers,
particles/sounds, spread stamping, and counters; `fire()` and both polluted-pos
helpers still preserve their attempts, predicates, constants, and stores.
Remaining drift is stack/register/FPR layout, local label ownership, and the
target's redundant switch-tree branch. Proof passed again with `python
configure.py --non-matching && ninja`, then plain `python configure.py && ninja`
restored the matching config and verified `build/GMSJ01/mario.dol: OK`.
