# Audit verdict: equivalent

- Date: 2026-06-13 11:31pm MNL
- TU: `mario/System/MarNameRefGen_NPC`
- Source: `src/System/MarNameRefGen_NPC.cpp`
- Verdict: `equivalent`

## Reason

Stale-cache refresh: the old `not_equivalent` note is obsolete. Current
`configure.py` already has this row as `Object(Equivalent, ...)`, and the current
overview shows all target text and rodata byte-matching.

Current overview:
- No missing target symbols.
- No nonmatching text functions.
- `TMarNameRefGen::getNameRef_NPC(const char*) const` is byte-exact, so the old
  `MareJellyFish` constructor-argument blocker is fixed in current source.
- Remaining drift is data/owner debt only: `.data-0` vtable/relocation layout,
  rebuilt-only `JDrama::TViewObj` destructor/vtable, rebuilt-only empty
  `TEnemyManager` draw-buffer helpers, infectious string owners, and `.sdata`
  owner drift.

Behavior verdict: equivalent. The factory function now branches, allocates, and
constructs with the same arguments as the target; the residual data/extra items
are source-emission and ownership debt, not referenced behavioral differences.

## Proof

- This tick's `python configure.py --non-matching && ninja` source-linked the
  current `Equivalent` set successfully.
- `python configure.py && ninja` restored the normal matching config and passed
  with `build/GMSJ01/mario.dol: OK`.
