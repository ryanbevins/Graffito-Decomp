# MoveBG/MapObjBall

Verdict: equivalent
Date: 2026-06-15 7:32pm MNL
TU: `mario/MoveBG/MapObjBall`

Strict AUDIT promoted the TU after fixing five real behavior mismatches found
in the low-score rows:

- `TResetFruit::touchWater()` and `TMapObjBall::touchWater()` now call
  `getWaterSpeed()` on the water/sender actor, matching the target helper's
  use of the water actor ID at offset `0x68`.
- `TMapObjBall::calcCurrentMtx()` uses the target zero-axis threshold
  `dot <= 0.0000038146973f`.
- `TMapObjBall::boundByActor()` uses `gpMarioPos->y` for the Mario
  top-collision height check.
- `TBigWatermelon::rebound()` restores the target `state 0xB -> 0xC` tail.
- `TResetFruit::receiveMessage()` preserves the target message-6 side effect:
  when state is `1`, set state to `0xB` after the lower-half
  general/hold/kick handling while preserving the return value.

Reviewed the remaining non-exact text rows as codegen/data debt only:
stack/register placement, helper inlining versus calls (`sqrt`,
`getWaterSpeed`, `gekko_ps_copy12`, vector helpers), float-store versus word-copy
shape, branch layout, jump-table/local label drift, and source-only weak/static
owners. Data sections still have layout/label drift but no missing rows or
behavioral constants after the implementation pass restored the `TResetFruit`
static parameters and `0x40000393` ball tuning constants.

Proof:

- `python tools/decomp-diff.py -u mario/MoveBG/MapObjBall -s missing` reports
  no missing rows.
- `python configure.py --non-matching && ninja` linked with
  `Object(Equivalent, "MoveBG/MapObjBall.cpp")`.
- `python configure.py && ninja` passed with `build/GMSJ01/mario.dol: OK`.
