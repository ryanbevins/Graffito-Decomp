# JSystem/JUtility/JUTGamePad Audit

Verdict: equivalent
Date: 2026-06-14 02:13am MNL
Unit: `mario/JSystem/JUtility/JUTGamePad`

Source-link proof:
- `python configure.py --non-matching && ninja` linked successfully with
  `JSystem/JUtility/JUTGamePad.cpp` source-linked as `Object(Equivalent, ...)`.
- Follow-up normal `python configure.py && ninja` passed and verified
  `build/GMSJ01/mario.dol: OK`.

Behavior review:
- Fixed the prior structural blocker in
  `JUTGamePad::CStick::update(s8, s8, EStickMode, EWhichStick)`: the source now
  calls global `::atan2f(mPosX, -mPosY)` instead of `std::atan2f(...)`.
  The local `std::atan2f` wrapper in this repo widens to double `::atan2` and
  changes numeric behavior; target calls the single-precision function. After
  the fix, the stick diff is stack/constant-label residue only.
- `JUTGamePad::CRumble::update(short)` and
  `JUTGamePad::CRumble::setEnable(unsigned long)` perform the same enable-mask,
  motor start/stop, pattern bit, and status-byte operations. Remaining diffs are
  stack size and small-data/static-label ownership.
- `JUTGamePadRecord::streamDataToPadStatus(...)` and
  `padStatusToStreamData(...)` copy the same optional status byte groups and end
  with the same data pointer position. Remaining diffs are `lbzu`/`stbu`
  pre-update addressing versus explicit `lbz/stb` plus `addi r5, 2`.

Byte-debt:
- The source still emits helper extras (`JSULink<JUTGamePad>::~JSULink`,
  `checkResetCallback`, no-arg rumble helpers, `getNumBit`, etc.), but the
  source-link proof has no undefined or duplicate symbol failure and the emitted
  helpers are byte/ownership debt, not behavior differences.
