# mario/JSystem/JAudio/JASystem/JASBNKParser

Verdict: equivalent
Time: 2026-06-13 6:29am MNL

## Verdict
equivalent

## Date
2026-06-12 5:33am MNL

## Reason
All target symbols are present. The sole nonmatching function,
`JASystem::BNKParser::createBasicBank(void*)`, is behaviorally identical: the
diff shows the same bank allocation, instrument/oscillator/effect/keymap loops,
drum-set parsing, sample table copies, velocity-region writes, and final heap
usage accounting.

Residue is codegen-only:

- target frame is larger (`0x108` vs `0xc8`) with shifted saved GPR/FPR and
  loop-counter stack slots;
- local template/static symbols are labeled differently in objdiff because of
  extra weak/local symbol ownership, but the instruction stream and call
  structure match.

`python configure.py --non-matching && ninja` linked cleanly after promoting the
TU.

2026-06-13 6:29am MNL recheck: overview still has no missing target rows, and
`python configure.py --non-matching && ninja` linked from source.

2026-06-13 9:57am MNL recheck:
- Current overview still has no missing target symbols.
- Re-read the full `--no-collapse` diff for
  `JASystem::BNKParser::createBasicBank(void*)`. It still performs the same
  bank allocation, instrument loops, oscillator table conversion/copy, effect
  setup, key/velocity-region parsing, drum-set parsing, and final heap-usage
  accounting. The apparent call-name changes are owner/label drift around local
  template helpers and extra source-owned helper bodies, not different calls.
- Residue remains frame/stack-slot size, saved-register/FPR placement, and
  local label ownership. Proof batch passed: `python configure.py
  --non-matching && ninja`, then `python configure.py && ninja` with
  `mario.dol: OK`.

2026-06-13 1:21pm MNL recheck: verdict remains `equivalent`. Current overview
still has no missing rows. Full diff for `createBasicBank(void*)` keeps the
same bank allocation, 0x80 instrument pass, oscillator reuse/copy logic,
Rand/Sense effect setup, key/velocity region parsing, 12 drum-set pass, PER2
release/volume handling, and final `sUsedHeapSize` accounting. The misleading
call labels around `getOscTableEndPtr`, `findOscPtr`, and local
`JSUConvertOffsetToPtr` helpers are symbol-owner attribution; branch opcodes
and subsequent dataflow remain aligned. Proof rerun passed with
`python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` verified `build/GMSJ01/mario.dol: OK`.
