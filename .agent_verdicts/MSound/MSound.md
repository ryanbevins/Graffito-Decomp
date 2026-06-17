# MSound/MSound

Verdict: equivalent
Date: 2026-06-13 3:16am MNL

Certified functionally identical and source-linkable.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- Plain `python configure.py && ninja` passed and verified
  `build/GMSJ01/mario.dol: OK`.

Review:
- Used `state/notes/MSound.md` plus fresh diffs for the largest remaining
  rows: `MSound::startMarioVoice`, `MSSeCallBack::setParameterSeqSync`, and
  `MSound::MSound(...)`.
- `startMarioVoice` remaining 71.5% diff is switch/compare layout,
  register/frame shape, and local `JAIActor` placement. Raw target paths match
  the source behavior: status-bit early return, slot selection, status-6
  special voice gate, occupied-slot suppression list, low-HP and random voice
  remaps, `0x7865` random-play side effect, `startSoundActorInner`, and post
  start port/pitch/volume writes.
- `setParameterSeqSync` remaining 67.6% diff is switch layout, stack/register
  placement, and static bool guard shape. The meaningful operations match the
  target behavior: base seq init plus track register writes, water filter port
  ramping, param-0x14 category scan and empty follow-up loop, parent-track
  pan/register setup, `unkD1` toggle, `unkCD/unkCE` gate, and ukulele flag
  toggle/read cases.
- Constructor residue is class-boundary/string-pool/frame codegen: automatic
  `JAICamera[2]` construction, heap/global path setup, JAIGlobalParameter
  setup, driver/interface init, category-volume initialization, globals, and
  final field initialization all reach the target state.

Remaining byte debt: switch table shape, stack/register/FPR coloring, string
pool labels, target-absent weak helper ownership, and minor rodata/data label
layout. No behavioral blocker found.

Reverified: 2026-06-13 11:41am MNL — verdict remains `equivalent`. Current
overview still has source-link-safe missing/extra local owner rows only; the
largest current diffs (`startMarioVoice`, `setParameterSeqSync`, and
`MSound::MSound`) were spot-checked against `state/notes/MSound.md`. The voice
ID remap cases, occupied-slot early returns, special `0x7865` random-play path,
post-start port/pitch/volume writes, sequence-sync port/register cases, and
constructor field/global initialization remain behaviorally aligned. Shared
proof passed: `python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` verified `build/GMSJ01/mario.dol: OK`.
