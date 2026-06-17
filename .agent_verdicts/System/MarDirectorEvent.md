# mario/System/MarDirectorEvent

Verdict: equivalent
Date: 2026-06-15 6:09pm MNL

Reason: source-link proof passed and remaining diffs are behavioral no-ops:
stack/register/helper-boundary drift, SDA/data label drift, and extra
unreferenced infectious/static-list owner data.

Reviewed non-exact rows:
- `fireStreamingMovie(unsigned char)`: same movie ID switch map, same
  `unk4C & 0x100` guard, same stage IDs (`1`, `0x101`, `0xE06`, `0xE07`,
  `0x3B`, `0x3C`, default `0xF`), same flag writes and movie store. Drift is
  frame size, jump-table label/offsets, and load scheduling.
- `setNextStage(unsigned short, JDrama::TActor*)`: same early return on
  `unk4C & 0x2`, next-area area/episode computation, `param_2` handling,
  Delfino-to-area `5/6/8` branch, fallback `unk4C |= 0x2`, and area `0x37`
  movie path. Drift is `TFlagT<u16>` helper boundary/frame/register shape and
  compare lowering.
- `fireRideYoshi`, `fireGetStar`, `fireGetNozzle`, `fireGetBlueCoin`: same
  guards, constants, calls, flag writes, and side effects. Non-exact rows are
  frame/data-label drift.
- `movement_game()`: same talk-cursor reset, paused/holding/camera exits,
  take-NPC scan, nearest-talk fallback, associate calls, controller flag,
  `unk128`/`unk126` updates. Drift is bool lowering, labels, frame/registers.
- `findNearestTalkNPC()`: same Mario action check, live-flag filters, loop
  bounds, squared-distance computation, and nearest-result update. Drift is
  stack/FPR placement.

Data/extra symbols:
- No missing rows.
- `.data` jump table maps the same cases; offsets differ because function body
  layout differs.
- `.sdata` camera-name relocations target the correct camera globals in the
  rebuilt object; confusing objdiff labels are section-offset symbol drift.
- Extra JSUList destructors and infectious/string owner data are unreferenced
  or behavior-equivalent static-init debt, not source-link blockers.

Proof:
- `python configure.py --non-matching && ninja` linked successfully with
  `System/MarDirectorEvent.cpp` sourced.
- `python configure.py && ninja` passed `build/GMSJ01/mario.dol: OK`.
