# JSystem/JAudio/JASystem/JASDSPInterface.cpp

Verdict: equivalent
Date: 2026-06-13 5:50am MNL
Unit: `mario/JSystem/JAudio/JASystem/JASDSPInterface`
Source: `src/JSystem/JAudio/JASystem/JASDSPInterface.cpp`
Classification: `Object(Equivalent, "JSystem/JAudio/JASystem/JASDSPInterface.cpp")`

Reason:
- Re-verified during the audit sweep. The overview has no missing target
  symbols. Static data and all target functions except
  `JASystem::DSPInterface::FXBuffer::setFXLine(short*, FxlineConfig_*)` match
  byte-for-byte.
- `FXBuffer::setFXLine` preserves the same interrupt disable/restore, zero
  active flag, nullable config handling, prefix table lookups, FX field copies,
  optional buffer pointer store, `bzero`/`DCFlushRange` path, active flag write,
  final buffer flush, and return value. The only diff is stack-frame/save-slot
  size.
- Extra standalone DSP helper bodies are unused source-owned helper drift.

Proof:
- `python configure.py --non-matching && ninja` linked
  `build/GMSJ01/mario.dol` from source.
- `python configure.py && ninja` restored the matching config and passed
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

2026-06-13 9:55am MNL recheck:
- Current overview still has no missing target symbols.
- Re-read the full diff for
  `JASystem::DSPInterface::FXBuffer::setFXLine(short*, FxlineConfig_*)`.
  It still performs the same interrupt disable/restore, active-flag clear,
  nullable-config field copies, prefix table lookups, filter copy, optional
  buffer `bzero`/flush path, final active-flag write, struct flush, and true
  return. Residue is stack/save-slot size only.
- Proof batch passed: `python configure.py --non-matching && ninja`, then
  `python configure.py && ninja` with `mario.dol: OK`.

2026-06-14 11:05pm MNL recheck:
- Current overview remains unchanged: no missing target symbols, all data rows
  match, and only `FXBuffer::setFXLine` is nonmatching at 99.9%.
- Re-read the full `--no-collapse` diff for `FXBuffer::setFXLine`; the only
  differences are the frame size and save/restore slot offsets. The interrupt,
  nullable-config, buffer-zero/flush, active-flag, struct-flush, and return
  behavior still match.
- Proof batch passed again: `python configure.py --non-matching && ninja`, then
  `python configure.py && ninja` with `mario.dol: OK`.
