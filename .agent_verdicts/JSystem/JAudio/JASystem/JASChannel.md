Verdict: equivalent
Date: 2026-06-14 02:11am MNL
Unit: mario/JSystem/JAudio/JASystem/JASChannel

Certified after fixing the prior FIR8 predicate blockers in
`JASystem::Driver::__UpdateJcToDSP` and
`JASystem::Driver::__UpdateJcToDSPInit`: both now test the low five bits of
`TChannelMgr::unk61` (`unk61 & 0x1f`) before
`DSPBuffer::setFIR8FilterParam`, matching the target `clrlwi. ..., 27`
condition instead of testing only bit `0x1`.

Current review:
- The former FIR8 blockers now differ only by stack-frame size.
- `updatecallLogicalChannel`, `updateMixer`, `updatecallDSPChannel`,
  `TChannel::init`, `overwriteOsc`, `stopLogicalChannel`,
  `playLogicalChannel`, and `updateEffectorParam` were rechecked; remaining
  drift is codegen-class frame/register/FPR coloring, argument setup order, and
  equivalent helper/branch layout.
- No target symbols are missing. Extra source-side helper rows are unreferenced
  weak/local emission and do not block source-linking.

Proof:
- `python configure.py && ninja` passed after the predicate fix.
- `python configure.py --non-matching` plus `ninja` linked
  `JASChannel.o` from source.
- Restored normal config with `python configure.py`; normal `ninja` passed with
  `build/GMSJ01/mario.dol: OK`.
