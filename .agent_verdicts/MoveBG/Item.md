# mario/MoveBG/Item

Verdict: equivalent
Date: 2026-06-13 5:42pm MNL

Certified `Object(Equivalent, "MoveBG/Item.cpp")`.

The previous blocker was the duplicate `TYoshi::onYoshi()` owner. Current
focused diff shows no `TYoshi::onYoshi()` extra and no missing `.text` rows.
The behavior review from 2026-06-13 3:01am MNL still applies: restored
`TNozzleBox`, `TItemNozzle`, `TEggYoshi`, `TShine`, coin, and base item paths
perform the target state transitions, messages, item/nozzle/Yoshi/shine spawns,
timer updates, demo-camera calls, particle/light setup, and pickup side effects.
Remaining nonmatching functions are codegen-class residue: stack/frame slots,
saved register coloring, local/static data labels, state-block layout, and
helper ownership.

Proof:
- `python configure.py --non-matching && ninja` passed with
  `MoveBG/Item.cpp` source-linked.
- `python configure.py && ninja` then passed with `build/GMSJ01/mario.dol: OK`.

Verdict: needs_impl  
Date: 2026-06-13 3:01am MNL

Behavior review is green enough to treat the source as functionally complete,
matching the existing `state/notes/MoveBG_Item.md` campaign summary: the
restored `TNozzleBox`, `TItemNozzle`, `TEggYoshi`, `TShine`, coin, and base item
paths perform the target state transitions, messages, item/nozzle/Yoshi/shine
spawns, timer updates, demo-camera calls, particle/light setup, and pickup
side effects. Fresh spot diffs for `TEggYoshi::control`, `TShine::calc`, and
`TShine::control` showed codegen-class residue: frame/slot layout, saved
register coloring, shared-label/state-block layout, local static/data base
labels, and helper ownership.

Do not promote yet. `python configure.py --non-matching && ninja` fails to link
after flipping this TU to `Object(Equivalent, ...)`:

```text
multiply-defined: 'TYoshi::onYoshi()' in Item.o
Previously defined in MarioAction.o
```

The next fix is an ownership cleanup for the unintended `TYoshi::onYoshi()`
definition emitted by `MoveBG/Item.cpp` (or the competing owner), then rerun the
same audit promotion. This is a source-link blocker, not a known runtime logic
gap.
