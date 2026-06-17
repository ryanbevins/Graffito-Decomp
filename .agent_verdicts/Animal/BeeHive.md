# Animal/BeeHive

Verdict: equivalent
Date: 2026-06-13 3:22am MNL

Certified functionally identical and source-linkable.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- Plain `python configure.py && ninja` passed and verified
  `build/GMSJ01/mario.dol: OK`.

Review:
- Used `state/notes/Animal_BeeHive.md` plus fresh diffs for
  `TBeeHive::calcRootMatrix`, `TBeeHive::receiveMessage`,
  `TBeeHive::doWait`, and `TNerveBeeHiveFall::execute`.
- `calcRootMatrix` remains a very low byte score because of quaternion and
  `TRotation3f::setSQ` helper-boundary/stack-shape differences; raw notes and
  the diff show the same center-radius/current-quaternion/X-axis-roll matrix
  construction, translation with +120.0f Y offset, and model matrix copy.
- `receiveMessage` covers target messages `0`, `1`, `0xc`, and `0xf` with the
  expected nerve transitions, particle/sound side effects, water-hit swing
  injection, and child/leader updates. The remaining gap is dominated by
  `TQuat4::setRotate` inlining, matrix temp layout, and static nerve label
  ownership.
- `doWait`, `bind`, and the water/fall/break/attack nerves match target state
  transitions and side effects after prior campaign fixes. Remaining diffs are
  repeated save-param accessor source shape, quaternion slerp/normalize
  temporaries, vector helper call boundaries, frame/register coloring, and
  weak/local helper ownership.

No behavioral blocker found. Remaining work is byte-matching/source-shape debt.

## 2026-06-13 11:24am MNL recheck

Refreshed during the stale-Equivalent sweep. Current overview still reports
target-only weak/data owner rows (`TVec3::set`, `TRotation3::setSQ`,
`entry$2985`, and local constants), but `powerpc-eabi-nm -u` on the rebuilt
source object has no undefined references to those helpers. Full diffs for
`TBeeHive::receiveMessage` and `TBeeHive::calcRootMatrix` still classify as
behavior-equivalent: message cases `0`, `1`, `0xc`, and `0xf` reach the same
state transitions and water-hit particle/sound side effects, while the low
matrix score is quaternion/vector helper boundary and stack/FPR scheduling.

Proof passed:
- `python configure.py --non-matching && ninja`
- `python configure.py && ninja` (`build/GMSJ01/mario.dol: OK`)
