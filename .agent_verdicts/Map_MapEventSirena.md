# Map/MapEventSirena

Verdict: `Equivalent` reverified.

## 2026-06-13 1:29pm MNL

- Proof earlier this tick, before any source/config changes:
  - `python configure.py --non-matching && ninja` linked `mario.elf` and `mario.dol`.
  - `python configure.py && ninja` linked and checksum passed.
- Reviewed `TMapEventSirenaSink::load(JSUMemoryInputStream&)`, `loadAfter()`, and `watch()`.
- `load` matches behavior: base load, ignored string, warp vector reads, dummy 4-byte read, warp-y read, and particle loads for `0x68` / `0x1e4`.
- `loadAfter` matches behavior: resolves `"ホテル上げカメラ"`, stores the actor pointer, sets frame fields to `240`, radius/threshold to `3500.0f`, and shine position to `(0, 3300, -2570)`.
- `watch` matches behavior: early false when inactive; when active, sets pollution layer bit `2`, clears state, starts the hotel demo camera with zero flags, spawns the shine demo, sets flag `0x50008`, warps Mario, emits particles `0x68` and `0x1e4`, and returns true.
- Residual drift is codegen/data-owner only: frame size, stack slot for `JDrama::TFlagT<u16>`, one scheduled `gpMarDirector` load, and helper/string ownership.
