# Enemy/generator audit

Verdict: equivalent  
Date: 2026-06-13 6:45am MNL
Status: reverified current source-link proof

Proof: `python configure.py --non-matching && ninja` links cleanly with
`Enemy/generator.cpp` built from source. All target `.text` functions are
present. A follow-up plain `python configure.py && ninja` restored the matching
configuration and verified `build/GMSJ01/mario.dol: OK`.

Reason: Non-100% function diffs are codegen-class stack-frame/slot,
register-coloring, and local constant/string-label residue.

Reviewed functions:
- `TGenerator::load(JSUMemoryInputStream&)`: same read/string/random/register
  sequence; residue is larger target stack frame and `r30`/`r31` coloring near
  the random timer conversion.
- `TGenerator::perform(unsigned long, JDrama::TGraphics*)`: same spawn timer,
  manager/graph lookup, matrix multiply, and `resetSRTV` call; residue is stack
  placement for the matrix/vector temps.
- `TOneShotGenerator::loadAfter()`: same manager/graph lookup, hit actor init,
  group lookup, JGadget iterator construction, list insertion, and conductor
  registration. Raw DTK disassembly confirms the objdiff callee-label mismatch
  around the iterator constructors is only symbol/offset presentation.
- `TOneShotGenerator::receiveMessage(THitActor*, unsigned long)`: same actor
  type test, pending gate, enemy spawn path, and pending clear; residue is stack
  placement for matrix/vector temps.

Notes: source emits extra unused weak owners (`JGadget` iterator ctors,
`THitActor`/`JDrama::TViewObj` destructors, and `JDrama::TViewObj` vtable), but
the behavior-visible generator vtables and target functions are present and the
from-source link succeeds.

2026-06-13 10:47am MNL recheck: verdict remains `equivalent`. Re-read the
current diffs for `receiveMessage`, `loadAfter`, `perform`, and
`TGenerator::load` (using mangled `load__10TGenerator` to avoid the one-shot
overload). The generator still performs the same stream reads, random timer
conversion, conductor registration, graph lookup, matrix/vector spawn setup,
enemy reset call, hit-actor/list insertion, and pending message clear. The
scary iterator/vtable labels in `loadAfter` remain owner-label presentation,
with the same call sequence and stores. Proof refreshed with
`python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
