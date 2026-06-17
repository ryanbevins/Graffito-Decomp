# JSystem/JAudio/JAInterface/JAIGFrameSequence

Verdict: not_equivalent
Date: 2026-06-13 1:28am MNL

Reason:
- `JAIBasic::checkPlayingSeqTrack(unsigned long)` is a 3740B function at
  74.9% and is not certifiable as codegen-only. The current source still
  relies on `JAISound::FabricatedPositionInfo` / unknown sequence parameter
  layout fields and has large structural drift through the 3D position update,
  distance parameter updates, track-port writes, and flag-clearing blocks.
- `JAIBasic::stopSeq(JAISound*)` and `JAIBasic::checkSeqWave()` are close
  stack/operand residues, and several smaller sequence state checks already
  byte-match, but the large track updater is the behavioral center of the TU.
  Leave the unit `NonMatching` until that function's layout and control flow
  are proven.

Offending function:
- `JAIBasic::checkPlayingSeqTrack(unsigned long)`
