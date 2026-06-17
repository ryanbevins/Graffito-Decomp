# Audit verdict: equivalent

- Date: 2026-06-13 11:41pm MNL
- TU: `mario/Enemy/SleepBossHanachan`
- Source: `src/Enemy/SleepBossHanachan.cpp`
- Verdict: `equivalent`

## Reason

All target symbols are present. All functions match exactly except
`TNerveSBH_Fall::execute(TSpineBase<TLiveActor>*) const` and
`TSleepBossHanachan::startFall(float, float, float)`, and both residual diffs
are codegen-class: stack-frame/slot offsets plus local singleton/vtable label
drift from extra weak/header-owned symbols. The operation order, calls, stores,
branch structure, and nerve transitions are aligned.

Extra symbols are weak/header helpers and infectious-string data only; there are
no missing target symbols and no extra startup constructors.

## Proof

- `python tools/decomp-diff.py -u mario/Enemy/SleepBossHanachan`
- `python tools/decomp-diff.py -u mario/Enemy/SleepBossHanachan -s missing`
- `python tools/decomp-diff.py -u mario/Enemy/SleepBossHanachan -s extra`
- `python tools/decomp-diff.py -u mario/Enemy/SleepBossHanachan -d "TNerveSBH_Fall::execute" --no-collapse`
- `python tools/decomp-diff.py -u mario/Enemy/SleepBossHanachan -d "TSleepBossHanachan::startFall" --no-collapse`
- `python configure.py --non-matching && ninja` passed earlier this tick after
  the `MarDirectorInitECT` promotion.
- `python configure.py && ninja` passed earlier this tick after the
  `MarDirectorInitECT` promotion.
