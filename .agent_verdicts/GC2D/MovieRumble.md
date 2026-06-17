## GC2D/MovieRumble

Verdict: equivalent
Audited: 2026-06-12 11:03am MNL
Date: 2026-06-13 9:12am MNL
Reverified: 2026-06-13 9:12am MNL — current source still links under
`python configure.py --non-matching && ninja`; restored matching build with
`python configure.py && ninja` and DOL hash check passed.

Promoted `Object(NonMatching, "GC2D/MovieRumble.cpp")` to `Equivalent`.

Evidence:
- Audited `init`, `perform`, and `checkRumbleOff`; constructor and destructor
  already byte-match.
- `init` preserves the same BCR path construction, `Koga::ToolData`
  allocation/attach, `dataExists` check, initial group setup, inlined
  `readCurInfo`, and `unk28 = false` store.
- `perform` preserves the same bit-0 gate and inlined movement logic. The
  objdiff label for the `unk28` path looked recursive, but the raw TU assembly
  shows the call is `bl checkRumbleOff__12TMovieRumbleFv`.
- `checkRumbleOff` preserves the same active-rumble/end-frame checks, stop
  call, group increment, inlined `readCurInfo`, and `unk28 = false` store.
- Remaining differences are frame size, register allocation, local rodata
  labels/offsets, and weak owner/data residue. The target rodata strings and
  sdata strings used by the functions are represented in source.
- Extra `JDrama::TViewObj` weak symbols and placeholder string owners are
  target-absent but unused; source-link validation succeeds.
- `python configure.py --non-matching && ninja` linked a source DOL with this
  TU enabled.
- `python configure.py && ninja` restored the matching build and passed the
  DOL hash check.
- 2026-06-13 9:12am MNL recheck: raw asm again confirms the objdiff-labeled
  recursive call in `perform` is the expected `checkRumbleOff()` call. The
  current diffs are still stack/register/string-label residue with identical
  rumble start/stop, group advance, `readCurInfo`, and `unk28` state behavior.
- 2026-06-13 12:52pm MNL recheck: current overview still has no missing target
  symbols. Re-read `checkRumbleOff`, `perform`, and `init`; they still perform
  the same active/end-frame checks, rumble stop/start calls, group advance,
  `Koga::ToolData` value reads, `readCurInfo` behavior, and `unk28` state
  stores. Residue is frame size, register allocation, and local string/data
  labels. Reused this tick's successful source-link and normal DOL proof batch.

Offending functions: none.
