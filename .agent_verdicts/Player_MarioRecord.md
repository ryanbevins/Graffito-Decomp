## Verdict: equivalent

Date: 2026-06-13 11:28pm MNL

Stale-cache refresh for current `Object(Equivalent, ...)`.

Current overview:
- No missing or extra symbols.
- Ten of eleven functions byte-match.
- The only nonmatching function is `TMarioInputReplay::init(unsigned char*)`
  at 99.9%.

Behavior review:
- `TMarioInputReplay::init` clears the same replay state fields, reads the same
  offsets from the input data block, initializes the five
  `TRecordValueManager` pointer pairs, and calls the same reset helpers.
- The visible diff is stack-frame size only (`0x20` target vs `0x18` rebuild).
- `decomp-diff` still mislabels the `init` helper calls because the reset/get
  specializations are laid out in a different order in the rebuilt object. Raw
  target asm and `powerpc-eabi-objdump -dr` relocations for both target and
  rebuilt objects show the same calls in order:
  `reset<f>`, `reset<s>`, `reset<unsigned short>`, `reset<unsigned char>`,
  `reset<unsigned char>`.

Proof:
- This tick's `python configure.py --non-matching && ninja` source-linked the
  current `Equivalent` set successfully.
- `python configure.py && ninja` restored the normal matching config and passed
  with `build/GMSJ01/mario.dol: OK`.
