# mario/MSound/MSoundSE

Verdict: needs_impl
Status: needs_impl
Time: 2026-06-13 5:35am MNL

## Verdict

## Reason
Do not promote. The overview reports missing target ctor/data rows `@1501`,
`@1953`, `@2486`, and `@2939`, plus broad source-only helper ownership in the
random-play and set-sound helpers. Several sound-routing functions remain
substantially nonmatching (`startSoundActorInner`, `startSoundActorWithInfo`,
`MSRandPlay::randPlay`), so this needs implementation/source-shape work before
source-link certification.
