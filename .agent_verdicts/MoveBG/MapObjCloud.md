# MoveBG/MapObjCloud

Verdict: equivalent
Date: 2026-06-12 2:44pm MNL

Reason: all target text functions are present. The remaining text diffs are
behavior-equivalent:

- `TRideCloud::control`: same base control, Mario-speed collision type switch,
  riding/cushion clamp, water-scale timer and scale clamps, hit radius/height
  updates, recycle/rail guards, delay timer, dummy graph guard, move-to-next,
  rail flag side effects, shortest-next movement, pitch wait, yaw speed update,
  and resetStep call. Residue is stack/register allocation, local CSE/reload
  shape for graph-web/current-node access, and operand ordering in equivalent
  multiply chains.
- `TRideCloud::load`: same stream reads, color component masking/stores, TEV
  packet setup, and live-flag toggles. Residue is stack size/slot layout.
- `TRideCloud::setGroundCollision`: same map-collision null guards, matrix copy,
  and virtual `moveMtx` call; objdiff labels the helper owner differently even
  though the instruction stream is otherwise exact.

Data note: `.data`, `.sdata`, and `.sdata2` match; extras are source-owned weak
helpers/destructors and MSound list teardown symbols.

Proof: `python configure.py --non-matching && ninja` linked from source, then
plain `python configure.py && ninja` passed and verified `mario.dol: OK`.

2026-06-13 8:13am MNL recheck:
- Overview still has no missing target functions. Nonmatching text remains
  limited to `TRideCloud::control`, `TRideCloud::load`, and
  `TRideCloud::setGroundCollision`; data sections still match except for
  source-owned extras.
- `TRideCloud::control`: current full diff preserves the same base control,
  Mario Y-speed BG-type switch, riding/cushion clamp, water-scale countdown,
  scale clamps, hit radius/height writes, recycle/rail guards, graph dummy
  guard, move-to-next path, rail-node flag side effects, shortest-next move,
  pitch wait, yaw-speed update, and `resetStep` call. Residue is frame size,
  saved-register coloring, graph-node reload/CSE shape, and equivalent multiply
  operand ordering.
- `TRideCloud::load`: current full diff preserves the same stream reads, color
  component masking/stores, TEV packet setup, and live-flag toggles. Residue is
  stack frame/slot size only.
- `TRideCloud::setGroundCollision`: current full diff preserves the same null
  guards, `getModel()->getAnmMtx(0)` copy, and virtual `moveMtx` call. Direct
  objdump of both target and source objects confirms the suspicious objdiff
  label at the matrix-copy call is `gekko_ps_copy12` on both sides; the drift is
  only stack slot placement.
- Proof refreshed in the same audit sweep: `python configure.py --non-matching
  && ninja` linked from source, and normal `python configure.py && ninja`
  verified `build/GMSJ01/mario.dol: OK`.

2026-06-13 11:43am MNL recheck: verdict remains `equivalent`. Current overview
is unchanged: no missing target rows, matching data/sdata/sdata2, and the same
three nonmatching text functions already reviewed above. The existing full-diff
classification remains valid: cloud control/load/ground-collision behavior is
aligned, with only stack/register/CSE/helper-owner residue. Shared proof from
this tick passed: `python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` verified `build/GMSJ01/mario.dol: OK`.
