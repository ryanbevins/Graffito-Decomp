Verdict: equivalent
Time: 2026-06-13 6:32am MNL

Reason: audited the single nonmatching function and verified source link with
`python configure.py --non-matching && ninja`; normal `python configure.py &&
ninja` then passed with `mario.dol: OK`.

Reviewed functions:
- `TSpider::bind(TLiveActor*)`: behavior matches the target's movement
  prediction, airborne velocity/gravity clamp, primary and fallback ground
  probes, illegal-ground kill/grace handling, grounded/airborne flag updates,
  wall-check record construction, no-wall grace behavior, wall-normal dot
  check, sticky-wall record assignment, push-off interpolation, and final
  linear-velocity write. The target/source both retain the same uninitialized
  saved-FPR fallback shape when a wall touch is reported but the normal-dot
  predicate rejects the wall.
- Constructor, destructor, vtable, and data rows are byte-matching.

No missing symbols. The only extra text row is `TVec3<float>::sub` helper-owner
drift and is source-link safe.

2026-06-13 6:32am MNL recheck: overview still has no missing target rows, and
`python configure.py --non-matching && ninja` linked from source.

Reverified: 2026-06-13 10:54am MNL — still equivalent. Re-read the full
`TSpider::bind(TLiveActor*)` diff. Movement prediction, airborne velocity clamp,
ground probes, illegal-ground/grace logic, grounded/airborne flag updates,
wall-check record construction, wall-normal dot test, sticky-wall assignment,
push-off interpolation, and final velocity write still match behaviorally. The
apparent `TVec3::sub` / destructor call-name split is owner-label drift at the
same call site, not a behavior change. Proof passed again with `python
configure.py --non-matching && ninja`, then plain `python configure.py && ninja`
restored the matching config and verified `build/GMSJ01/mario.dol: OK`.
