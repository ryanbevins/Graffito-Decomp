## System/MarioGamePad

Verdict: equivalent
Audited: 2026-06-13 9:44am MNL

Kept `Object(Equivalent, "System/MarioGamePad.cpp")` after a dedicated
current-source re-audit of `TMarioGamePad::updateMeaning()`.

Evidence:
- `python tools/decomp-diff.py -u mario/System/MarioGamePad` reports no
  missing or extra symbols. Nonmatching rows are only
  `TMarioGamePad::read()` (98.8%), `TMarioGamePad::updateMeaning()` (99.9%),
  and `.sdata2` (80.0%).
- `TMarioGamePad::read()` preserves the same `JUTGamePad::read()` call,
  reset-port stack local, `C3ButtonReset` checks, `mResetFlag` update, and
  `sResetOccurred = false` store. Differences are stack-local offsets, register
  coloring, and local-label numbering.
- Full `--no-collapse` scan of `TMarioGamePad::updateMeaning()` has zero
  opcode/insert/delete diff markers. The function performs the same disabled
  frame decrement, reset-meaning setup, camera/menu branches, analog byte to
  float conversions, all `mMeaning` bit updates, `mCompSPos` writes, and final
  enabled/disabled-frame mask stores. Residue is the 0x190 vs 0x188 frame,
  stack-slot offsets, GPR coloring, and local constant labels.
- Existing source still contains placeholder annotations and a manual
  stack-allocation residue in `updateMeaning`; per the current audit sweep
  rule, this is source-quality/codegen debt for INVESTIGATION mode, not a
  behavioral blocker.
- `.sdata2` bytes contain the same constants; the first four 32-bit values are
  ordered differently (`1.0, 0.0, 0.25, 0.5` in target vs
  `0.25, 0.5, 0.0, 1.0` in source), and the code references the corresponding
  local labels correctly.
- `python configure.py --non-matching && ninja` linked a source DOL with this
  TU enabled.
- `python configure.py && ninja` restored the matching build and passed the
  DOL hash check.
- 9:44am MNL recheck: fresh full/ranged diffs still show only
  stack-local/register/local-label drift. `read()` keeps the same
  `JUTGamePad::read`, reset-port check, reset-flag update, and
  `sResetOccurred` clear. `updateMeaning()` keeps the same disabled-frame
  decrement, meaning resets, camera/menu branches, analog conversions,
  meaning-bit updates, `mCompSPos` stores, and final enabled/disabled masks.

- 2026-06-14 9:35pm MNL safety-net recheck: overview still has no missing or
  extra rows. `read()` still preserves the same reset side effects; the
  remaining `r3/r4/r5` drift only affects the undefined/no-source-return value
  of a function whose sole in-DOL call (`Application.cpp`) ignores the result.
  `updateMeaning()` still has no opcode/insert/delete mismatches in the compact
  diff; residue is the known `0x190` vs `0x188` frame, stack-slot offsets,
  GPR coloring, and local constant labels. The 9:31pm MapObjHide
  `--non-matching` and normal proof builds covered this existing source-linked
  object.
