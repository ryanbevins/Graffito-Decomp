# Enemy/seal Audit

Verdict: equivalent
Time: 2026-06-13 6:33am MNL

## Verdict
equivalent — 2026-06-12 7:24pm MNL

## Reason
`Enemy/seal.cpp` links from source under `python configure.py --non-matching && ninja`
and the normal matching build still verifies `mario.dol: OK`.

Reviewed all six nonmatching text functions:
- `TNerveSealDie::execute`
- `TNerveSealWait::execute`
- `TNerveSealSleep::execute`
- `TSeal::perform`
- `TSeal::receiveMessage`
- `TSeal::init`

All visible behavior matches the target: nerve transitions, animation changes,
particle and sound calls, collision setup/removal, actor management, damage
counter updates, map-collision matrix setup, save-param hit-point initialization,
and spine initialization use the same calls, constants, offsets, stores, and
branch conditions. Remaining diffs are stack frame/slot placement,
callee-saved register coloring, equivalent scheduling, and objdiff label-owner
drift around local singleton guards, infectious strings, `JGadget` iterator
helpers, and weak/base helper rows. No missing target symbols were reported.

2026-06-13 6:33am MNL recheck: overview still has no missing target rows, and
`python configure.py --non-matching && ninja` linked from source.

2026-06-13 11:10am MNL recheck:
- Verdict remains `equivalent`.
- Current overview still has no missing target symbols. Re-read all six
  nonmatching text functions: `TNerveSealDie::execute`,
  `TNerveSealWait::execute`, `TNerveSealSleep::execute`, `TSeal::perform`,
  `TSeal::receiveMessage`, and `TSeal::init`.
- The three nerves still preserve the same animation changes, timers, death /
  wait / sleep transitions, particle/sound side effects, and return values.
- `perform`, `receiveMessage`, and `init` still preserve the same hit/actor
  management, sender-type checks, collision/list insertion, map-collision setup,
  water/damage counter update, and model/collision matrix side effects.
- Remaining differences are frame/stack size, saved-register coloring, static
  list owner labels, and JGadget iterator/temp placement. The 11:08am proof
  batch linked from source with `python configure.py --non-matching && ninja`,
  then normal `python configure.py && ninja` restored the matching build.
