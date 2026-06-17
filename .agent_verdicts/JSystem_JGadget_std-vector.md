# JSystem/JGadget/std-vector

Verdict: `Equivalent` reverified.

## 2026-06-13 1:32pm MNL

- Proof earlier this tick, before any source/config changes:
  - `python configure.py --non-matching && ninja` linked `mario.elf` and `mario.dol`.
  - `python configure.py && ninja` linked and checksum passed.
- Reviewed `JGadget::TVector<void*, JGadget::TAllocator<void*>>::InsertRaw(void**, unsigned long)` and `JGadget::TVector_pointer_void::reserve(unsigned long)`.
- `InsertRaw` matches behavior: zero-count returns the insertion pointer; in-capacity paths move/copy the correct ranges for overlap and non-overlap cases, advance `end`, and return the inserted range; growth path computes new capacity, allocates, copies prefix and suffix, updates begin/end/capacity, deletes old storage, and returns the new insertion position.
- `reserve` matches behavior: only grows when requested capacity exceeds current capacity, allocates new storage, copies current range, updates begin/end/capacity, and deletes old storage.
- Residual drift is codegen/symbol-owner only: GPR coloring, label addresses from local weak/specialization ownership, and `mr.` versus `cmplwi` + `addi` null-test spelling in `reserve`.
