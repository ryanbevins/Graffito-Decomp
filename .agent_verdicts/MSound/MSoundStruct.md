# MSound/MSoundStruct.cpp

Verdict: `equivalent`

Last rechecked: 2026-06-15 7:50am MNL.

Safety-net recheck: current overview still has no missing rows. The only
nonmatching text rows are the already-reviewed `searchGroup`, the two
`startSoundSetDyna` instantiations, and the `MSSetSoundTL<MSSetSound>`
constructor; remaining drift is branch/register/FPR/helper-owner residue.
Today's full `python configure.py --non-matching && ninja` proof linked this
object from source, and normal `python configure.py && ninja` passed
`build/GMSJ01/mario.dol: OK`.

## 2026-06-13 6:35pm MNL - reverified

Verdict remains `equivalent`.

Current overview still has no missing symbols. The nonmatching functions are the
same four behavior-reviewed rows (`searchGroup`, both `startSoundSetDyna`
instantiations, and the `MSSetSoundTL<MSSetSound>` constructor), with only
branch/register/FPR/helper-owner residue. The `--non-matching` proof from the
same tick linked this object from source as part of the full Equivalent set, and
the follow-up normal build passed `build/GMSJ01/mario.dol: OK`.

## 2026-06-13 9:29am MNL - refreshed

Verdict remains `equivalent`.

Re-read full `--no-collapse` diffs for `searchGroup`, both
`startSoundSetDyna` instantiations, and the `MSSetSoundTL<MSSetSound>`
constructor. The two dynamic-start instantiations still preserve the active
early return, distance calculation, first-start path, random/min-frame gate,
optional group-member gate, start-ID/remapped-distance handling, optional
position copy, `JAIActor` setup, sound start, modulation, old-position/state
writes, volume/pitch interpolation, port-13 fallback, and ring-index advances.
Raw objdump confirmed the pretty diff's group-member lookup label drift: both
target and source call `searchD__31JALListD<16MSSetSoundMember,Ul>FUl`; the
constructor also calls the expected `JALListHioNode` and `JADPrm` helpers.

Verdict: equivalent
Time: 2026-06-12 9:50pm MNL

Build proof:
- `python configure.py --non-matching && ninja`
- `python configure.py && ninja`

No missing symbols.

Reviewed nonmatching functions:
- `JALListGrp<MSSetSoundGrp, unsigned long, MSSetSoundMember>::searchGroup(unsigned long)`
  - Equivalent. The target and source both walk `JALList<MSSetSoundGrp>::smList`, then each group's member list at `0xbc`, compare the member id against the requested sound id, and return the owning group or null.
  - Residue is branch shape/register drift only.
- `MSSetSoundTL<MSSetSound>::startSoundSetDyna(...)`
  - Equivalent. Early active-state return, distance computation, previous-slot probe, first-start path, repeat/min-frame/random gating, optional group/member lookup, actor-position copy, sound start, mod processing, volume/pitch interpolation, port 13 reset, and ring index updates all match behavior.
  - Residue is stack/register/FPR allocation and helper-label ownership drift.
- `MSSetSoundTL<MSSetSound>::MSSetSoundTL(...)`
  - Equivalent. Base list node construction, vtable setup, JADPrm field initialization, float/control defaults, position zeroing, flags, and ring slots match.
  - Residue is stack/register allocation and inline-owner drift.
- `MSSetSoundTL<MSSetSoundGrp>::startSoundSetDyna(...)`
  - Equivalent by the same review as the `MSSetSound` instantiation; same behavioral structure and same codegen-class residue.

Extra symbols are emitted inline/template support and destructor/list owner drift (`JADPrm` constructors, `frameLoopDyna`, `JSUList`/`JSULink` destructors), not missing runtime behavior.
