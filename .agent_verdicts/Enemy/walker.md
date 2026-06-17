# Audit verdict: equivalent

Verdict: equivalent
Time: 2026-06-13 6:33am MNL

Date: 2026-06-13 12:17am MNL

`mario/Enemy/walker` is functionally equivalent.

Reviewed functions:

- `calcFarthestVertex(const TBGCheckData*, const TVec3f&, const TVec3f&)`:
  same three horizontal vertex deltas, positive-dot filter, max squared
  distance selection, empty-set zero return, and single `frsqrte` sqrt
  approximation. Residue is stack-slot layout for the local vector array and
  volatile sqrt temp.
- `TWalker::bind(TLiveActor*)`: behavior matches for spider delegate mode,
  next-position integration, airborne velocity/gravity clamp, ground/ignore-water
  checks, illegal-ground timeout/kill path, landing flag clears, wall check
  record setup, new-wall selection against the ring buffer, pool-edge marker,
  wall normal tangent direction, farthest-vertex probes in both tangent
  directions, path-node push/update, and final linear-velocity write. The large
  diff is codegen-class helper/stack residue: `TVec3` copy slots, by-value
  scale/add/sub helper ownership labels, saved-register coloring, and local
  vector temporary placement.

Proof:

- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` passed with `build/GMSJ01/mario.dol: OK`.
- 2026-06-13 6:33am MNL recheck: overview still has no missing target rows,
  and `python configure.py --non-matching && ninja` linked from source.

Reverified: 2026-06-13 10:57am MNL — still equivalent. Re-read
`calcFarthestVertex` and `TWalker::bind`, including a second ranged dump for
the middle of the long bind diff. Spider delegation, next-position integration,
airborne velocity clamp, ground/water probes, illegal-ground handling,
wall-record selection, ring-buffer bookkeeping, farthest-vertex probes,
push-off/vector interpolation, and final velocity write still match
behaviorally. Remaining drift is stack/temp layout and helper-owner labels for
`TVec3` operations. Proof passed again with `python configure.py
--non-matching && ninja`, then plain `python configure.py && ninja` restored the
matching config and verified `build/GMSJ01/mario.dol: OK`.
