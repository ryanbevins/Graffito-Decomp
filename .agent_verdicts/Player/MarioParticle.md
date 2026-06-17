# Audit: mario/Player/MarioParticle

Verdict: `equivalent`

Date: 2026-06-15 3:37pm MNL
Mode: AUDIT

## Reason

`MarioParticle.cpp` source-links and has no missing target rows. Remaining
non-exact text diffs are behavior-neutral: stack/register placement, argument
setup order, helper-boundary choices, local label ownership, and duplicated
target branch layout in `initParticle()`.

Reviewed low-score rows:
- `TWarpInCallBack::execute()` computes the same
  `((particle >> 2) & 0x3f) * 0.0625f + 1.0f` random scale, multiplies the
  emitter warp direction by `unk468`, `mActionTimer`, and that scale, then adds
  the result to `particle->unk14`. Raw asm confirms the target calls
  `TVec3<f32>::scale`; decomp-diff's local call label is address drift.
- `swimmingBubbleEffect()` has the same `isMario()` / flag guards, mouth bubble
  callback path, matrix set, lower-depth test, and body-bubble call. Raw asm
  confirms the lower-depth call is `bubbleFromBody__6TMarioFv`; the
  decomp-diff `bubbleFromMouth` label is address drift.
- `blurEffect()` emits ID `0x10e` bound to the center animation matrix; target
  inlines the matrix lookup while source calls `getCenterAnmMtx()`.
- `toroccoEffect()` computes the same distance from `mPosition` to
  `mToroccoPos`, emits IDs `0x11f` and `0x120`, and stores wind/spark child
  spawn rates from the same parameter fields.
- `emitFootPrintWithEffect()`, `bubbleFromBody()`, `bubbleFromMouth()`,
  `inOutWaterEffect()`, `frontSlipEffect()`, `strongTouchDownEffect()`,
  `emitSweat()`, `moveParticle()`, and the small emitter helpers preserve the
  same conditions, emitter IDs, positions/matrices, and stores.

Data residue is also byte debt: `cParticleFileNames` points at the same three
matched strings, while `.rodata`/`.data`/`.sdata2` mismatches are label and
extra infectious-owner drift. Extra source rows are unreferenced weak/list
destructors, callback no-op methods, local pointer strings, and an inlined
`TVec3<f32>::scale` owner; none create undefined source-link requirements.

## Proof

Passed:

```sh
python configure.py --non-matching && ninja
python configure.py && ninja
```

The normal build ended with `build/GMSJ01/mario.dol: OK`.
