# JSystem/JSupport/JSUInputStream

Verdict: `Equivalent` reverified.

## 2026-06-13 1:32pm MNL

- Proof earlier this tick, before any source/config changes:
  - `python configure.py --non-matching && ninja` linked `mario.elf` and `mario.dol`.
  - `python configure.py && ninja` linked and checksum passed.
- Reviewed `JSUInputStream::readString(char*, unsigned short)` at `92.5%`.
- Behavior matches: reads the 16-bit source length, null-terminates and sets EOF on failed length read, reads either the full string or `len - 1` bytes, null-terminates at the actual bytes read, skips the remaining source bytes in the truncation case, sets EOF when total consumed bytes differ from the source length, and returns the caller buffer.
- Residual drift is codegen/data-owner only: saved-register coloring/order and owner drift for `JSUIosBase` destructor/vtable.
