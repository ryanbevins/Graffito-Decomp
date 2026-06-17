# MSL Single precision exponentialsf.c

Verdict: equivalent
Date: 2026-06-13 9:31pm MNL

Unit: `mario/PowerPC_EABI_Support/Msl/MSL_C/MSL_Common_Embedded/Math/Single_precision/exponentialsf`
Source: `src/PowerPC_EABI_Support/Msl/MSL_C/MSL_Common_Embedded/Math/Single_precision/exponentialsf.c`
Classification: `Object(Equivalent, ".../exponentialsf.c")`

## Review

- `expf` implements the same behavior as target: overflow returns `_inf`,
  underflow returns `0.0f`, then the normal path builds the same integer scale,
  indexes `__two_to_log2e_m1_tI`, evaluates the same `__exp_to_x` polynomial,
  and multiplies table value by scale and polynomial.
- `powf` implements the same positive, negative, and zero-base cases as target.
  Positive base returns `two_to_x(exponent * __log2f(base))`; negative base
  checks integer exponent parity, returns `_nan` for fractional exponents, and
  applies the sign only for odd integer exponents. The zero/special path uses
  the same local `fpclassifyf` cases for NaN, zero exponent, Inf/NaN exponent,
  finite negative exponent, and final zero return.
- The repeated inlined `__log2f` and `two_to_x` bodies use the same tables,
  bitfield extraction, index adjustment, polynomial constants, overflow/
  underflow thresholds, and return formulas. Residue is frame size, stack slot
  selection, FPR/GPR allocation, equivalent polynomial scheduling, and local
  branch labels.
- Data residue is local-static ownership/name debt only:
  `__log2e_m1$localstatic0$__log2f(float)` is reported missing and
  `__log2e_m1$localstatic0$__log2f` extra with the same 8-byte contents.
  Target-visible tables and scalar constants are otherwise byte-identical.

## Validation

- Shared proof from this tick: `python configure.py --non-matching && ninja`
  linked successfully with current `Equivalent` rows.
- Normal `python configure.py && ninja` passed with `build/GMSJ01/mario.dol:
  OK`.
