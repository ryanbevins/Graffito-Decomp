# Animal/AnimalNerve audit

Verdict: equivalent
Date: 2026-06-13 7:47am MNL

Reason: reverified in the audit-only sweep. All functions are either
byte-matching or behaviorally aligned, and `python configure.py --non-matching
&& ninja` linked the TU from source. Raw asm confirms the nonmatching execute
body calls the same animation, random timer, distance, stack-pop, graph advance,
and BCK helpers; apparent helper-name drift in objdiff is relocation labeling.

Function review:
- `TNerveAnimalGraphWander::execute(TSpineBase<TLiveActor>*) const`: shared
  animation syncing, bird-specific timer setup/reset, animation-end transition,
  `execWalk(true)`, stacked-path distance check/pop, current-node distance
  check, random graph-node advance, height comparison, BCK selection, and timer
  reset paths match behaviorally. Remaining drift is stack frame size,
  register coloring, equivalent branch layout, and helper-owner/symbol-label
  differences for local wrappers such as distance, stack pop, PAL frame, and
  round helpers.
- `TNerveAnimalGraphWander::theNerve()`: byte-matches.
- `TNerveAnimalGraphWander::~TNerveAnimalGraphWander()`: byte-matches.

Notes:
- Source emits extra helper/weak owners and small data labels, but the required
  `--non-matching` source-link proof passed.
- Plain `python configure.py && ninja` also passed with `mario.dol: OK` after
  the proof build, leaving the repo in matching configuration.

Reverified: 2026-06-13 10:52am MNL — still equivalent. Re-read the current
`TNerveAnimalGraphWander::execute` diff. Animation sync, bird/random timer
setup, animation-end transitions, `execWalk(true)`, stacked-path pop, graph-node
advance, height/BCK selection, and timer reset paths still match behaviorally.
No wrong branch condition, call, constant, or field offset found. Remaining
drift is stack/register layout plus weak/helper owner-label presentation. Proof
passed again with `python configure.py --non-matching && ninja`, then plain
`python configure.py && ninja` restored the matching config and verified
`build/GMSJ01/mario.dol: OK`.
