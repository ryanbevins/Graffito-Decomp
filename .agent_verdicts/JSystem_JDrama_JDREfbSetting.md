## Verdict: equivalent

Date: 2026-06-12 1:38am MNL

Reason: no missing or extra artifacts. All GX calls and argument values match.
The remaining diffs are codegen-class: stack-frame size/register allocation,
equivalent zero materialization (`mr r0,r3` after `li r3,0` vs `li r0,0`), and
register choices for the copied clear color / flag temporaries.

Proof: promoted `JSystem/JDrama/JDREfbSetting.cpp` to
`Object(Equivalent, ...)` and `python configure.py --non-matching && ninja`
linked `build/GMSJ01/mario.dol` successfully.

## 2026-06-13 1:34pm MNL

Verdict reverified against today's build proof:

- `python configure.py --non-matching && ninja` linked `mario.elf` and
  `mario.dol`.
- `python configure.py && ninja` linked and checksum passed.
- Re-reviewed both `IssueGXPixelFormatSetting` overloads,
  `IssueGXSetCopyClear`, and `IssueGXCopyDisp`.
- Behavior still matches: pixel format selection, dither eligibility, field
  mode arguments, copy-filter flags, clear/color/alpha/Z state gating,
  copy-source rectangle, y-scale, aligned copy destination width, and
  `GXCopyDisp` clear flag.
- Residual drift is codegen only: frame size, register allocation, and
  equivalent zero materialization (`mr r0,r3` from known-zero `r3` versus
  `li r0,0`).
