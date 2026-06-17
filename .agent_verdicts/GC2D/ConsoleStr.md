Verdict: needs_impl
Date: 2026-06-13 3:38am MNL
TU: mario/GC2D/ConsoleStr

Reason:
- Current source still has explicit unfinished/TODO behavior in user-visible
  wipe and console-string animations.
- `TConsoleStr::processGo(float)` is structurally incomplete: the effect/rect
  setup loop is commented out as "all wrong", later `param_1 >= 175.0f`
  handling has an empty loop and a TODO branch, and the function has no
  explicit `return` despite returning `bool`.
- `TConsoleStr::perform(unsigned long, JDrama::TGraphics*)` contains TODO
  placeholder branches around `unk2A9`/`unk2AC`, so those state transitions are
  not behavior-auditable.
- `TConsoleStr::startCloseWipe(bool)` is marked "all of this is wrong" and
  should not be certified as functionally identical.

No promotion attempted.
