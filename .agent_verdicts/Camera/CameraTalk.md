# mario/Camera/CameraTalk

Verdict: equivalent
Date: 2026-06-13 1:07pm MNL

Reason:
- Reverified current `Object(Equivalent, "Camera/CameraTalk.cpp")` again during
  the audit-only sweep.
- `CPolarSubCamera::makeMtxForPrevTalk()` differs only by stack-frame size and saved-register slot offsets.
- `CPolarSubCamera::makeMtxForTalk(const TBaseNPC*)` has an equivalent branch layout for choosing talk camera modes:
  - `0x400001A` -> mode `0x40`
  - `0x4000007` -> mode `0x0A`
  - values in `(0x400001A, 0x400001C)` -> mode `0x3F`
  - otherwise small NPCs -> mode `0x2D`
- `CPolarSubCamera::ctrlTalkCamera_()` already byte-matches. No missing target symbols were present.

Proof:
- `python tools/decomp-diff.py -u mario/Camera/CameraTalk`
- `python tools/decomp-diff.py -u mario/Camera/CameraTalk -d "makeMtxForPrevTalk" --no-collapse`
- `python tools/decomp-diff.py -u mario/Camera/CameraTalk -d "makeMtxForTalk" --no-collapse`
- Shared proof: `python configure.py --non-matching && ninja` passed, then `python configure.py && ninja` restored the normal matching config and verified `build/GMSJ01/mario.dol: OK` at 2026-06-13 1:07pm MNL.
