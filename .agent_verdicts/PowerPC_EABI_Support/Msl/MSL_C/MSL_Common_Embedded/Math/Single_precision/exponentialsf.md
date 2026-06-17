# PowerPC_EABI_Support/Msl/MSL_C/MSL_Common_Embedded/Math/Single_precision/exponentialsf.c

Verdict: equivalent
Date: 2026-06-13 9:24am MNL

Reviewed `powf` and `expf`. Both keep the same special-case gates, exponent
range handling, table lookups, polynomial evaluation, sign handling, and final
NaN/inf/overflow behavior. Drift is stack frame size, local temp/FPR allocation,
equivalent polynomial scheduling, and local-static rodata label ownership
(`__log2e_m1$localstatic0$__log2f(float)` versus the source local-static name).

Proof:
- `python configure.py --non-matching && ninja`
- `python configure.py && ninja` verified `build/GMSJ01/mario.dol: OK`

2026-06-13 8:49pm MNL recheck: verdict still `equivalent`.
`powf` and `expf` diffs are still codegen-class only: same range gates,
integer exponent extraction, table-index math, polynomial terms, sign handling,
fpclassify cases, NaN/inf/zero returns, and overflow/underflow behavior.
Remaining drift is frame size, stack slots/FPR allocation, equivalent
polynomial scheduling, and local-static rodata label ownership
(`__log2e_m1$localstatic0$__log2f(float)` versus source local-static name).
No current rebuilt object has an undefined reference to the local helper/static
names. Fresh proof passed: `python configure.py --non-matching && ninja`, then
`python configure.py && ninja` verified `build/GMSJ01/mario.dol: OK`.
