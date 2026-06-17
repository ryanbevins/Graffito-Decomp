# MarioUtil/MathUtil Audit

Verdict: equivalent  
Updated: 2026-06-14 3:23pm MNL

Implementation pass `65fe3b69` fixed the real behavior blockers from the
previous audit:

- `matan(float, float)` now uses the atan-table denominator from the larger
  absolute component in every octant and applies the target quadrant transforms
  (`0x4000 -`, `0x8000 -`, `0xc000 -`, and signed wrap cases). This removed the
  source TODO and moved the function `95.9% -> 98.4%`.
- `GetAtanTable(float, float)` is now `static inline`, so the target-absent 92B
  source text helper no longer emits.
- `MsGetRotFromZaxis(const TVec3f&)` now matches the target pitch edge cases:
  normalized `axis.y == 1.0f` returns `-90.0f`, `axis.y == -1.0f` returns
  `90.0f`, and the sqrt helper preserves the original argument when the
  computed horizontal length squared is not positive. The function moved
  `96.7% -> 97.0%`.

Audit verdict:
- `matan(float, float)` now uses the target southwest quadrant spelling
  `GetAtanTable(...) + 0xC000` instead of the equivalent low-16-bit
  `-0x4000` form. The function is `99.0%`; remaining diffs are local data base
  label ownership, operand coloring, and sign/zero-extension spelling that does
  not alter the declared `s16` angle result.
- `MsMtxSetXYZRPH(float(*)[4], float, float, float, short, short, short)` is
  still `56.5%`, but direct target-asm review shows the same R/P/H sin-cos
  formulas and translation stores as source. The diff is scheduling/register
  residue around table loads, store order, and fused multiply-add ordering, not
  a behavior gap.
- `MsGetRotFromZaxis(const TVec3f&)` is `97.0%`; target and source both zero the
  result, normalize a local axis copy, handle `axis.y == +/-1.0f`, preserve
  non-positive horizontal length in the sqrt helper, call `matan` for X/Y
  rotations, and return the same `TVec3` result. Remaining residue is stack
  frame/slot layout, FPR coloring, and const-pool label numbering.
- `python tools/decomp-diff.py -u mario/MarioUtil/MathUtil` reports no missing
  or extra text helpers.
- Anonymous target data rows `@1210`, `@1411`, and `@1431` still appear as
  missing against source-owned `dummy1210`, `dummy1411`, and `dummy1431`. Their
  bytes and layout match; this is local-label ownership debt, not behavior.
- Proof: `python configure.py --non-matching && ninja` linked with MathUtil
  sourced, then `python configure.py && ninja` restored normal config and
  passed `build/GMSJ01/mario.dol: OK`.
