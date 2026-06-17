# mario/PowerPC_EABI_Support/Msl/MSL_C/MSL_Common_Embedded/ansi_fp

Verdict: equivalent  
Status: equivalent  
Date: 2026-06-13 8:01pm MNL  
Source: `src/PowerPC_EABI_Support/Msl/MSL_C/MSL_Common_Embedded/ansi_fp.c`

## Reason

Re-verified existing `Object(Equivalent, ...)`.

- The only text function, `__num2dec`, is byte-identical.
- All named rodata constants (`bit_values`, `digit_values`, and local double
  constants) are byte-identical.
- Remaining object drift is the 40B `.sdata2` aggregate/label layout shown by
  objdiff, which is data-label byte debt for this already source-linked runtime
  TU, not a functional behavior difference in callable code.

## Proof

- `python tools/decomp-diff.py -u
  mario/PowerPC_EABI_Support/Msl/MSL_C/MSL_Common_Embedded/ansi_fp` shows
  `__num2dec` and rodata at 100%.
- The current tick's `python configure.py --non-matching && ninja` linked all
  `Equivalent` objects from source.
- The current tick's `python configure.py && ninja` restored normal config and
  passed `build/GMSJ01/mario.dol: OK`.
