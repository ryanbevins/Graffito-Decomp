# mario/System/CardManager

## Verdict
equivalent — 2026-06-16 12:08am MNL

Source-linked proof passed after fixing one real behavior mismatch in
`readOptionBlock_`: criteria slot 0 must clear the option sector only when its
state is `STATE_EMPTY`, matching the target's `cmpwi ..., 1` path. With that
fixed, the remaining non-exact rows are byte/codegen debt:
helper-boundary/inlining differences for private workers and `TCardSector`
helpers, stack/register drift, `setCardStat_` loop-vs-unrolled SDK macro shape
for clearing icon slots 2..7, anonymous `@1431/@1411/@1210` labels vs named
dummies, and source-owned `JSUIosBase` weak/vtable residue.

Proofs:
- `python configure.py --non-matching && ninja` linked with
  `Object(Equivalent, "System/CardManager.cpp")`.
- `python configure.py && ninja` passed `build/GMSJ01/mario.dol: OK`.
