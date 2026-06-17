## Verdict: equivalent

Date: 2026-06-13 11:27pm MNL

Stale-cache refresh for current `Object(Equivalent, ...)`.

Current overview:
- `__num2dec` is byte-exact.
- All target rodata symbols are present and exact.
- No missing or extra symbols.
- The only current diff is `.sdata2` order: both objects contain the same five
  double constants (`0.0`, `1.0`, `2^52`, `0.1`, `10.0`), but the rebuilt
  object orders `2^52` after `0.1`/`10.0`.

Behavior verdict: equivalent. The exact text relocates to the rebuilt local
constants, so the constant-pool order drift is byte debt only.

Proof:
- This tick's `python configure.py --non-matching && ninja` source-linked the
  current `Equivalent` set successfully.
- `python configure.py && ninja` restored the normal matching config and passed
  with `build/GMSJ01/mario.dol: OK`.
