# JSystem/JAudio/JAInterface/JAIBasic Audit

Verdict: not_equivalent / needs_impl
Recorded: 2026-06-13 2:50am MNL

Unit: `mario/JSystem/JAudio/JAInterface/JAIBasic`
Source: `src/JSystem/JAudio/JAInterface/JAIBasic.cpp`

## Blocking Structural Diffs

- `JAIBasic::checkInitDataOnMemory()` is still partially reconstructed. Source
  has an explicit TODO in command `2` and does not implement the full original
  init-data parsing path.
- `JAIBasic::startSoundBasic(...)` is an empty source stub, while target owns a
  376B nonmatching body.
- `JAIBasic::stopSoundHandle(JAISound*, unsigned long)` is an empty source stub
  behind `#pragma dont_inline`, while target owns a 496B nonmatching body.

## Other Notes

- No missing target `.text` symbols in the overview.
- Some small extra 4B text symbols are empty virtual/default API stubs and are
  not the main blocker.
- The TU should remain `NonMatching` until the real sound start/stop logic and
  remaining init-data parser behavior are implemented.
