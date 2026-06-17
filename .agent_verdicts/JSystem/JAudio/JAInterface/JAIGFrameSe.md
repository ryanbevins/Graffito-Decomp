# JSystem/JAudio/JAInterface/JAIGFrameSe

Verdict: equivalent
Date: 2026-06-14 5:34pm MNL

Certified under the current behavioral AUDIT bar. The TU has no missing/extra
symbols and all `.sdata2` rows are exact. Full diff review reclassified the old
red blockers as codegen-class:

- `JAIBasic::checkNextFrameSe()`: same candidate initialization, per-camera
  position transform, distance/priority score calculation, distance cull,
  candidate insertion tie-break (`sound->unk1 <= stored state`), queued-state
  promotion, registry-slot release/reuse, duplicate suppression, and fallback
  slot clear. Remaining drift is register/FPR allocation, stack-frame size,
  conversion order for the two commutative score terms, and equivalent loop
  break/boolean spelling.
- `JAIBasic::releaseSeRegist(JAISound*)`: same port stop, sequence mute
  restoration, category-slot clear, main pointer clear, parameter release, and
  controller release. The slot-clear loop differs by keeping zero in `r3`
  versus loading it into `r0`, plus branch layout.
- `sendPlayingSeCommand()` and `sendSeAllParameter()` have matching command,
  parameter aggregation, clamping, cache update, port-write, and final state
  semantics. Remaining differences are frame/register choices, helper-boundary
  spelling, equality operand order, and address-formation variants.

Proof:
- `python configure.py --non-matching && ninja` linked with
  `JAIGFrameSe.o` sourced.
- `python configure.py && ninja` restored the normal graph and passed with
  `build/GMSJ01/mario.dol: OK`.

---

Verdict: not_equivalent  
Date: 2026-06-13 1:09am MNL

Reason:
- Not promoted. This TU has no missing symbols, but the main nonmatching audio
  routines contain structural-looking diffs that I cannot prove codegen-only in
  an audit pass. The source also labels `checkNextFrameSe()` with a TODO around
  the reconstructed control/data flow.

Blocking functions:
- `JAIBasic::checkNextFrameSe()` at 90.2%: large candidate queue/scoring and
  slot-reassignment regions differ by more than register coloring, including
  reordered conversion blocks and different branch/materialization shape around
  candidate insertion and registry slot filling.
- `JAIBasic::releaseSeRegist(JAISound*)` at 93.5%: slot-clearing/break logic
  and mute-release loop have structural-looking branch/materialization
  differences. They may be equivalent, but this pass could not prove it.

Also requires review before any future green verdict:
- `JAIBasic::sendPlayingSeCommand()` at 95.2%.
- `JAIBasic::sendSeAllParameter(JAISound*)` at 97.1%.
