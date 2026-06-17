## Verdict: equivalent

Date: 2026-06-13 5:50am MNL
Unit: `mario/JSystem/JAudio/JASystem/JASDSPChannel`
Source: `src/JSystem/JAudio/JASystem/JASDSPChannel.cpp`
Classification: `Object(Equivalent, "JSystem/JAudio/JASystem/JASDSPChannel.cpp")`

Reason:
- Re-verified during the audit sweep. The overview has no missing target
  symbols. Static data and most functions match byte-for-byte.
- `getLower()` and `getLowerActive()` execute the same 64-channel scan over
  `old_time`, skip the same channel states, select by the same priority and
  age tie-break rules, and return the same channel slot. The diff is only GPR
  coloring for the base pointer and priority byte.
- `updateAll()` performs the same `OSGetTick` history update, DSP-limit ratio
  test, lower-channel break sequence, 64-channel update loop, callback/stop/
  dequeue/flush behavior, `checkQueue`, and `PPCSync`. Remaining residue is
  stack-frame size, save-slot offsets, and elapsed-time register coloring.
- Extra helper/accessor bodies are unused source-owned helper drift; target
  call sites either inline the behavior or do not call those standalone bodies.

Proof:
- `python configure.py --non-matching && ninja` linked
  `build/GMSJ01/mario.dol` from source.
- `python configure.py && ninja` restored the matching config and passed
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

2026-06-13 9:55am MNL recheck:
- Current overview still has no missing target symbols.
- Re-read full diffs for `getLower`, `getLowerActive`, and `updateAll`.
  The lower-channel scans still use the same 64-entry loop, state skips,
  priority comparison, age tie-break, and selected-slot return. `updateAll`
  still performs the same tick/history update, DSP ratio gate, inlined
  `getLowerActive`/force-stop path (confirmed in raw target asm), 64-channel
  update loop, callback/dequeue/flush behavior, `checkQueue`, and `PPCSync`.
- Remaining residue is stack-frame size, save-slot offsets, register coloring,
 and source-owned helper emissions. Proof batch passed: `python configure.py
 --non-matching && ninja`, then `python configure.py && ninja` with
 `mario.dol: OK`.

2026-06-15 1:48am MNL safety-net recheck:
- Verdict remains `equivalent`.
- Current overview still has no missing target symbols and all data sections
  match byte-for-byte. Extra helper/accessor bodies remain source-owned byte
  debt only.
- Re-read full diffs for `getLower`, `getLowerActive`, and `updateAll`.
  `getLower`/`getLowerActive` still perform the same 64-channel scans, state
  skips, priority comparison, age tie-break, and returned slot selection.
  `updateAll` still performs the same tick/history update, DSP ratio gate,
  lower-channel force-delete path, 64-channel update loop, callback/dequeue/
  flush handling, `checkQueue`, and `PPCSync`.
- Remaining residue is register coloring and stack-frame/save-slot offsets.
  Proof passed again with `python configure.py --non-matching && ninja`, then
  normal `python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
