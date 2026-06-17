# System/TalkCursor audit

Verdict: equivalent
Date: 2026-06-13 7:35am MNL

Reason: all functions are either byte-matching or behaviorally aligned, and
`python configure.py --non-matching && ninja` linked the TU from source.

Function review:
- `TTalkCursor::associateNPC(TBaseNPC*)`: null NPC path sets the hidden flag;
  non-null path reads the NPC cursor position, builds a translation matrix,
  copies it to the model matrix, and clears the hidden flag. Remaining drift is
  stack frame size, sret/temp copy layout, and helper-owner/symbol-label residue
  around `identity33`.
- `TTalkCursor::loadAfter()`: allocation, animation-data init, model resource
  load, model setup, BCK/BRK selection, and hidden flag set match. Remaining
  drift is stack size only.
- `TTalkCursor::perform(unsigned long, JDrama::TGraphics*)`: byte-matches.
- `TTalkCursor::~TTalkCursor()`: byte-matches.

Notes:
- Source emits extra infectious-string and weak helper owners, but the required
  `--non-matching` source-link proof passed.
- Reverified this pass against the full current diffs: `associateNPC` still
  matches both the hide-on-null path and the cursor-position translation/model
  matrix copy path, with the misleading helper-owner label around `identity33`
  unchanged. `loadAfter` remains allocation/setup/BCK/BRK/hide-equivalent with
  only stack-size residue.

2026-06-13 10:43am MNL recheck: verdict remains `equivalent`. Re-read the
current diffs for `associateNPC` and `loadAfter`. `associateNPC` still hides on
null NPC, otherwise reads the NPC cursor position, builds the translation
matrix, copies it into the model matrix, and clears the hide flag. The apparent
call-label mismatch at the translation helper is owner-label drift around the
same matrix setup; the following stores and `PSMTXCopy` destination match.
`loadAfter` still allocates animation data, initializes model data, loads and
sets up the model, applies BCK/BRK names, and hides the cursor. Proof refreshed
with `python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
