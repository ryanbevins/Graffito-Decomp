# MoveBG/MapObjFloat

Verdict: `Equivalent` reverified.

## 2026-06-13 1:29pm MNL

- Proof earlier this tick, before any source/config changes:
  - `python configure.py --non-matching && ninja` linked `mario.elf` and `mario.dol`.
  - `python configure.py && ninja` linked and checksum passed.
- Reviewed `TMapObjFloatOnSea::initMapObj()` at `97.6%`.
- Behavior matches: calls `TLeanBlock::initMapObj`, scans `param_table` with `strcmp(unkF4)`, copies the same float fields to `unk140/144/148/138/13C/1AC`, sets `unk198 = 0xf`, loads particles `0x1f6` and `0x1c6`, and constructs `TMapObjLibWave` with the same four table fields.
- Residual drift is codegen/data-owner only: GPR coloring, local label names for rodata/string table, and helper/destructor ownership from rogue includes.
