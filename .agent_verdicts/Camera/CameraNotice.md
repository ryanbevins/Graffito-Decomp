# Camera/CameraNotice audit

Verdict: equivalent
Date: 2026-06-15 6:09am MNL
Unit: `mario/Camera/CameraNotice`

Reason:

- Strict AUDIT recheck found one remaining real behavior bug before
  certification: `getNoticeActor_()` target passes the candidate `distSq` as
  the fourth `MsIsInSight()` argument and the `0x68` camera parameter as the
  fifth argument; source had them reversed. Fixed that order.
- Reviewed all non-exact text functions after the fix:
  `getNozzleTopPos_()`, `ctrlLButtonCamera_()`,
  `calcNoticeTargetYrot_()`, `getNoticeActor_()`, and `setNoticeInfo()`.
  Remaining diffs are codegen/data-owner debt: frame/register/FPR allocation,
  vector helper-boundary choices, branch layout, unrolled loop register shape,
  and local rodata label names.
- `execNoticeOnOffProc_()` is byte-identical.
- The missing `@1490`/`@1526` rows are the same bytes as the source-owned
  `dummyMactorStringValue1` and `SMS_NO_MEMORY_MESSAGE`; source-linking is not
  blocked.

Proof:

- `git diff --check` passed.
- `python configure.py --non-matching && ninja` linked successfully with
  `CameraNotice.cpp` sourced.
- `python configure.py && ninja` restored matching config and passed
  `build/GMSJ01/mario.dol: OK`.
