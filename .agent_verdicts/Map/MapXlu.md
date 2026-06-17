# mario/Map/MapXlu

Verdict: equivalent
Time: 2026-06-13 6:32am MNL

## Verdict
equivalent

## Date
2026-06-12 5:31am MNL

## Reason
All target symbols are present and both nonmatching functions are codegen-only:

- `TMapXlu::changeXluJoint(int)`: same bounds check, same root-child `sit()`
  loop, same indexed child `stand()` loop, same return values. Residue is only
  target-larger frame (`0x90` vs `0x88`) and saved-register offsets.
- `TMapXlu::changeNormalJoint()`: same root-child `stand()` loop and nested
  indexed child `sit()` loop. Residue is only frame size (`0x88` target vs
  `0x98` build) and saved-register offsets.

`python configure.py --non-matching && ninja` linked cleanly after promoting the
TU.

2026-06-13 6:32am MNL recheck: overview still has no missing target rows, and
`python configure.py --non-matching && ninja` linked from source.

2026-06-13 10:02am MNL recheck: full `--no-collapse` diffs still show only
frame-size/saved-register offset residue. `changeXluJoint` keeps the same
index bounds check, root-child `sit()` loop, selected joint-child `stand()`
loop, and return values. `changeNormalJoint` keeps the same root-child
`stand()` loop and nested child `sit()` loop. Source unchanged since the
10:00am proof batch, which passed both `--non-matching` source link and normal
`mario.dol: OK`.
