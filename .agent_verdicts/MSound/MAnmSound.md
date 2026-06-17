# MSound/MAnmSound

Verdict: equivalent
Date: 2026-06-13 2:42pm MNL

Revalidated during the AUDIT secondary safety sweep. Current overview has no
missing symbols; the existing `Object(Equivalent, "MSound/MAnmSound.cpp")`
classification remains behaviorally sound.

Current full diff classification:
- `MAnmSound::startAnimSound`: same `MSGMSound->gateCheck`, top-bit category
  remap, category `0` actor-flag suppress path, category `7`
  `startMarioVoice`, and default `MSoundSE::startSoundActorInner` call. The
  `actor->unkC & 0x1000` arm has equivalent branch layout: flag set exits,
  flag clear starts the actor sound.
- `MAnmSoundNPC::startAnimSound`: same loop-count/modulo gate, random gate,
  `checkMonoSound`, actor sound start, null sound guard, `0x8000` volume skip,
  camera distance component math, volume-curve selection from bits 12..14, and
  final `setSeInterVolume`. Target calls weak `std::sqrtf`; source inlines the
  same positive-input frsqrte refinement and non-positive passthrough body.

Proof reused from this tick: `python configure.py --non-matching && ninja`
linked with this row from source, then `python configure.py && ninja` restored
normal config and verified `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

Verdict: equivalent
Date: 2026-06-13 9:08am MNL

Refreshed existing `Object(Equivalent, "MSound/MAnmSound.cpp")` during the
AUDIT sweep.

Reviewed functions:
- `MAnmSound::startAnimSound(void*, unsigned long, JAISound**, JAIActor*,
  unsigned char)` preserves the gate check, top-bit category remap, category
  `0` actor-flag suppress path, category `7` Mario voice path, and default
  `MSoundSE::startSoundActorInner` path. Residue is branch layout, stack size,
  and signed-extension/register choice.
- `MAnmSoundNPC::startAnimSound(void*, unsigned long, JAISound**, JAIActor*,
  unsigned char)` preserves loop/random gating from frame sound flags,
  mono-sound check, actor sound start, `0x8000` volume-skip flag, optional
  camera-to-actor distance calculation, volume curve selection, and final
  `setSeInterVolume`. The target calls weak `std::sqrtf`; current source
  inlines the same frsqrte refinement sequence and non-positive passthrough
  from that weak body.
- `MAnmSound::setSpeedModifySound`, `animeLoop`, `initAnmSound`, constructor,
  and `__sinit_MAnmSound_cpp` byte-match.
- Source-only `JSUList` destructors and minor `.sdata2` label drift are
  ownership/codegen residue accepted by source-link validation.

Validation:
- `python configure.py --non-matching && ninja` linked successfully with
  `MAnmSound` from source.
- `python configure.py && ninja` restored the matching config and verified
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.
