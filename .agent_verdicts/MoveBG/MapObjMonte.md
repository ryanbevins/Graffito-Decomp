# Audit verdict: equivalent

Date: 2026-06-14 8:51pm MNL
Mode: AUDIT
Unit: `mario/MoveBG/MapObjMonte`
Source: `src/MoveBG/MapObjMonte.cpp`

Verdict: `equivalent`

Reason:
- The prior implementation pass fixed the only known behavior bug in
  `THangingBridge::loadAfter()`: newly registered bridge boards now dispatch
  `appear()` through vtable slot `0xfc` after `unk1BC` is linked.
- Re-reviewed the current overview and source for missing/stubbed target text.
  No target text symbols are missing; remaining missing/extra rows are
  `.ctors`/data-label drift and source-only weak helpers/destructors.
- Remaining nonmatching function diffs are codegen-class residue: stack and
  register allocation, FPR coloring, helper-boundary differences
  (`TUtil<f32>::sqrt`, matrix/vector helpers), local data-label ownership, and
  draw/FIFO macro spelling. I did not find a remaining behavioral mismatch in
  the fluff, swing-board, hanging-bridge, board, goal-flag, jump-mushroom, or
  Monte-root paths.

Proof:
- `python configure.py --non-matching && ninja` linked with
  `MoveBG/MapObjMonte.cpp` sourced.
- `python configure.py && ninja` restored matching config and passed
  `build/GMSJ01/mario.dol: OK`.

Byte-debt to leave for INVESTIGATION:
- Local data label drift around target `@2111/@2178/@2179/@2180/@2181`,
  `@2934`, and `@3352`.
- Source-only weak/destructor helper emission such as `TTakeActor::moveRequest`,
  `TMapCollisionBase::moveMtx`, `TLeanBlock::~TLeanBlock`, and JALList
  destructor owners.
