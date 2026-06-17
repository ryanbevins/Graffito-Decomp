# GC2D/SelectMenu Audit

Verdict: equivalent
Recorded: 2026-06-13 2:47am MNL

Unit: `mario/GC2D/SelectMenu`
Source: `src/GC2D/SelectMenu.cpp`

## Evidence

- No missing target `.text` symbols.
- `TSelectMenu::startOpenWindow()` keeps the same pane visibility reset,
  frame calculation, size/offset setup, BGM start, and `_138` reset. Remaining
  differences are stack/register choices.
- `TSelectMenu::startMove()` keeps the same `TSelectShineManager::initData`
  call and current-shine `unk24` mark. Remaining difference is stack/register
  codegen.
- `TSelectGrad::perform(...)` keeps the same RGB channel mode updates, mode
  rollover, GX setup, midpoint color calculation, and quad draw. Remaining
  differences are stack/register choices and byte-store packing for color
  locals.
- `TSelectMenu::perform(...)` preserves the state-machine behavior: open
  transitions, pane animations, pad input branches, shine-manager updates,
  texture/message swaps, left/right arrow fades, selected-pane alpha pulsing,
  close fade, and final screen draw. The large fuzzy gap is from stack frame
  size, register allocation, local temporary construction, and inline/call
  selection for pure helpers.
- `TSelectMenu::initData(...)` preserves screen/pane allocation, texture table
  loading, coin/shine display setup, stage-state scan, arrow pane setup,
  scenario icon setup, font setup, and final scenario-name copy. Remaining
  differences are local table materialization, cached member pointers, stack
  frame/register allocation, and anonymous/local data labels.
- Missing `.ctors` rows `@1431`, `@1411`, and `@1210` are paired with matching
  local dummy arrays (`dummy1431`, `dummy1411`, `dummy1210`) of the same size;
  this is data-label ownership rather than behavior.

## Build Proof

- `python configure.py --non-matching && ninja` linked `mario.elf` and produced
  `mario.dol` successfully after the promotion.

## 2026-06-13 11:31am MNL Recheck

Verdict remains equivalent.

- Overview still has no missing target `.text` rows. The remaining missing
  `.ctors` rows are still paired by local dummy data of the same sizes.
- Rechecked `TSelectMenu::perform`, `TSelectMenu::initData`,
  `TSelectMenu::startOpenWindow`, `TSelectMenu::startMove`,
  `TSelectGrad::perform`, `getPrevIndex`, `getNextIndex`, and
  `SMS_getShineID`.
- The large `perform`/`initData` residues are still codegen-class: stack frame
  size, local table/data ownership, register allocation, color temporary
  packing, and inline/helper selection. State writes, pane visibility and
  animation updates, shine/texture/message selection, alpha clamps, draw setup,
  and GX calls preserve the target behavior.
- Proof: `python configure.py --non-matching && ninja` linked from source, then
  `python configure.py && ninja` restored the normal config and verified
  `build/GMSJ01/mario.dol: OK`.
