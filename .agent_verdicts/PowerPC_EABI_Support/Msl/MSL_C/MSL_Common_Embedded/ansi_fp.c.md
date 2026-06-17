# MSL Common Embedded ansi_fp.c

Verdict: equivalent
Date: 2026-06-13 9:26pm MNL

Unit: `mario/PowerPC_EABI_Support/Msl/MSL_C/MSL_Common_Embedded/ansi_fp`
Source: `src/PowerPC_EABI_Support/Msl/MSL_C/MSL_Common_Embedded/ansi_fp.c`
Classification: `Object(Equivalent, ".../ansi_fp.c")`

## Review

- The only text symbol, `__num2dec`, is byte-identical.
- All target-visible rodata objects are byte-identical.
- Residual `.sdata2` drift is local constant ownership/label debt only; there
  are no behavior-bearing code diffs.

## Validation

- Shared proof from this tick: `python configure.py --non-matching && ninja`
  linked successfully with current `Equivalent` rows.
- Normal `python configure.py && ninja` passed with `build/GMSJ01/mario.dol:
  OK`.
