# Enemy/mameGesso audit

Verdict: `equivalent`

Checked: 2026-06-14 6:28pm MNL in AUDIT mode.

## Result

Certified `mario/Enemy/mameGesso` as behavior-equivalent and promoted
`Enemy/mameGesso.cpp` to `Object(Equivalent, ...)`.

Fixed one additional behavior gap before promotion:

- `TNerveMameGessoGraphJumpWander::execute()` no longer calls
  `walkBehavior(3, 1.0f)` when the current graph goal is reached and the actor
  is airborne. Target computes a read-only ground-type predicate in that path
  and then skips walking; current source skips the walk and leaves the unused
  predicate as byte-debt.

Remaining residue is behavior-neutral:

- Stack/register/FPR allocation and local vector slot drift in the jump,
  thrown, wait, damage, jitabata, reset, and collision paths.
- Helper-boundary and static-label debt around `JMASSin`/`JMASCos`, `sqrt`,
  `TVec3` construction/copy, `theNerve()` accessors, and anonymous data labels.
- Data-section drift from extra weak/vtable/string owners, with no missing
  target symbols and no source undefined reference blocker.

## Verification

- `python tools/decomp-diff.py -u mario/Enemy/mameGesso -s missing` reports no
  symbols.
- `python configure.py --non-matching && ninja` passed and linked the source
  object.
- `python configure.py && ninja` passed with `build/GMSJ01/mario.dol: OK`.
