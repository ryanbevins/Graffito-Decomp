# mario/System/FlagManager

Verdict: equivalent
Time: 2026-06-13 9:44am MNL

## Verdict
equivalent

## Date
2026-06-13 6:57am MNL

## Reason
All target functions are present and the two nonmatching emitted functions are
codegen-only:

- `TFlagManager::save(JSUMemoryOutputStream&)`: instruction stream is identical;
  only the frame size differs (`0x50` target vs `0x48` build) and saved-register
  restore offsets follow from that frame delta.
- `TFlagManager::start(JKRHeap*)`: instruction stream is identical; only the
  frame size differs (`0x28` target vs `0x20` build) and saved-register restore
  offsets follow from that frame delta. `objdiff` mislabels the source-side SDA
  relocations as `scShineTableBianco` because the source object owns extra data;
  raw `powerpc-eabi-objdump -dr` shows both target and source relocations point
  to `smInstance__12TFlagManager`.

`python configure.py --non-matching && ninja` linked cleanly after promoting the
TU, so the extra emitted unused helpers/data do not block source linking.

2026-06-13 6:29am MNL recheck: overview still has no missing target rows, and
`python configure.py --non-matching && ninja` linked from source.

2026-06-13 6:57am MNL recheck: full diffs for `save` and `start` still have no
opcode/insert/delete mismatches; all other target text rows are exact. Current
source-link proof (`python configure.py --non-matching && ninja`) and normal
hash proof (`python configure.py && ninja`) both passed during the audit tick.

2026-06-13 9:44am MNL recheck: current overview is unchanged. Fresh full diffs
for `TFlagManager::save(JSUMemoryOutputStream&)` and `start(JKRHeap*)` still
show identical operations: save-time backup/update, option flag writes, all
stream writes/seeks, singleton allocation, constructor/reset calls, memcpy
copies, and singleton store. Residue is frame size/save-slot offsets and local
SDA label naming. Source-link and normal hash proof both passed again.

2026-06-14 9:34pm MNL safety-net recheck: current overview still has no
missing target rows. `save(JSUMemoryOutputStream&)` remains instruction-identical
except for the target `0x50` vs source `0x48` frame and derived save-slot
offsets. `start(JKRHeap*)` remains instruction-identical except for the target
`0x28` vs source `0x20` frame; objdiff still prints source-side SDA references
as `scShineTableBianco`, but this is local data-owner label drift for
`smInstance__12TFlagManager`. The 9:31pm `--non-matching` and normal proof
builds from the MapObjHide audit covered this existing source-linked object.
