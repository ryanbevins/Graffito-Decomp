# JSystem/JAudio/JAInterface/JAIGlobalParameter

Verdict: equivalent
Date: 2026-06-15 12:57pm MNL
Unit: `mario/JSystem/JAudio/JAInterface/JAIGlobalParameter`
Source: `src/JSystem/JAudio/JAInterface/JAIGlobalParameter.cpp`
Classification: `Object(Equivalent, "JSystem/JAudio/JAInterface/JAIGlobalParameter.cpp")`

Reason: secondary safety-net re-verification kept the existing certification.
The overview has no missing target symbols. All target-visible functions and
exported parameter storage match exactly except
`JAIGlobalParameter::setParamSoundOutputMode(unsigned long)` and a small
`.sdata2` local-label/codegen residue row.

Reviewed behavior:
- `setParamSoundOutputMode()` maps input modes `0`, `1`, and `2` to the same
  driver/stream output modes as target.
- It stores the raw low byte of the input value to `JAIBasic::basic->unk14`.
- It calls `JASystem::Driver::setOutputMode()` with the mapped driver mode,
  then `JAInter::StreamLib::setOutputMode()` with the mapped stream mode.

Remaining debt: stack-frame/save-slot size in `setParamSoundOutputMode`,
`.sdata2` label drift, and extra unused setter/getter helper ownership.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored normal config and passed
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.
