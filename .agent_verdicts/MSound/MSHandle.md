# MSound/MSHandle

Verdict: equivalent
Date: 2026-06-14 01:36am MNL

Certified after the implementation fix for unordered distance checks.
`calcDolby()` and `calcPan()` now both emit the target `fcmpo; cror eq, lt, eq;
bne` branch shape, so positive and unordered/NaN distances take the `MSACos()`
path like the original. Remaining text differences are codegen-class: category
index branch layout, local helper/constant label ownership, FPR/result-clamp
register choices, and an unreferenced local `computeCategoryIdx` helper. Data
drift is local label naming (`@1431/@1411/@1210` versus `dummy*`) and `.sdata2`
constant ownership/order, with matching values. Extra `JSUList` weak
destructors come from the rogue includes needed for `__sinit` and do not block
source-linking.

Proof:
- `python configure.py --non-matching` passed.
- `ninja` linked the source `MSHandle.o` into `build/GMSJ01/mario.dol`.
- Restored normal config with `python configure.py`; normal `ninja` passed with
  `build/GMSJ01/mario.dol: OK`.

Recheck: 2026-06-14 6:50am MNL

Focused `--no-collapse` diffs for `calcDolby()` and `calcPan()` still show the
fixed unordered-distance guard (`fcmpo; cror eq, lt, eq; bne`) before the
`MSACos()` path. The current source has not changed since commit `a490157c`;
the equivalent verdict stands.

Implementation update: 2026-06-14 01:27am MNL

The two known unordered-distance behavior blockers were repaired in
`src/MSound/MSHandle.cpp`: `calcDolby()` and `calcPan()` now use
`(distance <= 0.0f) ? 0.0f : MSACos(...)`, so positive and unordered/NaN
distances take the `MSACos()` path like the target. Normal
`python configure.py && ninja` passed. Focused diffs after the fix:
`calcDolby` 88.2%, `calcPan` 92.1%. Re-audit this TU; the original blockers
below should no longer apply.

Verdict: not_equivalent
Date: 2026-06-12 11:09pm MNL

Do not promote yet. Most rows are close and `__sinit_MSHandle_cpp` now
byte-matches, but two math helpers still have structural branch-condition
differences:

- `MSHandle::calcDolby(const Vec&, float)`: target tests the distance with
  `fcmpo; cror eq, lt, eq; bne`, so the MSACos path runs for positive or
  unordered/NaN inputs. Current source uses ordinary `(param > 0.0f)` and emits
  `ble`, which skips the path for unordered/NaN inputs.
- `MSHandle::calcPan(const Vec&, float, float)`: same `cror ...; bne` target
  pattern versus current `ble` source around the angle/MSACos path.

Other observed diffs in `setSeDistanceVolume`, `setSeDistanceParameters`, and
the category-index expansion look like equivalent branch layout or helper
inlining/register drift, but the two unordered-distance checks are enough to
keep the TU red for the audit sweep.

Notes:
- Existing investigation notes in `docs/MWCC.md` already describe failed source
  rewrites for this pattern; an implementation/investigation pass should find a
  source shape that preserves the target NaN/unordered behavior without forcing
  unwanted inlining.
- Local `@1431/@1411/@1210` versus `dummy1431/dummy1411/dummy1210` rows are
  symbol-name/local-label drift over matching data bytes, not the blocker.
