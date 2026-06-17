Verdict: equivalent
Time: 2026-06-13 6:33am MNL

Reason: reviewed both nonmatching functions and verified source link with
`python configure.py --non-matching && ninja`; normal `python configure.py &&
ninja` then passed with `mario.dol: OK`.

Reviewed functions:
- `CPolarSubCamera::execSecureView_(short, Vec*)`: behavior matches Mario-back
  angle derivation, near/far CLB interpolation, cosine-gated far contribution,
  `-abs(near*sin + far*cos)` secure-view offset, Mario turn-speed factor,
  0..1 clamp, `CLBChaseDecrease` calls, and X/Z output accumulation.
- `CPolarSubCamera::calcSecureViewTarget_(short, float*, float*)`: same target
  offset computation and output writes without the chase/update tail.

No missing or extra symbols. Remaining residue is stack/register/FPR allocation
and equivalent branch/source-shape differences around the absolute-value math.

2026-06-13 6:33am MNL recheck: overview still has no missing target rows, and
`python configure.py --non-matching && ninja` linked from source.

2026-06-13 10:49am MNL recheck: verdict remains `equivalent`. Re-read the
current diffs for `execSecureView_` and `calcSecureViewTarget_`. Both preserve
the same Mario-back angle derivation, near/far interpolation, cosine-gated far
contribution, absolute secure-view offset, Mario turn-speed factor, clamp,
`CLBChaseDecrease` calls in `execSecureView_`, and X/Z output writes. Remaining
drift is stack/register/FPR allocation and equivalent branch/source shape around
the absolute-value math. Proof refreshed with `python configure.py
--non-matching && ninja`, then normal `python configure.py && ninja` with
`build/GMSJ01/mario.dol: OK`.
