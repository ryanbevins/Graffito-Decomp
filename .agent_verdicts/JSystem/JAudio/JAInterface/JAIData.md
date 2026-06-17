# JSystem/JAudio/JAInterface/JAIData audit

Verdict: equivalent
Date: 2026-06-14 11:15am MNL
Unit: `mario/JSystem/JAudio/JAInterface/JAIData`

Promoted to `Object(Equivalent, "JSystem/JAudio/JAInterface/JAIData.cpp")`.

Fix before certification:
- Completed the `JAIData::initData()` FX-line config tail. Source now copies
  the four buffer counts from `unk1F4->unk6C + 4..0x10` into `unk18C[0..3]`,
  allocates `unk1AC`, and fills each config pointer from the offset table at
  `unk6C + 0x14`. This removes the old behavior bug where
  `getFXHandle(i)->setFXLine(...)` could read uninitialized `unk1AC` entries.

Reviewed functions:
- `initSeqParameter()` matches the sequence parameter defaults, track loops,
  bitfield clears, and per-track move parameter initialization. Remaining diffs
  are stack/register and store-order shape.
- `getFreeStayHeapPointer()` matches stay-heap capacity checks, aligned size
  rounding, heap block advancement, ID storage, and null fallback.
- `getInfoPointer()` matches table selection for SE/seq/stream IDs, category
  extraction, null fallback, bounds check, and final info pointer calculation.
- `initData()` now matches the large setup sequence behavior: info tables,
  scene-derived SE track max, dummy/link/sound buffers, parameter buffers,
  auto/stay heap blocks, seq/stream update buffers, default sound-scene table,
  and FX-line config setup.
- `initInfoDataWork()` matches file load/reuse, table header parsing, count
  and pointer table setup, and last-used category tracking.

Proof:
- `python configure.py --non-matching && ninja` links with `JAIData.o` built
  from source.
- `python configure.py && ninja` restores the normal matching config and passes
  `build/GMSJ01/mario.dol: OK`.

Residual debt:
- Source still emits several no-op/UNUSED JAIData helper extras. They are
  source-link safe and behavior-neutral; remaining text differences are
  register allocation, store ordering, and loop lowering.
