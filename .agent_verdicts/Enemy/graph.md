# Enemy/graph audit

Verdict: `equivalent`

Checked 2026-06-14 8:31am MNL in AUDIT mode.

`mario/Enemy/graph` is behaviorally equivalent and links from source under
`--non-matching`.

Proof:

- `python configure.py --non-matching && ninja` linked `mario.elf` and built
  `mario.dol` with `Enemy/graph.cpp` sourced.
- `python configure.py && ninja` then restored the normal matching config and
  passed `build/GMSJ01/mario.dol: OK`.

Audit notes:

- Fixed one real behavior bug before promotion:
  `TGraphWeb::getRandomButDirLimited` now returns the sampled result whenever
  the accepted-candidate count is positive. The prior `result > 0` test was
  wrong for valid selected node index `0`; target checks the count.
- Remaining `.text` residue is codegen-class: stack/frame size, stack slot
  placement, local vector expression shape, loop lowering, register/FPR
  coloring, and anonymous constant label numbering.
- Remaining data residue is local owner/name debt, not behavior: target has
  anonymous `@1431/@1411/@1210` leading dummy data while source has equivalent
  `dummy1431/dummy1411/dummy1210` bytes. The rebuilt object has no undefined
  references to the target local labels.
