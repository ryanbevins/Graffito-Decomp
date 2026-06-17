# mario/JSystem/JAudio/JASystem/JASTrack

Verdict: not_equivalent
Status: not_equivalent
Time: 2026-06-13 5:35am MNL

## Verdict

## Reason
Do not promote. There are no missing target symbols, but
`JASystem::TTrack::writeRegParam(unsigned char)` is still a structural blocker
at 65.3%. The command/register dispatch compares operations in a different
order from target, with divergent cases for arithmetic, shift, random,
table-load, app-port writes, and later special-register handling.

This is command-interpreter logic, so the differences are behavioral risk, not
register-coloring. Rework `writeRegParam` and recheck `rootCallback` afterwards.
