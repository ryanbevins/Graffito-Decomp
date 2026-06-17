# GC2D/CardLoad Audit

Verdict: equivalent  
Date: 2026-06-15 1:15am MNL

Tick 793 audit certification:

- Certified `GC2D/CardLoad.cpp` as functionally equivalent and promoted it to
  `Object(Equivalent, "GC2D/CardLoad.cpp")`.
- Fixed one real inline helper bug before certification:
  `TExPane::updateCenteredSize(time, target_w, target_h)` must use the current
  offset interpolator values as the centered initial position, not the current
  size interpolator values. This matches the `selectFunction()` target loads
  from `TExPane + 0x14/0x18`.
- Fixed one real `TCardLoad::waitForAnyKey()` animation bug: the close
  animation uses target size `(0, 0)` with the saved pane width/height as the
  initial size, matching raw target asm and sibling `CardSave`.
- Reviewed all remaining non-100% functions against raw asm and
  `decomp-diff`: `changeScene`, `selectFunction`, `selectBookmark`,
  `drawMessageBM`, `drawMessage`, `waitForStart`, `waitForAnyKeyBM`,
  `waitForAnyKey`, `waitForChoiceBM`, `waitForChoice`, `titleDraw`,
  `perform`, `loadAfter`, `setupScoreScreen`, and `load`.
- Required proof passed: `python configure.py --non-matching && ninja`
  linked the source object into `mario.dol`; normal
  `python configure.py && ninja` was restored afterward and passed
  `build/GMSJ01/mario.dol: OK`.

Tick 792 implementation update:

- Fixed missing `.text`: `JSUInputStream::JSUInputStream()` (36B) is now a
  100% owner in `CardLoad.cpp` via an owner-only declaration split.
- `python configure.py --non-matching && ninja` linked with a temporary
  `Object(Equivalent, "GC2D/CardLoad.cpp")`; normal
  `python configure.py && ninja` was restored and passed `mario.dol: OK`.
- Fixed real `TCardLoad::titleDraw()` behavior bugs: state 4 now iterates the
  target 11 title-logo panes, the state-1 pane path advances to state 2 before
  state 3, and the function returns true for states above 4. `titleDraw()`
  moved 90.8% -> 92.6%.
- Follow-up review fixed two more real behavior bugs: `perform()` state 10
  clears Mario state bit `0x8000` (target `rlwinm 0,17,15`) rather than
  `0x400`, and `selectFunction()` states 1 and 4 update the three bookmark
  panes only. The touched diff ranges now match exactly.
- `load()` review fixed real initializer bugs: `unk20C` title delays after
  index 4 get the target +20 frame adjustment, duplicate `unk22E`/`unk248`
  initialization was removed, and the score-screen result panes are searched
  from `unk2C` rather than `unk28`. `load()` improved 92.0% -> 95.6%.
- `waitForAnyKeyBM()` and `waitForStart()` review fixed real pane-target
  behavior bugs: the three-digit score path now updates `unk504[0..2]` rather
  than mixing in `unk514`, and the progress-12/13 start prompt now animates
  `unk54C` after showing it rather than animating `unk568`.
- `drawMessageBM()` case 0 fixed two target-offset mismatches: the third
  bookmark type pane is `unk4CC[2]`, not out-of-bounds `unk4CC[3]`, and the
  `unk4AC` entry animation uses `unk4B0` bounds rather than `unk46C`.
- `waitForChoice()`/`waitForChoiceBM()` fixed stale selected-choice pulse
  parameters. The target and sibling `CardSave` use 40-frame 1.5x pulse
  animations; source still had 20-frame 0.5x pulse-in paths and a 20-frame
  0.5x non-BM pulse-out. `waitForChoice()` improved 98.3% -> 98.4%.
- `setupScoreScreen()` fixed two display-logic bugs: stage completion panes
  now show when the completion flag is set, and the two extra-shine icon
  slots query extra shines 1/2 instead of duplicating 0/1 while extra 0 is
  handled separately.
- `waitForAnyKey()` and `waitForAnyKeyBM()` fixed timeout behavior. Target
  advances after `unkB4 > 600` or the key trigger; source only advanced on a
  key trigger before the timeout. `waitForAnyKey()` improved 98.6% -> 98.9%.
- `selectFunction()` fixed the remaining selected-file pulse-out duration.
  The target `unkC4 == 44` branch uses a 40-frame return from 1.5x size
  (`li r4, 0x28`); source still used 20 frames. The focused range now leaves
  only register/stack residue.
- `load()`/header fixed the title-logo state array sizes. The raw target
  initializes 13 `unk22E` delay entries and 13 `unk248` state entries; the
  source struct only declared eight plus padding and initialized eight. After
  changing both arrays to 13 entries, the initializer block matches
  structurally and `load()` improved 95.6% -> 98.0%.
- `load()` and `setupScoreScreen()` fixed the stage-order local tables.
  Target rodata uses `{2,3,4,5,6,8,7}` for `load()` and
  `{2,3,4,5,6,7,8}` for `setupScoreScreen()`; source had them reversed.
  The rebuilt object now contains the same table contents/order.
- `titleDraw()` fixed return signedness. Target returns signed `unk18 > 4`;
  source used `(u32)unk18 > 4`, which made negative/default states return
  true and emitted a different compare. `titleDraw()` improved
  92.6% -> 94.0%.
- `perform()` fixed the case-0 arrow alpha update structure. Target reuses
  one loaded alpha local across fade-in, fade-out, and the movement test;
  source reloaded between phases, changing behavior if the pane alpha was
  externally modified and producing an inverted code shape. `perform()`
  improved 95.2% -> 97.9%.
- `perform()` fixed the state-5 fade directions. The target fades the score
  screen panes down and the load-menu root up before returning from the score
  screen; source reused the state-4 fade direction, so the screens moved the
  wrong way. `perform()` improved 97.9% -> 98.1%.
- `titleDraw()`/header fixed the `unk22E` delay type to unsigned. Target
  loads these entries with `lhzx` and compares them with `cmplw`; source used
  `s16`, which emitted signed loads/compares. `titleDraw()` improved
  94.0% -> 94.7%.
- `selectBookmark()` fixed the final hide-pass selected-slot check. Target
  tests `unk40[unkB0].unk0` after comparing each pane index to `unkB0`; source
  used `unk40[i].unk0`, which is behavior-equivalent under that guard but not
  the target dataflow. `selectBookmark()` improved 97.3% -> 98.1%.
- Tested but did not keep a `changeScene()` signed previous-progress local:
  it matches the target final `cmpw` but drops the rounded function score.
  Current source remains unsigned there pending a broader investigation pass.

Remaining byte-diff classes after certification:

- Missing `.ctors`: `@7209` (32B), the target top-level `titleDraw()` jump
  table. Source lowers the same state logic as a compare tree; the case bodies,
  state writes, and signed final return have been reviewed as behavior-identical.
- The rebuilt object still emits target-absent helpers such as
  `TCardLoad::setMessage(J2DTextBox*, long, int)`,
  `TCardLoad::makeBuffer(J2DTextBox*, int)`, `loadBookmark()`,
  `clearBookmark(unsigned long)`, and inherited/weak helper owners. Raw asm
  review shows these are helper-boundary/inline-owner residue, not missing
  behavior.
- The remaining instruction diffs are register allocation, stack-slot layout,
  integer/FPR scheduling, equivalent clamp/index arithmetic, and const/string
  label ownership residue.
