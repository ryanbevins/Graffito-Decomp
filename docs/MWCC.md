# MWCC — current understanding

_The graffito decomp bot's living theory of how the Metrowerks CodeWarrior compiler
(MWCC GC/1.2.5) behaves. Updated by the bot as it observes the compiler. Read by the
bot at the start of every tick alongside `CLAUDE.md` and `AGENTS.md`._

This file is **not** authoritative — `CLAUDE.md` is. This is where the bot accumulates
its working hypotheses, open questions, and confirmed observations as it works. When a
hypothesis here becomes well-established with multiple independent confirmations, it
should be promoted into `CLAUDE.md` and removed from here.

---

## Structure

Organise the content under exactly these three top-level sections. Don't add new
top-level sections — extend the existing ones. If you want a deeper hierarchy, use
H3 (`###`) inside a section.

- **Settled** — observations confirmed across at least two independent TUs. Each
  entry is a one-paragraph statement of the rule with a citation list (TU/function
  names where it was observed). These are candidates for promotion to `CLAUDE.md`.
- **Hypotheses under investigation** — patterns the bot has noticed but isn't yet
  sure about. Each entry explains the hypothesis and the specific experiment that
  would confirm or refute it.
- **Open questions** — phenomena the bot doesn't understand yet. Each entry is a
  question + the symptom that prompted it. Move to "Hypotheses under investigation"
  when an idea forms.

Within each section, order entries newest-first (prepend, don't append). Old entries
that turned out wrong should be moved to a `## Refuted / wrong turns` section at the
bottom rather than deleted — preserving the dead-ends prevents the bot from re-trying
them in future ticks.

---

## Settled

### Hoist a struct-field read into a local INSIDE a loop, BEFORE a function call, to lock it into a NV-FPR across the call

**Rule:** When the loop body needs `obj.field` *after* a `bl` call but the
field's address is the same each iteration, reading it into a local
**before** the `bl` (still inside the loop) forces MWCC to allocate a
callee-saved FPR (`f31` etc.) for it, so the value survives the call.

Without the explicit local, MWCC re-reads `lfs frV, off(rN)` *after*
the call returns, paying a load per iteration but freeing the NV-FPR.
With the local declared just before the `bl`, MWCC computes the load
*before* the call and keeps the value in `f31`, then stores from `f31`
after the call. This shifts one NV-FPR slot to the field, increasing
the function's NV-FPR count by one (which itself can match or mismatch
the target — verify the count first).

```cpp
// Before — MWCC reloads center.y after cosf returns:
for (int i = 0; i < N; ++i) {
    f32 sinAngle = sinf(angle);
    f32 z        = center.z + drawRadius * sinAngle;
    f32 cosAngle = cosf(angle);            // <-- bl
    f32 x        = center.x + drawRadius * cosAngle;
    GXPosition3f32(x, center.y, z);        // center.y reloaded here
}

// After — MWCC pins center.y into f31 across the cosf bl:
for (int i = 0; i < N; ++i) {
    f32 sinAngle = sinf(angle);
    f32 z        = center.z + drawRadius * sinAngle;
    f32 cy       = center.y;               // <-- hoist BEFORE the bl
    f32 cosAngle = cosf(angle);
    f32 x        = center.x + drawRadius * cosAngle;
    GXPosition3f32(x, cy, z);
}
```

**How to identify the target pattern:**
- Target's loop body has `lfs frV, off(rN)` to a callee-saved FPR
  (f24-f31) *before* the `bl`, and the same `frV` is used *after*
  the call.
- Our build has `lfs frV, off(rN)` *after* the `bl` instead.
- Stack frame includes an extra `stfd/lfd f{V}, off(r1)` save/restore
  for the additional NV-FPR.

**Where observed:**
- `Map/BathWaterManager.cpp::drawCap` — 92.14% → 98.30% by hoisting
  `center.y` into a `cy` local immediately before `cosf` inside the
  outer `for (i < 0x1e)` loop. Target also pre-loads `lfs f31, 0x4(r28)`
  pre-`bl cosf`; ours did `lfs f1, 0x4(r28)` post-call. Hoisting added
  one NV-FPR (f28 was used by target but not us) and matched the loop
  layout. Remaining gap is NV-FPR allocation order (target uses
  f28..f31 in reverse).

### Hoist a scalar pre-computation BEFORE a `lwz`/`stw` integer-copy block

**Rule:** When a function does both (a) a scalar arithmetic expression
involving struct-field loads and (b) an integer-copy `result = src;` of
another struct, putting the scalar expression *first* in source forces
MWCC to interleave its loads/computes with the integer copy. This
matches target's layout when target's prologue mixes
`lfs/fsubs/fmuls` with `lwz/stw`.

The mechanism: MWCC tries to schedule the float load early to hide
the load latency. If source puts the scalar after the copy, MWCC may
still schedule the float load early — but it ends up *between* the
`lwz/stw` pair-sequence (which target does), OR *after* them entirely
(which is what we get with the float expression at source-bottom). The
ambiguity is resolved by the source order: explicit early declaration
locks in the early scheduling.

```cpp
// Before — MWCC schedules unk3C load late:
JGeometry::TVec3<f32> result = unk0;            // 3x lwz/stw
f32 t      = (f32)index / (f32)count;
f32 radius = t * (unk3C - height);              // unk3C loaded LATE

// After — fsubs scheduled BEFORE the lwz/stw chain:
f32 radiusFactor = unk3C - height;              // fsubs early
JGeometry::TVec3<f32> result = unk0;            // 3x lwz/stw, interleaved
f32 t       = (f32)index / (f32)count;
f32 radius  = t * radiusFactor;
```

**Where observed:**
- `Map/BathWaterManager.cpp::TBathtubData::getPos` — 88.40% → 92.95%
  by hoisting `f32 radiusFactor = unk3C - height;` to the first
  statement above `JGeometry::TVec3<f32> result = unk0;`. Target's
  prolog interleaves `lfs f0, 0x3c(r4); fsubs f0, f0, f30; lwz r3,
  0x0(r4); ...` while ours had the lwz/stw block come before any
  unk3C load. The radiusFactor local survives until used in `t *
  radiusFactor` later. Caveat: putting the assignment *between*
  result-copy and t/radius does NOT help — must be FIRST.

### Avoid hoisting a member-array base pointer when the loop is unrolled or short

**Rule:** The 2D-array-cast lever (above) is *not* always a win.
When the loop iterates a known small count and MWCC fully unrolls,
or when there is only one access site per iteration, hoisting the
grid pointer into a local can cause MWCC to allocate a register that
target does *not* use. Target uses `this` directly with the full
`+0x20+field` offset folded into the access.

The asymmetry: the 2D cast helps when target's asm shows a single
`+0x4` (field-only) offset, indicating target also has a local. It
hurts when target's asm shows `+0x20`, `+0x24`, `+0x28` etc.
(this-relative including the array base).

**Where observed:**
- `Map/BathWaterManager.cpp::TBathWaterMeshRenderer::calcCoord` —
  first loop on `unk60020` (TVec2): 91.87% → 92.42% by *removing*
  the 2D cast and using `unk60020[x * 0x80 + z].x` directly. Target
  has `addis r5, r5, 0x6` + `stw f1, 0x20(r8)` (this-relative).
  Second loop on `unk20` (TVec3) keeps the 2D cast (regression
  test: removing it drops to 88.87%).

### Use 2D array casts for `arr[i * stride + j]` flat indexing into row-major data

**Rule:** When source has a 1D-declared array member `T arr[N*N]`
but the original code clearly treats it as a 2D `arr[i][j]` grid,
MWCC compiles `arr[i * N + j].field` differently from `arr2d[i][j].field`:

- `arr[i * N + j].field` produces: one combined `mulli (i*N+j)*sizeof(T)`
  followed by `add this + offset` and `lfs/stfs +4(reg)` (only the `.field`
  byte offset folded into lfs).
- `arr2d[i][j].field` produces: two separate `mulli i*0x600` and
  `mulli j*sizeof(T)`, an `add` of both into `this`, and `lfs/stfs
  +0x24(reg)` (the full `&this->arr[0].field` offset folded — base of arr
  in `this` + field offset).

The two-mulli + offset-folding pattern is what the original code emits
when the source was `T arr[N][N]; arr[i][j].field;`. To match without
changing the header type (which may break sibling accesses), cast a
local pointer:

```cpp
JGeometry::TVec3<f32> (*grid)[0x80]
    = (JGeometry::TVec3<f32>(*)[0x80])unk20;
for (int x = 0; x < 0x80; ++x)
    for (int z = 0; z < 0x80; ++z)
        grid[x][z].y = 0.0f;
```

**Caveat:** the local pointer bakes in the `arr`'s base offset (e.g.
`+0x20`), so MWCC folds `.field` only (`+0x4`). For full match parity
target produces `+0x24` because the original source uses arr as a member
(no local). To get the full match, change the header type to 2D. The
local cast alone closes 80-90% of the gap.

**How to apply:**
- Read target asm for the access — if you see two separate `mulli`s and
  `+0xN` offset that includes the array-base offset, the original was 2D.
- Add a local pointer cast `T (*grid)[INNER_DIM] = (T(*)[INNER_DIM])arr;`
- Replace `arr[i * INNER + j]` with `grid[i][j]`

**Where observed:**
- `Map/BathWaterManager.cpp::TBathWaterMeshRenderer::getHeight` —
  82.76% → 86.44% with grid cast alone, then 90.92% with an additional
  `cell = &grid[ix][iz]` local pointer.
- `Map/BathWaterManager.cpp::TBathWaterMeshRenderer::clearHeightMap` —
  84.96% → 97.68% combining 2D nested loops with the cast.
- `Map/BathWaterManager.cpp::TBathWaterMeshRenderer::makeHeightMap` —
  70.66% → 92.41% via grid cast applied to the inner-loop store.
- `Map/BathWaterManager.cpp::TBathWaterMeshRenderer::calcCoord` —
  88.87% → 91.87% via grid casts on both unk20 (TVec3) and unk60020 (TVec2).

### Hoist `param->field.member` to a local pointer when used across multiple `bl` calls

**Rule:** When a function uses the same `param->field.member` address
twice across `bl` calls (e.g. as a first argument to two different
library functions), MWCC re-emits `addi rN, rParam, OFFSET` at each
use site by default. Hoisting the address into a typed local pointer
causes MWCC to pre-compute the offset once into a callee-saved
register (target's `addi r31, r30, 0x74`) and reuse via `addi r3, r31, 0x0`
or `mr r3, r31` at subsequent sites.

This is closely related to the existing "Reference-typed arguments"
rule but applies to plain pointer arguments — no reference type
needed. The trigger is **multiple bl calls** consuming the same
field-address as a leading argument.

**How to apply:**

```cpp
// Before (recomputes addi rN, rParam, OFFSET twice):
C_MTXPerspective(param_2->mProjMtx.mMtx, fovY, aspect, n, f);
...
GXSetProjection(param_2->mProjMtx.mMtx, GX_PERSPECTIVE);

// After (single addi at top, reused via mr):
MtxPtr projMtx = param_2->mProjMtx.mMtx;
C_MTXPerspective(projMtx, fovY, aspect, n, f);
...
GXSetProjection(projMtx, GX_PERSPECTIVE);
```

The local must be a typed pointer (`MtxPtr`, `T*`, `T&`) so MWCC keeps
it in a register rather than spilling each component. If the field
is accessed only once between the two `bl` calls AND nowhere else,
MWCC may already hoist it, so the lever applies mainly when there's
intervening computation that would otherwise cause re-materialization.

**Where observed:**
- `JSystem/JDrama/JDRCamera.cpp::TLookAtCamera::perform` —
  85.12% → 99.73% by hoisting `param_2->mProjMtx.mMtx` to a single
  `MtxPtr projMtx` local. Diff revealed target's `addi r31, r30, 0x74`
  + `mr r3, r31` + later `addi r3, r31, 0x0` pattern. Adds one extra
  callee-saved register slot.
- `JSystem/JDrama/JDRCamera.cpp::TOrthoProj::perform` — 85.58% → 99.74%
  via identical fix. Both functions share the structure
  `C_MTX*(projMtx, ...); ...; GXSetProjection(projMtx, MODE);`.

### Multi-char tag literals: `+ i` not `+ (i << 24)` for per-iteration suffix increment

**Rule:** When iterating over pane/tag names that differ only in
the last character (e.g. `ss_1`, `ss_2`), the source should use
`'ss_1' + i` — NOT `'ss_1' + (i << 24)`. MWCC packs 4-char literals
big-endian (`'X1X2X3X4'` = `(X1<<24)|(X2<<16)|(X3<<8)|X4`), so the
last char sits in the LOW byte and `+ i` correctly increments it.

**Symptom:** Diff shows target asm doing a 2-instruction tag build:

```
addis r4, rIdx, 0xHHHH
addi  r4, r4,   0xLLLL
```

(where `(0xHHHH << 16) + 0xLLLL` = `'X1X2X3X4'`). Our build does
3 instructions because of the spurious shift:

```
slwi  r4, rIdx, 24
addis r4, r4,   0xHHHH
addi  r4, r4,   0xLLLL
```

The `(i << 24)` form would correctly increment the FIRST char (high
byte) — producing tags like 'ss_1', 'ts_1', 'us_1', not 'ss_1',
'ss_2', 'ss_3'. The names with shifted high byte rarely exist in
the BLO file, so this is both a codegen AND correctness bug.

**How to apply:** look for source patterns like `'XXXX' + (i << N)`
for N in {8,16,24}; nearly always the correct form is just `+ i`,
unless the iteration genuinely changes the high byte (rare —
typically only for stage-prefix tags like `'0c_1' + (stage << 24)`
in resetScore-like code where the stage digit IS the high byte).

**Where observed:**

- `GC2D/Guide::load` — fixing six per-loop tag patterns (ss_1/sq_1/
  sc_1/sb_1/cu_a/pn00) lifted load 89.57% → 93.70% (+4.13pp).
- Also applies to single-tag stage-prefixed literals where target's
  asm uses `lis r24, 0xHHHH; addi r4, r24, 0xLLLL` with the literal
  not matching naive interpretation of the source's `'XXXX' + (... shift)`.
  See `'mi09' → '01mi'` fix in Guide::load — original source used
  the actual BLO pane name (`'01mi'`, stage-prefixed) directly.

### Combined `clrlwi+slwi` → `clrlslwi` requires unsigned source type

**Rule:** When indexing an array by a u8-returning function result
(e.g. `array[SMS_getShineStage(...)]`), the result variable's
declared type controls the codegen:

- `u8 stage = SMS_getShineStage(...)` → MWCC fuses zero-extend +
  shift into one `clrlslwi r0, r3, 24, 2` (for *4 indexing).
- `s16 stage = ...` → MWCC emits `clrlwi r0, r3, 24; extsh r0, r0;
  slwi r0, r0, 2` (3 insns: zero-extend, sign-extend, shift).

The s16 cast forces an unnecessary `extsh` between the zero-extend
and the shift, breaking the `clrlslwi` fusion.

**How to apply:** use the natural unsigned type for the function's
return value. If the function returns `u8`, store in `u8`. Don't
widen to `s16`/`s32` unless the value is genuinely used signed later.

**Where observed:**

- `GC2D/Guide::perform` case 9 — changing `s16 stage` to
  `u8 stage` shaved 2 insns and lifted match 67.52% → 68.11%.
- See also `feedback_s32_intermediate_vs_u8.md` for the opposite
  case (when storing TO a u8 field, `s32` intermediate beats `u8`).

### Unsigned div magic differs from signed: drop `(u32)` cast to get signed `/10`

**Rule:** Dividing an `int` by a constant 10 emits MWCC's signed
magic `lis 0x6666; addi 0x6667 → mulhw → srawi 2 → srwi 31 → add`.
If the source casts the dividend to `u32`, MWCC uses the unsigned
magic `lis 0xcccd; subi 0x3333 → mulhwu → srwi 3` instead. These
produce identical instruction COUNTS but different opcodes — break
the match for any TU that originally used signed division.

**How to apply:** prefer `int x = ...; int q = x / 10;` over
`u32 x = ...; u32 q = x / 10;` unless the dividend is genuinely
unsigned (e.g., from a u8 zero-extend that could exceed INT_MAX —
extremely rare for index math).

**Where observed:**

- `GC2D/Guide::load` 13-loop — changing `u32 hi = (u32)i / 10;`
  to `int hi = i / 10;` matched target's `lis 0x6666` signed magic
  and gained +1.91pp (93.70% → 95.61%).

### Char-array row pointer: avoid pre-adding the constant struct offset to the base

**Rule:** When a function iterates over a fixed-stride per-stage
(or per-row) block of bytes within `this`, MWCC matches better
when the source uses `(u8*)this + stage * stride` as the row
pointer and accesses fields via literal byte offsets within
the struct — NOT `(u8*)this + base_offset + stage * stride`
with offset-relative access.

**Symptom:** Diff shows target computing the row pointer
without the +0x14 (or whatever the struct's static offset is),
e.g. `add r19, r31, r18` (this + stage*8), then reading via
`lbz r0, 0x15(r19)`. Our build computes `addi rN, rStage, 0x14;
add rM, r31, rN; lbz r0, 0x1(rM)`. Same effective address but
extra `addi` to fold the static offset into the address compute.

**How to apply:**

```cpp
// Before — pre-add the static offset to the base:
u8* stageData = (u8*)this + 0x14 + stage * 8;
... stageData[1] ...;  // shineCount
... stageData[2] ...;  // redCoin
... *(u16*)(stageData + 4) ...;  // deaths

// After — keep the row pointer canonical, fold offset into the access:
u8* stageData = (u8*)this + stage * 8;
... stageData[0x15] ...;  // shineCount  (= 0x14 + 1)
... stageData[0x16] ...;  // redCoin     (= 0x14 + 2)
... *(u16*)(stageData + 0x18) ...;  // deaths (= 0x14 + 4)
```

The C ABI guarantees these two forms produce identical addresses,
but MWCC's address-arithmetic lowering emits a single `add`
instruction for the row pointer plus a load with the full offset,
rather than two `add`s for "row pointer + static base" and a load
with the small offset.

The lever may also affect register allocation around the row
pointer (canonical row pointer can be reused across nested blocks
with offset-relative loads, vs needing to recompute when adding
varying offsets).

**Where observed:**
- `GC2D/Guide::resetScore` — 89.22% → 90.00% (also folded in
  with other lever changes for total 90.93%).
- `GC2D/Guide::changeBotStatus` — combined with the if/else
  inversion below, 45.67% → 95.73%.
- `GC2D/Guide::resetObjects` — combined with the static-inline
  wrapper trick, 73.32% → 89.89%.

### Top-level if/else order: target's `bne label` always means the LATER block

**Rule:** MWCC NEVER swaps the THEN and ELSE blocks. If target
asm shows `bne .L_X` (or `beq .L_X`) at a top-level branch,
where .L_X is LATER in the function body, then the jumped-to
block is the LATER body in source order. Specifically:

- `if (cond) A else B` → compiles to `test cond; beq label_B;
  A; b end; label_B: B; end:` — so the FIRST block emitted
  is the THEN.
- `bne label` jumping FORWARD to a later block means the THEN
  block IS the fall-through (the FIRST block).

When a function has an early-return pattern like
`if (cond) { ... return; } /* rest */`, MWCC emits THEN
(the early-return body) first, then a `b end` to skip over
"rest". If target instead emits "rest" first and the early-
return body LATER, the source must be rewritten as
`if (!cond) { rest } else { early-return body }`.

**How to apply:**

```cpp
// Before — early-return pattern produces THEN-first emission:
if (stageData[0] != 0) {
    /* completed-stage block: lots of code, return */
    ...
    return;
}
/* active-stage block: lots of code */
...

// After — explicit if/else produces correct order:
if (stageData[0] == 0) {
    /* active-stage block: lots of code */
    ...
} else {
    /* completed-stage block: lots of code */
    ...
}
```

Same logic applies to inner branches:
- Target `bne .L_X` where X is the LATER block → source has the
  LATER block as the ELSE.
- Target `beq .L_X` where X is the LATER block → source has the
  LATER block as the THEN (if cond is the negation).

**Where observed:**
- `GC2D/Guide::changeBotStatus` — top-level `if (stageData[0x14]
  != 0)` early-return was inverted to `if (== 0) { active } else
  { completed }`. Combined with three inner-block inversions
  (`if (idx <= 1 || stageData[0x16] == 0) { hide } else if
  (stageData[0x16] == 1) ...`, and `if (idx != 0) { show } else
  { hide }`), changeBotStatus went 45.67% → 95.73%.
- `GC2D/Guide::resetScore` — `if ((u8)redCoinTotal == 0) hide
  else show` matched target's `bne show_block` pattern (the show
  block is the LATER one in the function body).

### Field stores hoisted before a copy-ctor bl: source has assignment BEFORE the local declaration

**Rule:** When target asm shows `stw rVal, OFF(rThis)` field
stores happening BEFORE a `bl copy__7JUTRectFRC7JUTRect` (or any
copy-ctor bl) but our build emits them AFTER the bl, the original
source has the assignments LITERALLY BEFORE the local-variable
declaration that triggers the copy-ctor. MWCC respects source
order in this case.

**Symptom:** Diff shows our build with `bl copy__7JUTRectFRC7JUTRect`
followed by `lwz rA, OFF1(rThis); stw rA, OFF2(rThis); ...`, while
target has the loads/stores first, then the bl. Effective ordering
is reversed.

**How to apply:**

```cpp
// Before (our build defers stores until after the bl):
JUTRect rect1(_218[idx]);   // bl copy
_424 = _1C0[idx];           // ← MWCC schedules these after the bl
_428 = _378[idx];

// After (matches target):
_424 = _1C0[idx];           // stores hoisted before the bl
_428 = _378[idx];
JUTRect rect1(_218[idx]);   // bl copy
```

This is purely a source-order lever — MWCC's scheduler treats the
copy-ctor `bl` as a barrier and never reorders explicit field
stores around it in either direction. The original author chose
the order; you just have to find it.

**Where observed:**

- `GC2D/Guide::appearGuidePane` — moving the `_424 = _1C0[idx]; _428 = _378[idx];`
  assignments BEFORE the `JUTRect rect1(_218[idx]);` declaration took it
  from 95.20% → 98.60%. Diff showed our build doing the stores after the
  first `bl copy__7JUTRectFRC7JUTRect`; target does them before.

### `J2DPane::show()` / `hide()` vs direct `mVisible = true/false` literal assignment

**Rule:** Even though `J2DPane::show()` and `hide()` are inline
methods whose bodies are `mVisible = true;` and `mVisible = false;`
respectively, using the inline-helper form changes MWCC's scheduling
of the surrounding code. Specifically, around an `if/else` whose
branches each end in a `mVisible = literal` assignment that follows
a `bl` (e.g. `unkBC->search(tag)->mVisible = true;`), calling
`show()`/`hide()` produces different scheduling than the direct
assignment.

**Symptom (before):** Direct `unkBC->search(tag)->mVisible = true;`
emits `bl search; li r0, 0x1; stb r0, 0xc(r3)`. Target shows the
same sequence — but the surrounding stack frame size, branch
offsets, and constant-pool offsets differ. Match drifts ~5pp.

**How to apply:**

```cpp
// before:
if (flag) {
    unkBC->search(tag)->mVisible = true;
} else {
    unkBC->search(tag)->mVisible = false;
}

// after (use the inline helpers):
if (flag) {
    unkBC->search(tag)->show();
} else {
    unkBC->search(tag)->hide();
}
```

Both forms produce the same final `li r0, 0x1; stb r0, 0xc(r3)`
sequence at the call site, but `show()/hide()` is what the original
authors used and MWCC produces matching scheduling of the
surrounding code (loop counter, prologue/epilogue offsets, branch
targets) when the source uses the helpers.

**Where observed:**

- `GC2D/Guide::placeMario` — switching the 8-iteration loop body
  from `pane->mVisible = true/false` to `pane->show()/pane->hide()`
  jumped match 76.89% → 82.02% (~5pp). Loop body asm is byte-identical
  in both forms; the gain comes from scheduling drift in adjacent
  basic blocks settling into target's layout.

### Reference-typed arguments: bind to a local reference BEFORE the call to control argument-eval order

**Rule:** When a function takes a reference-typed argument
(`T&` / `const T&`) and our source passes a struct field of
`this` directly (e.g. `foo(this->field)`), MWCC may emit a
spurious "save `this` to a callee-saved register first" sequence
before computing the reference address. Binding the reference
to a local **before** the call forces MWCC to compute the
reference's address from `r3` (`this`) directly, in the order
the original code expects.

**Symptom:** Diff shows our build doing `addi rN, r3, 0x0`
(save this) then `addi rM, rN, OFFSET` (compute field-ref via
saved this), while target does `addi rM, r3, OFFSET` directly
(compute field-ref from this in one shot, before clobbering r3
with the first argument).

**How to apply:**

```cpp
// before:
foo(param_1, this->matrix_field, this->vec_field);

// after (when foo signature ends with a `T&` reference arg):
JGeometry::TVec3<f32>& v = this->vec_field;
foo(param_1, this->matrix_field, v);
```

For functions with **multiple** trailing reference arguments,
bind each one in the order they appear (reverse-bind also
works — the order of `T& ref = field;` statements doesn't seem
to matter; MWCC computes addresses from `r3` in argument-list
order at the call site).

**Where observed:**

- `JSystem/JPABaseEmitter::setGlobalRTMatrix` (one
  `TVec3<f32>&` arg) — 91.17% → 100%. Binding `v = unk160` to
  a local reference eliminated the `addi r5, r3, 0x0` (save
  `this`) instruction.
- `JSystem/JPABaseEmitter::setGlobalSRTMatrix` (two
  `TVec3<f32>&` args) — 90.15% → 100%. Same fix applied to both
  the `unk154` and `unk160` parameters.

### `TVec3::zero()` writes z,y,x (descending) — `TVec3::set(0,0,0)` writes x,y,z (ascending)

**Rule:** The inline `void zero() { x = y = z = 0.0f; }` in
`JSystem/JGeometry/JGVec3.hpp` uses C++ right-to-left chained
assignment, which MWCC compiles to **descending** stfs order
(0xC, 0x8, 0x4 — i.e., z first, then y, then x). In contrast,
`set(0.0f, 0.0f, 0.0f)` (the 3-arg template `set<TY>`) assigns
x = x_; y = y_; z = z_; in source order, producing **ascending**
stfs (0x4, 0x8, 0xC).

**How to apply:** look at the target diff for the TVec3 store
group. If the asm hits offsets in ascending order (lowest first),
the source used `set(0,0,0)` or explicit `x = 0; y = 0; z = 0;`.
If descending, the source used `zero()`. The choice may differ
between TUs and even between functions within a TU — don't
mass-rewrite, fix only where the diff demands.

**Where observed:**
- `Strategic/TLiveActor::ctor` — three TVec3 stores in ascending
  order; changing `mLinearVelocity.zero(); mAngularVelocity.zero();`
  to `set(0.0f,0.0f,0.0f)` brought 99.88% → 99.93% (rest is
  unrelated stack phantom).
- `Strategic/TLiveActor::moveObject` — descending order in target;
  source uses `zero()` and matches at 100% — confirming zero()
  produces descending.
- `Enemy/beam::TConeBeam::ctor` — descending in target; source
  uses `zero()`, matches at 100%.

### Excess field init: drop the source assignment when target asm skips that offset

**Rule:** The inverse of the missing-field-init pattern below.
If target asm stores zero/0.0f at a contiguous block of offsets
but **skips** one specific offset that our source initializes
(via initializer list or body assignment), **delete that
assignment**. The original source left the field uninitialized.

This often surfaces when a class has a declared `int`/`float`
field at a "natural" zero-init position but the original code
didn't explicitly assign it (despite our intuition saying it
should).

**Where observed:**
- `Enemy/bossManta::TBossMantaManager::ctor` — diff showed our
  build had an EXTRA `stw r0, 0x84(r30)` (the
  `mShadowAlphaTimer` field, initialized as `mShadowAlphaTimer(0)`
  in our init list). Target asm wrote 0 at 0x7c, 0x80, 0x88, 0x8c
  (skipping 0x84). Removing `mShadowAlphaTimer(0)` from the init
  list took it 97.66% → 100%.

### Derived class adds own fields at parent's "next" offset — declare and init

**Rule:** Subclasses that inherit from a parent ending at offset
`OFFSET` will store their *own* fields starting at `OFFSET`. If
target asm stores 0 at those offsets in the derived ctor (after
the parent ctor's bl), but the derived class has no fields
declared in our header, add them.

This differs from the "missing-field-init" pattern in that the
fields don't exist in our header yet — they must be **added** to
the class definition, not just initialized.

**Where observed:**
- `MoveBG/Item::TEggYoshi::ctor` — TEggYoshi inherits from
  TMapObjGeneral (size 0x148). Target ctor stored 0 at 0x148,
  0x14C, 0x150 after `bl __ct__14TMapObjGeneralFPCc`. Adding
  3 `u32 unk148/14C/150` fields to TEggYoshi's class + init body
  took ctor 77.62% → 100%. (Note: these are *different* fields
  from TItem's same-offset fields; TEggYoshi inherits directly
  from TMapObjGeneral, not TItem.)

### Empty-body ctor with declared fields — fields MUST be initialized in body or init list to match

**Rule:** An empty ctor body (`Foo::Foo() { }` or `Foo::Foo(...) : Parent(...) { }`)
will NOT initialize declared member fields. Target asm consistently shows
explicit `stw`/`stfs` stores at field offsets after the parent ctor call —
those stores require **explicit assignments in the source**. Skipping
fields (even if the class declares them) is a top cause of 60-90% near-matches
on small ctors.

When the field is at offset 0 (i.e., before the vtable / before any inherited
fields), it must be in the **initializer list** to fire before parent member
inits (e.g., before an embedded TList's ctor). Body assignments are too late
in this case.

When the field is at higher offset, body assignments are fine — MWCC emits
them in source order.

Heuristic for what to add: in the diff, find missing stw/stfs/stb instructions
at offsets that map to declared member fields. The source code IS missing
those assignments. Check the field type to pick the right init value:
- ptr/int slot stored as `0`: `field = nullptr` or `= 0`
- `lfs f0, @const_zero` stored as 0.0f: `field = 0.0f`
- Non-zero literal (like `li r0, 0x3`): use that literal value
- Stored argument register value: use the constructor parameter

**Where observed:**
- `Enemy/hamukuri::TBossDangoHamuKuri::TBossDangoHamuKuri` — `unk238(0)`
  added to init list, ctor 97.22% → 100% and inlined caller
  `createEnemyInstance` 91.76% → 100%.
- `M3DUtil/MActorData::MActorAnmData::MActorAnmData` — `unk0` (offset 0,
  before TList unk1C) added to **init list**, 83.88% → 99.88%. Body
  assignment was wrong because TList ctor must run AFTER unk0 = 0.
- `MSound/MAnmSound::MAnmSound` — inherited field `mData = nullptr`
  (offset 0x90 in JAIAnimeSound), 84.71% → 100%.
- `Animal/fishoid::TRealoid::TRealoid` — also revealed pattern of `mActors`
  init that was wrong; real ctor was `onLiveFlag(0x38)`, 90.86% → 100%.
- `MoveBG/Item::TCoinRed::TCoinRed` — 3 new f32 fields at 0x158-0x160
  (only on TCoinRed, not TCoin / TFlowerCoin), 88.89% → 96.67%.
- `Map/PollutionLayer::TPollutionLayerWallBase::ctor` — unkAC, unkB0,
  83.33% → 100%.
- `MoveBG/MapObjTrap::TLampTrapSpike::ctor` — unk138=3, unk13C/140=0,
  74.55% → 100%.
- `Animal/Butterfly::TButterfloid::ctor` — 3 fields including
  `unk158 = count` (ctor arg), 60.12% → 100%.
- `MoveBG/MapObjMare::TCogwheel::ctor` — same fields all initialized,
  but **field-init order** matters: reordering body to put scalar
  fields BEFORE the TVec3::zero() calls 94.06% → 100%. (TVec3 zero
  produces multiple stfs in source-line position; non-TVec3 stores
  hoisted by MWCC if before/after a function call differs.)

### `static const GXColor c = {...}` + `GXColor local = c;` preserves the intermediate stash–reload pattern

**Rule:** When passing a small (≤ 4-byte) struct by value through a stack
argument slot, MWCC emits a stash-then-copy through a local stack temp —
**but only when the source is a real symbol load**, not a folded
immediate.

```cpp
static const GXColor cAmbColor = { 0xFF, 0xFF, 0xFF, 0xFF };
GXColor ambColor = cAmbColor;             // local
GXSetChanAmbColor(GX_COLOR0A0, ambColor); // by-value pass
```

lowers to:

```
lwz r0, cAmbColor@sda21(r0)   ; load const
stw r0, 0x54(r1)               ; store to local
lwz r0, 0x54(r1)               ; reload
stw r0, 0x50(r1)               ; store to arg slot
bl GXSetChanAmbColor
```

The redundant stash+reload at 0x54 is the by-value copy of the local
into the arg slot. MWCC does NOT eliminate the dead store to 0x54
(stores aren't reordered/elided), so this remains visible.

**The anti-pattern:** writing the constant as a u32 and casting through:

```cpp
static const u32 cAmbColor = 0xFFFFFFFFu;
GXColor ambColor;
*(u32*)&ambColor = cAmbColor;   // folded by MWCC: const u32 → li r0, -1
GXSetChanAmbColor(GX_COLOR0A0, ambColor);
```

MWCC inlines the const value (`li r0, -1`) instead of emitting the sda
load, breaking the match against rodata.

**Where observed:**
- `src/GC2D/SelectMenu.cpp::TSelectGrad::perform` ambient color setup —
  the `GXColor cAmbColor` form produces the target's sda21 load +
  stash+reload pattern; the u32 cast form folds to `li -1`.

### `s8 result` (not `s32 result`) for int functions that compute via `(s8)idx`

**Rule:** A function whose semantic return type is `int` but whose
"hit" value comes from casting a small unsigned index produces target's
`extsb r_result, r_idx` directly when the result local is declared
`s8`, not `s32`:

```cpp
int func() {
    u32 idx = mField;
    s8 result = -1;          // not s32!
    if (idx == 0) return -1;
    ...
    for (...) {
        if (cond) {
            result = idx;    // implicit (s8) cast on store
            break;
        }
    }
    return result;           // implicit sign-extend on return
}
```

With `s8 result`, MWCC emits `extsb r_result, r_idx` (one
instruction). With `s32 result; result = (s8)idx;`, MWCC emits two
instructions: `extsb r_temp, r_idx; mr r_result, r_temp`. The s8 form
also keeps function byte-size correct.

**Companion lever — return type must also be `s8`.** With `int func()`
returning `int`, MWCC emits `extsb r3, r5` at the return path to
sign-extend the s8-result variable into int. Target uses `mr r3, r5`,
which means **the function actually returns `s8`** — the EABI/PPC
convention delegates s8 sign-extension to the *caller*, so an s8
return needs no callee extsb. The combination is `s8 func() { s8
result; ... result = idx; ... return result; }` — gives `extsb r5,
r4` in-loop AND `mr r3, r5` at tail. (Earlier ticks misread the
single-extsb-at-return as an unavoidable trade-off; that was wrong.)

**Where observed:**
- `src/GC2D/SelectMenu.cpp::TSelectMenu::getNextIndex` reached 100%
  with `s8 return type + s8 result + u32 idx + s32 i` (separate loop
  counter for signed `i < 8` while idx keeps unsigned compare for
  early-return).
- `src/GC2D/SelectMenu.cpp::TSelectMenu::getPrevIndex` reached 100%
  with `s8 return + s8 result + u32 idx`, plus a single-variable loop
  `for (s32 i = idx - 1; i >= 0; i--)` that lets MWCC set CR0 via
  `subic.` on the decrement (vs `addic.` on the loop-count). See
  separate Settled rule on `subic.; addi; blt` vs `subi; addic.; ble`
  below.

### Single-var loop `for (s32 i = base - 1; i >= 0; i--)` flips MWCC's flag-on-which-op choice

**Rule:** When a CTR-loop counts down from `(base - 1)` to `0`
inclusive, **two semantically-equivalent ways** to source it produce
different prologue patterns:

```cpp
// A — separate count + decremented base
base--;
for (s32 i = base + 1; i > 0; i--) {
    ... mStageStates[base] ...
    base--;
}
// → subi r4, r4, 1
//   addic. r0, r4, 1        ← flag on the count (i+1)
//   mtctr r0
//   ble .end                ← test "i+1 <= 0"

// B — single variable, "loop while i >= 0"
for (s32 i = base - 1; i >= 0; i--) {
    ... mStageStates[i] ...
}
// → subic. r4, r4, 1        ← flag on the decrement (i)
//   addi r0, r4, 1
//   mtctr r0
//   blt .end                ← test "i < 0"
```

Both produce a valid CTR loop with the same iteration count. The
difference is which operation sets CR0 and which signed branch tests
the result. To match a target asm that shows `subic.; addi; blt`, use
the single-variable form (B). The `i >= 0` loop check and the use of
`i` directly for array indexing are the cues MWCC reads.

**Where observed:**
- `src/GC2D/SelectMenu.cpp::TSelectMenu::getPrevIndex` — Form A at
  ~88-90% with `subi; addic.; ble`; switching to Form B gave the
  target's `subic.; addi; blt` and the final 10pp to 100%.

### `u32 idx` for unsigned array-index loops produces `cmplwi` (not `cmpwi`)

**Rule:** Loop counters/indices that come from a u8 field (`lbz`-loaded,
always non-negative) should be declared `u32` in source if the target
asm uses `cmplwi` (unsigned compare) for the loop-entry / loop-exit
checks. Declaring `s32 idx` produces signed `cmpwi`.

**Where observed:**
- `src/GC2D/SelectMenu.cpp::TSelectMenu::getNextIndex`,
  `getPrevIndex` — `lbz r4, 0x13b(r3)` followed by target's
  `cmplwi r4, 0x8` (or `r4, 0x0`) settled by changing
  `s32 idx = mScenarioIndex;` to `u32 idx = mScenarioIndex;`.

### `switch (mode) { case 0: ...; case 3: ...; default: }` for sparse two-case dispatch

**Rule:** For an if/else-if cascade over two integer constants (e.g.
`if (mode == 0) ... else if (mode == 3) ...`), MWCC emits a flat
sequence of `cmpwi; bne; cmpwi; bne` — but the target asm sometimes
shows a **switch-table-ish branch tree**:

```
cmpwi r0, 0x3
beq case_3
bge default       ; mode > 3 → skip
cmpwi r0, 0x0
beq case_0
b default
```

That tree (test the largest case first, then `bge default` to bail on
out-of-range high, then test smaller cases) is what MWCC emits for a
`switch` statement with two non-contiguous cases — even when only two
cases exist. Rewriting `if (mode == 0) ... else if (mode == 3) ...`
as a `switch (mode) { case 0: ...; case 3: ...; }` yields the tree.

**Where observed:**
- `src/GC2D/SelectMenu.cpp::TSelectGrad::perform` — color-byte
  inc/dec dispatch (74.13% → 87.35% just from this switch rewrite,
  applied twice).

### `JUtility::TColor color;` triggers default-ctor `set(0xffffffff)` stw — use ctor form instead

**Rule:** The default constructor for `JUtility::TColor` runs
`set(0xffffffff)` which lowers to `li r0, -1; stw r0, 0(slot)` —
visible in the prologue of the local's stack slot. If a function
then immediately overwrites the color with `color.set(r,g,b,a)`
(byte-by-byte), the int store from the default ctor remains in the
asm because **MWCC never eliminates stores to memory** (CLAUDE.md
*MWCC can eliminate redundant reads, but not writes*).

If the target asm has no early `stw -1` for the color slot, the
source did **not** default-construct the local. Two patterns avoid
the default ctor:

1. Direct-init with the 4-arg ctor: `JUtility::TColor color(r,g,b,a);`
2. Pass a ternary of TColor temporaries directly to the call site:
   `gpApplication.mFader->setColor(cond ? TColor(a,b,c,d) : TColor(e,f,g,h));`
   — this avoids both the default ctor AND the extra "anonymous prvalue
   temp → named local" copy step that `T color = cond ? T(...) : T(...);`
   produces.

**Where observed:**
- `src/GC2D/SelectDir.cpp::direct` 91.0% → 95.5% by switching both
  TColor uses from `JUtility::TColor color; color.set(...)` to (a) a
  ternary passed directly to setColor and (b) a `TColor(...)` ctor.
  Each removed the corresponding default-ctor `li -1; stw` prologue.

**Caveat:** The auto-generated TColor copy (`stw` of the loaded u32)
is still emitted at the call-site argument-slot copy (4-byte by-value
pass), which is the desired pattern. The fix removes only the
default-ctor's extra store, not legitimate copies.

### Two-step pointer init `p = base; p += off;` vs combined `p = base + off;`

**Rule:** When you want MWCC to emit an **in-place** `add rDst, rA, rB`
(where rDst is a callee-saved register that already held one of the
operands), declare the local with a base assignment and then add into
it as a separate statement:

```cpp
u8* readPos = dest;
readPos += size;
```

The combined form `u8* readPos = dest + size;` routes the result
through a volatile scratch register first, producing
`add r3, r30, r31; addi r31, r3, 0` (or `mr r31, r3`) — one extra
instruction and a different register layout. The two-step form lets
the allocator reuse the same register the operand `size` already
lived in (e.g. r31 in the example), giving `add r31, r30, r31`.

Required conditions for the lever:
- One operand is already in a callee-saved register (e.g. `size` was
  the third arg in r5 and got mr'd into r31 to survive a `bl memcpy`).
- The destination local will be **used multiple times across calls**
  (otherwise it stays in a volatile and the rule is moot).
- No reads of the original base after the add (so reusing the operand's
  register is safe).

**Where observed:**
- `src/JSystem/JKernel/JKRDvdAramRipper.cpp::nextSrcData` —
  `u8* readPos = dest + size;` capped the function at 98.02% with an
  extra `addi r31, r3, 0`. Switching to
  `u8* readPos = dest; readPos += size;` → 100%. The same two-step
  pattern also separates `origDest` (returned) from `readPos` (used
  for DVDReadPrio + srcLimit), letting the allocator give them
  distinct r30/r31 lives.

**Related but distinct:** This is NOT the same as the `addi rN, rM,
0` vs `mr rN, rM` open question — that one is about how a *move*
gets encoded. This is about whether an *arithmetic op* uses an
in-place destination or a temp + move.

### Static TVec3 init: direct-init `T x(args)` vs copy-init `T x = T(args)`

**Rule:** A file-scope `static JGeometry::TVec3<f32> name(a, b, c);`
(direct-initialization) lowers to three direct `stfs` into the
global's bss slot. The copy-initialization form
`static JGeometry::TVec3<f32> name = JGeometry::TVec3<f32>(a, b, c);`
materialises a temporary on the stack and then copies it word-by-word
via the auto-generated copy ctor — producing 3× `stfs` to stack +
3× `lwz`/`stw` from stack to global + the stack-frame inflation that
the temp implies. The two are NOT equivalent in MWCC despite being
semantically identical in C++98.

**Where observed:**
- `src/Player/MarioInit.cpp::__sinit` — `cDeformedTerrainCenter`
  rewritten 94.95% → 99.95% by changing copy-init to direct-init.
  Stack frame shrunk from 0x20 → 0x10 (the temp was 12 bytes plus
  padding).
- `src/Player/Atom.cpp::__sinit` already used direct-init form,
  matching. The MarioInit form was the outlier.

**How to spot:** `__sinit` shows 3 `stfs` into a stack offset followed
by 3 `lwz` / `stw` from that stack offset into the bss slot. Target's
asm has just the 3 direct `stfs` into the bss slot.

### Header-include controls __sinit JALList chain offsets

**Rule:** Each `JALListHioNode<T,...>`-derived class T defined in a
header instantiates a static `JALList<T>` singleton at TU scope when
that header is `#include`d. Missing the include causes the
TU-local bss to start its JALList chain at a different offset,
shifting EVERY downstream `addi r5, r31, OFFSET` and breaking
`__sinit` match.

`<MSound/MSoundBGM.hpp>` is the most common missing include for
TUs that need `JALList<MSBgm>` at bss offset 0. Without it, MWCC
puts `JALList<MSSetSoundGrp>` at offset 0 instead, shifting the
chain by exactly 0xC.

**Where observed:** 15+ TUs lifted 91.5% → 100% by adding
`#include <MSound/MSoundBGM.hpp>`. Recent ticks:
- `Camera/CameraJetCoaster` (tick 34)
- `Camera/sunmgr` (tick 36)
- `Player/Mario{Action,Autodemo,Collision,Draw,Jump,Run,Sound,Special,Wait,WaterGun,Main}.cpp`
- `MSound/{MAnmSound,MSoundStruct}.cpp`
- `NPC/NpcEvent.cpp`
- `Animal/AnimalBase.cpp`

**Caveat:** Some TUs have additional missing items beyond JALList<MSBgm>
(MarioMain needed `cDeformedTerrainCenter` static; MarioParticle
needs TBubbleCallBack/TWarpInCallBack vtables — these are TU-specific
classes not yet reconstructed).

### TVec2/TVec3 copy-ctor needs explicit POD-cast operator= for `lwz/stw` pattern

**Rule:** MWCC's auto-generated copy ctor for a class containing a
`JGeometry::TVec2<f32>` or `TVec3<f32>` member walks the floats
field-by-field, emitting interleaved `lfs`/`stfs` pairs — even though
the target asm uses batched `lwz`/`stw` integer-move pairs. To force
the integer pattern for these vector members, the vector type itself
must override copy ctor + `operator=` with a *POD struct cast*:

```cpp
struct _POD { T x, y; };       // or T x, y, z for TVec3
*(_POD*)this = *(const _POD*)&other;
```

MWCC lowers POD struct assignment to memcpy-like integer loads/stores;
field-by-field assignment respects the float types. The existing trick
in `TVec3<f32>` uses `*(Vec*)this = *(Vec*)&other` (where `Vec` is the
dolphin SDK POD); for `TVec2` we synthesise an inline POD struct.

**Where observed:**
- `TVec3<f32>::operator=` in `include/JSystem/JGeometry/JGVec3.hpp`
  has carried this trick since project start; comments explicitly note
  "yes, this has to use lwz/stw and not lfs/stf".
- `TVec2<T>::operator=` added in this tick → `TCameraMapTool` copy
  ctor 91.7% → 100% (the only `TVec2<f32>` member of TCameraMapTool's
  layout had been emitting float pattern; everything else integer).

**Caveat:** The POD cast emits 2× `lwz` then 2× `stw` (or 3 of each for
TVec3) in a *batched* shape. For the trick to take effect, the vector
member must be reached through an auto-generated whole-class copy
ctor; explicit field-by-field assignment in user code still uses the
float pattern. If you want batched int moves at a *specific call
site*, write `node.mPos = src.mPos;` (assignment) rather than
`node.mPos.set(src.mPos);` (the `set` overload uses float pattern).

### Typed class field beats `*(T*)((u8*)this + OFFSET)` cast for store sites

**Rule:** Writing to a `void*`-typed class field via the cast form
`*(void**)((u8*)this + 0x2A4) = nullptr;` emits a two-instruction
sequence (`addi rN, rThis, 0x2A4; stw r0, 0(rN)`) — MWCC materialises
the address into a scratch register before the store. The same field
declared as `void* unkXXX` on the class produces the natural
single-instruction `stw r0, 0x2a4(rThis)`.

Loads tend to be unaffected (MWCC happily emits `lwz r3, 0x2a4(r4)`
in either form), so the gain shows up mostly on store sites and on
addresses passed to subsequent calls.

**Where observed:**
- `CPolarSubCamera::execNoticeOnOffProc_` in `CameraNotice.cpp`
  95.67% → 100% after splitting the `unk21C` char-array into
  `unk21C[0x88]` + `void* unk2A4` + `unk2A8[0x20]` and rewriting all
  12 `*(void**)((u8*)this + 0x2A4)` sites to `this->unk2A4`. Also
  lifted `getNoticeActor_` 77.8% → 78.5% in the same TU.

**Caveat:** This is *not* the same as the addi-field-address caching
anti-pattern (where MWCC pre-computes `addi rN, rThis, OFFSET` once
and reuses across many accesses). That happens when the SAME field is
accessed ≥2 times across a function — and `this->field` doesn't fix
it. The fix here is the *single-access* case where each cast site
gets its own scratch-register-materialise + store.

### Repeated `a / b` divisions: rewrite as `a * (1.0f / b)` to enable reciprocal CSE

**Rule:** When the same integer→float divisor `b` is used in multiple
floating-point divisions, MWCC's `-O4,p` optimiser does **not** CSE the
`(1/b)` automatically — each `a / b` emits the full int→float magic
sequence (`lis 0x4330`, `stw`/`stw`, `lfd`, `fsubs` from magic constant)
plus a `fdivs`. Rewriting as `a * (1.0f / b)` with the literal `1.0f`
DOES trigger CSE: the entire reciprocal expression is hoisted once and
the result is reused via `fmuls` at each call site.

**Pattern (target asm, ONE reciprocal computed, reused):**
```
clrlwi r4, r30, 16          ; u16 cast of duration
stfs   ...                   ; (some unrelated store)
lis    r0, 0x4330
stw    r4, OFFSET(r1)
lfd    f1, magic_dbl@sda21
stw    r0, OFFSET-4(r1)
lfs    f2, 1.0f@sda21
lfd    f0, OFFSET-4(r1)
fsubs  f0, f0, f1            ; (double)dur
fdivs  f2, f2, f0            ; f2 = 1.0 / dur     <-- computed once
fmuls  f0, f3, f2            ; phase_X * (1/dur)
... (Y axis later) ...
fmuls  f0, f1, f2            ; phase_Y * (1/dur)  <-- reuses f2!
... (Z axis later) ...
fmuls  f0, f1, f2            ; phase_Z * (1/dur)  <-- reuses f2 again
```

**Source that does NOT CSE (3 separate int→float→fdivs sequences):**
```cpp
mAngleX.mDecrement = phase_X / (f32)(u16)duration;
mAngleY.mDecrement = phase_Y / (f32)(u16)duration;
mAngleZ.mDecrement = phase_Z / (f32)(u16)duration;
```

**Source that DOES CSE (1 reciprocal, 3 multiplies):**
```cpp
mAngleX.mDecrement = phase_X * (1.0f / (f32)(u16)duration);
mAngleY.mDecrement = phase_Y * (1.0f / (f32)(u16)duration);
mAngleZ.mDecrement = phase_Z * (1.0f / (f32)(u16)duration);
```

Important caveats observed:
- Pre-computing `f32 inv_dur = 1.0f / (f32)(u16)duration;` as a function-
  scope local **does not** match: MWCC computes inv_dur EARLY (at the
  declaration site) and uses a callee-saved register to carry it
  across the rest of the function, growing the stack by 4 bytes. The
  target instead computes the reciprocal LAZILY at the first use site
  and keeps it in a volatile fp register. Writing the literal `1.0f / b`
  expression INSIDE each axis's compound expression triggers CSE
  WITHOUT promoting it to a saved local.
- Pre-computing `f32 dur_f = (f32)(u16)duration;` then using `phase / dur_f`
  also fails — MWCC sees three separate divisions and emits three fdivs.
  The literal `1.0f` and the divide it controls must appear in the
  source AT each axis site.

**Where observed:**
- `src/Camera/camerashake.cpp::startShake` — 75.3% → 99.68% (+24.4pp).
  Three axis blocks each computing `phase / (f32)(u16)duration` rewritten
  to `phase * (1.0f / (f32)(u16)duration)`. Stack frame shrank from
  0x38 → 0x50 to match target's 0x50.
- `src/Camera/camerashake.cpp::keepShake` — 75.9% → 87.3% (+11.4pp).
  Same rewrite, three axis blocks. Remaining 13% from unrelated
  pointer-walk pattern in the dispatch loop.

### Multiple bools live simultaneously: declare and init up-front to get separate registers

**Rule:** When target's asm shows multiple boolean variables initialized
to 0 BEFORE any test runs (e.g. `addi r4, r3, 0; addi r0, r3, 0` —
two distinct registers being zeroed at the top of an inline) and each
later conditionally set to 1 via `li rN, 1`, the original source
declared each bool up front with `bool x = false;` and used a
SEPARATE `if (cond) x = true;` for each.

The naive chained form `bool b = a && extraCheck;` puts each bool's
register-life into a tight window (alive only briefly between
assignment and use) so MWCC reuses ONE register across all of them —
producing extra `li r0, 0` re-inits and redundant `clrlwi.` normalize
steps.

**Symptom in our build:** Function inlines `inViewCone`-like helpers
with 4+ normalize positions (vs target's 3) and 1-2 extra
`li rN, 0; clrlwi. r0, ...` reinitializations between each `&&`
check. Match drops 15-25pp depending on how many times the inline is
expanded.

**Pattern (target asm) for a 3-bool chain like inViewCone:**
```
addi r4, r3, 0          ; b = 0 (initialized BEFORE any check)
addi r0, r3, 0          ; a = 0
... a-check ...
li r0, 1                 ; a = 1 if check passed
clrlwi. r0, r0, 24       ; test a
beq L_skip_b             ; if !a, b stays 0
... b-check ...
li r4, 1                 ; b = 1
L_skip_b:
clrlwi. r0, r4, 24       ; test b
beq L_skip_c             ; if !b, c stays 0
... c-check ...
li r3, 1                 ; c = 1
```

Note: 3 distinct registers (r0, r4, r3) for a/b/c, each only zeroed
ONCE at the top. No re-initialization between checks.

**Source that matches:**
```cpp
bool a = false;
bool b = false;
bool c = false;
if (firstCheck()) a = true;
if (a && secondCheck()) b = true;
if (b && thirdCheck()) c = true;
return c ? true : false;
```

**Source that does NOT match (extra re-inits + 1 register reused):**
```cpp
bool a = firstCheck();
bool b = a && secondCheck();
bool c = b && thirdCheck();
return c ? true : false;
```

The chained form makes each bool short-lived → MWCC coalesces them
into one register, requiring `li r0, 0` before each use to "reset" the
register's contents.

The final `return c ? true : false` keeps the trailing
intermediate-byte normalize (see Settled rule
[[Ternary `? true : false` produces intermediate-byte sequence]]).

**Where observed:**
- `src/Camera/CameraNotice.cpp::inViewCone` — namespace-anonymous
  inline used twice in `getNoticeActor_`. With the chained `bool a =
  X && Y;` form, both inline expansions had 4 normalize steps and 1
  reused register, capping the function at ~72.4%. Switching to the
  up-front-init + `if` form gave 3 normalize steps and 3 distinct
  registers per inline — `getNoticeActor_` 72.43% → 75.88% (+3.45pp)
  in one commit, with the second inline expansion confirming the
  same delta. Together with the `? true : false` removal on each
  bool, +22.14pp across the function.

### Cache global-pointer-via-stores: introduce a local to silence reload

**Rule:** When the same global pointer is dereferenced multiple times in a
row, with intervening stores to memory through *another* pointer, MWCC
pessimistically reloads the global (`lwz r3, gFoo@sda21`) before each
deref because the store could alias the global. Caching the pointer
into a local `T* foo = gFoo;` before the first use lets MWCC keep
the pointer in a single register across all the reads.

**Pattern in source:**

```cpp
// Pessimistic — emits 3× `lwz r3, gpCameraMario@sda21`:
out->x = gpCameraMario->mPosX;
out->y = gpCameraMario->mPosY;
out->z = gpCameraMario->mPosZ;

// Cached — single `lwz r3, gpCameraMario@sda21`:
TCameraMarioData* mario = gpCameraMario;
out->x = mario->mPosX;
out->y = mario->mPosY;
out->z = mario->mPosZ;
```

The relevant signal in the target is whether `lwz rN, gFoo@sda21`
appears once or repeatedly across consecutive stores to a different
pointer. Repeated reloads => no local; single load => cached local.

**Where observed:**
- `src/Camera/CameraTalk.cpp::ctrlTalkCamera_` (100% match) — uses
  the cached `TCameraMarioData* mario = gpCameraMario;` pattern.
- `src/Camera/CameraNotice.cpp::getNozzleTopPos_` — 96.90% → 99.84%
  applying the same cache for the non-watergun branch.
- `src/Camera/CameraNotice.cpp::ctrlLButtonCamera_` — same lever
  applied twice (3 separate `gpCameraMario->mPos*` blocks), part of
  61.18% → 73.25% gain.

### `if (!cond)` to flip block-ordering when target's branch is on TRUE

**Rule:** MWCC always emits the source's THEN-block before the
ELSE-block (per CLAUDE.md). So when target's asm shows
`bne THEN_LABEL` (jump *on true*) with the THEN block placed AFTER
the unconditional fall-through ELSE block, the original source
wrote the condition INVERTED. I.e., `if (!cond) ELSE_body else
THEN_body`, with the labels named from the diff's perspective.

**Pattern (target asm):**
```
test
bne LABEL_X     ; jump on TRUE
... block A ... ; fall-through (this is the ELSE branch of original src!)
b END
LABEL_X: ... block B ...   ; reached only if test was TRUE
END:
```

**Source that matches:**
```cpp
// NOT: if (cond) { block_B } else { block_A }
// (would emit `beq LABEL_A; block_B; b END; LABEL_A: block_A; END:`)
//
// YES:
if (!cond) {
    block_A
} else {
    block_B
}
// emits `bne LABEL_B; block_A; b END; LABEL_B: block_B; END:` — match.
```

In other words: invert the condition AND swap the branches if you
want the textual order of THEN/ELSE in asm to swap.

**Where observed:**
- `src/Camera/CameraNotice.cpp::ctrlLButtonCamera_` — outer test
  `SMS_CheckMarioFlag(0x8000)`. Target emits `bne 0x84` (jump on
  true to the watergun-path). Natural `if (flag) { watergun } else
  { copy }` emits `beq` to the else; rewriting as
  `if (!flag) { copy } else if (gun == null) { copy } else { watergun }`
  swaps the layout and matches target. +8pp this fn alone.
- `mario/Camera/CameraBGCheck::isValidCamClip` (tick 22) — also
  uses this lever indirectly via the nested-ternary form of
  `isLegal()`. The `?:` chain creates similar branch-order inversions.

This is a refinement of the broader CLAUDE.md "compiler NEVER swaps
the true/false block" rule — combined with the observation that
target's branch direction (`bne` vs `beq`) tells you whether the
THEN block lives at the fall-through or at the labeled position.

### `isLegal()` style: nested `? true : false` is the double-normalize lever

**Rule:** When the target asm shows the *double* bool-normalize pattern
(two consecutive `li r0, 1; b; li r0, 0` blocks separated by
`clrlwi r0, r0, 24; cmplwi r0, 1; bne ...`), the original source
constructed the bool by *nesting* two ternary `? true : false`
expressions, not by writing `bool x = ...; if (!x)`.

The reference pattern is `TBGCheckData::isLegal()` in
`include/Map/MapData.hpp`:
```cpp
// checkFlag returns first ? true : false  (normalize #1)
bool checkFlag(u32 flag) const { return mFlags & flag ? true : false; }
// isLegal compares the bool with 1, then ? false : true  (normalize #2)
bool isLegal() const {
    return checkFlag(BG_CHECK_FLAG_ILLEGAL) == 1 ? false : true;
}
```
When `isLegal()` is inlined at a call site, the call site emits the
characteristic
```
lhz r0, 4(rX); rlwinm. r0, r0, 0, 27, 27   # mFlags & ILLEGAL
beq L1; li r0, 1; b L2                      # normalize #1: bool conv
L1: mr r0, r3 (or li r0, 0)
L2: clrlwi r0, r0, 24; cmplwi r0, 1; bne L3 # negate the bool
li r0, 0; b L4
L3: li r0, 1                                 # normalize #2: !bool
L4: clrlwi. r0, r0, 24; beq end             # finally, test
```

In contrast, the natural-looking source `bool b = expr; if (!b)`
collapses to a single `cmpwi r0, 0; bne` — *one* normalize, no
roundtrip. So when matching, swap to the nested-ternary form (or
call the existing `isLegal()` / similar helper).

A single-normalize sibling pattern — one `li 1; b; li 0` block,
no `cmplwi r0, 1` afterwards — comes from one explicit
`int_expr & MASK ? true : false`. Used at the same call site for
e.g. `data->mBGType & BG_PROPERTY_FLAG_CAMERA_WONT_CLIP ? true : false`.

**Citations:**
- `CameraBGCheck.cpp::isValidCamClip` (tick 22) — `data->isLegal()`
  + `data->mBGType & WONT_CLIP ? true : false` together produced the
  full target asm for the validity check inlined into
  execGroundCheck_ / execRoofCheck_ / execWallCheck_, +30pp combined.
- `MapData.hpp::isLegal` is itself the canonical example (its
  callers across the codebase emit the same shape).

### Inner parens around the divisor expose `fdivs+fmadds` fusion

**Rule:** With `-fp_contract on`, MWCC fuses `a + b * c` into `fmadds`.
When the multiplicand involves a division, the *position of the division*
in the expression decides whether the multiply (and thus the fusion) is
the top-level rightmost operation.

```cpp
// Outer division — division is the LAST op at top level.
// Compiles as: fmuls tmp, b, (c-d); fdivs tmp, tmp, e; fadds r, a, tmp
//   → 3 instructions, no fusion.
f32 r = a + b * (c - d) / e;

// Inner-parens division — division is buried inside the multiplicand,
// so the multiply is the top-level rightmost op.
// Compiles as: fdivs tmp, (c-d), e; fmadds r, b, tmp, a
//   → 2 instructions, fused.
f32 r = a + b * ((c - d) / e);
```

The same logic explains a sister shape:

```cpp
// Left-to-right `b/c*d` parses as `(b/c)*d`; the multiply is rightmost.
// Compiles as: fdivs tmp, b, c; fmadds r, tmp, d, a (= fdivs + fmadds).
f32 r = a + b / c * d;
```

When the target asm shows `fdivs` immediately followed by `fmadds` (no
intervening `fmuls`), the source has the division *inside* the multiplicand
(via inner parens, or via left-associative `/` placement that puts the
multiply last). When the target shows `fmuls; fdivs; fadds` instead, the
source has the division as the top-level last op (`a + b*(c-d)/e` shape).

This is the inverse of [[Splitting `a + b * c` into two statements
defeats -fp_contract on fusion]] — that one shows how to BREAK fusion;
this one shows how to PROMOTE it from a 3-instruction unfused chain to
the 2-instruction fused pair.

**Where observed:**
- `src/MSound/MSHandle.cpp::calcDolby` HiSence_Dist interpolation
  (tick 19): `0.5f + param * ((d - 0.5f) / cPan_HiSence_Dist)` —
  inner-parens form — compiled to `fdivs; fmadds` matching target.
  Without the inner parens (the natural `a + b*(c-d)/e` form), MWCC
  emits `fmuls; fdivs; fadds`. +6pp on calcDolby.
- `src/MSound/MSHandle.cpp::calcDolby` cDol_HalfRad branch (tick 19,
  same TU, different expression shape):
  `0.5f + 0.5f / (cDol_FullRad - cDol_HalfRad) * (angle - cDol_HalfRad)`
  — left-associative `/` so it parses as `0.5f + ((0.5f / (FR-HR)) * (angle-HR))`,
  putting the multiply last. Compiles to `fdivs + fmadds` matching target.

### `static inline` wrapper trick defeats auto-inlining of one call site

**Rule:** When target keeps a header-defined inline function as a `bl` weak
call but MWCC's `-inline auto` (or `-inline deferred`) is recursively
inlining it at the call site, wrap the call in a `static inline` helper
inside the same TU:

```cpp
// In NpcParts.cpp (the TU that's losing match):
static inline void setEffectMtxOnTex0(J3DMaterial* mat, MtxPtr mtx)
{
    mat->getTexGenBlock()->getTexMtx(0)->setEffectMtx(mtx);
}

// Then at the call site:
setEffectMtxOnTex0(mdata->getMaterialNodePointer(j), effectMtx);
```

MWCC inlines the wrapper at the call site, but does NOT recursively
inline through it — so the contained `bl setEffectMtx` survives as a
direct call to the weak symbol. The weak symbol itself is still emitted
in the TU (whichever way the inline body is reached), so the linker
deduplicates across TUs.

This is **selective** in a way `#pragma dont_inline on` is not: only the
specific call site routed through the wrapper is affected. Other inlines
in the same TU (accessor inlines, `getModel()`/`getModelData()` chains,
etc.) continue to inline as before.

**Where observed:**
- `src/Map/MapModel.cpp` ships with a `static inline void fake(J3DMaterial*,
  MtxPtr)` helper (line 16) wrapping `setEffectMtx`. This is the original
  source pattern — confirmed by 99.97% match on the whole TU.
- `src/NPC/NpcParts.cpp::partsPerform` was at 56.73% with MWCC inlining
  the matrix copy + adding f30/f31 callee-saved spills. Adding the same
  wrapper pattern (`setEffectMtxOnTex0`) lifted match to 92.56% (+35.83pp)
  with the bl call restored. The previous IMPLEMENTATION tick's attempt
  with `#pragma dont_inline on` matched setEffectMtx but regressed the
  ctor 62.5% → 43.3% and addJellyFishParts 89.5% → 76.5% by breaking
  unrelated accessor inlines.

**Related:** see also the `MWCC sometimes inlines a call selectively`
hypothesis in this file — the wrapper trick is the source-level lever
to control selective inlining, and may resolve that hypothesis once
applied broadly.

### Vtable order in headers must match target asm one-for-one

**Rule:** MWCC emits `virtual` methods into the vtable in their lexical order
of declaration in the class definition. Even a single virtual swap in the
header shifts every later slot by 4 bytes, causing all virtual-call sites
that dispatch the later methods to use the wrong vtable index. The symptom
at the call site is a `lwz r12, OFFSET(r12)` whose `OFFSET` differs from
target by exactly 4 bytes (or a multiple of 4 if multiple slots are wrong),
with no other instruction differences.

A non-`virtual` method declared between `virtual` methods does NOT consume
a vtable slot but does NOT shift later slots either — but if a method
should be virtual and isn't, it's missing from the vtable entirely.

**How to detect:** Diff the function's asm against the target; if the only
mismatch is a single virtual-call `lwz r12, OFFSET(r12)` (and possibly a
trailing `bne 0xXXX` whose only difference is the branch target shifted
by 4 due to the size mismatch), it's a vtable-order bug. The fix is in the
class header, not the .cpp file. The class's own vtable (`__vt__<class>`)
will also appear in the original .o; reading off the symbol order there
gives the canonical order.

**Tool:** `tools/agent/find_vtable_diffs.py` scans `report.json` for
near-match functions with exactly this pattern and prints them as
single-line fixes.

**Where observed:**
- `mario/Player/MarioCollision::floorDamageExec` — target used
  `lwz r12, 0xdc(r12)` (damageExec); ours used `0xd8` (checkCollision).
  TMario header had `damageExec` declared BEFORE `checkCollision`; target's
  vtable order is `checkCollision`, `damageExec`, `getVoiceStatus`,
  `drawSyncCallback`. Also, `drawSyncCallback` was declared non-virtual in
  the header but appears in target's vtable. Fix: swap declaration order
  and add `virtual` to `drawSyncCallback`. Net: +3 matched functions.
- `mario/MoveBG/MapObjSirena::TPanelRevolve::control` — target used
  `lwz r12, 0x114(r12)` (`setUpCurrentMapCollision`); source called
  `updateObjMtx()` (`0x110`). Method name in source was wrong, not vtable
  order — same diagnostic signature.

### Splitting `a + b * c` into two statements defeats `-fp_contract on` fusion

**Rule:** Under the project-wide `-fp_contract on` flag, MWCC fuses
`a + b * c` (or `a - b * c`) into a single `fmadds`/`fmsubs`
instruction. To force the unfused `fmuls; fadds` pair the target
emits, split the expression across two statements with an
intermediate `f32` local:

```cpp
// Fused — produces fmadds f0, b, c, a (1 instr)
f32 r = a + b * c;

// Unfused — produces fmuls tmp, b, c ; fadds r, a, tmp (2 instrs)
f32 r = b * c;
r = a + r;
```

The intermediate store/reload of the multiply result through the
named local introduces a sequence point that MWCC respects: it will
not fuse across the assignment.

**Where observed:**
- `src/NPC/NpcTrample.cpp::updateTrample` —
  `*out = dt * (1.0f + unk0 * (-JMASSin(angle)))` produced
  `fmadds` instead of target's `fmuls; fadds; fmuls`. Splitting
  into `f32 mod = unk0 * (-JMASSin(angle)); mod = 1.0f + mod;
  *out = dt * mod;` produced the target's unfused pair (90.9%→99.2%).

### `JMASSin(idx) + JMASCos(idx)` inline at use site → shared address bases

**Rule:** When the same `s16` index is passed to both `JMASSin` and
`JMASCos`, inlining the lookups *at the assignment-site expression*
(rather than caching `sin_a`/`cos_a` as `f32` locals) causes MWCC to
share the index computation via CSE and to precompute the per-table
addresses (`add r6, sin_table, idx*4`; `add r5, cos_table, idx*4`)
once for reuse across the dependent expressions.

```cpp
// Locals — MWCC emits `lfsx fN, base, idx` indexed loads, possibly
// reloading the table base each time.
f32 sin_a = JMASSin(yawShort);
f32 cos_a = JMASCos(yawShort);
dst.x = a * cos_a + b * sin_a;
dst.z = -a * sin_a + b * cos_a;

// Inlined at use — `add rA, table, idx*4; lfs fN, 0(rA)`. The
// shifted-index register can be reused for the second expression's
// table loads.
dst.x = a * JMASCos(yaw) + b * JMASSin(yaw);
dst.z = -a * JMASSin(yaw) + b * JMASCos(yaw);
```

**Where observed:**
- `src/NPC/NpcThrow.cpp::throwMario` — caching `sin_a`/`cos_a` gave
  88.6% (lfsx form, with `lwz r4, table` reloaded between the two
  rotation expressions). Inlining `JMASSin/JMASCos` directly in the
  two assignments gave 100% (target's `add; lfs 0(rN)` form, with
  the indices/table-bases reused across both expressions).

### Initialise pointer to NULL before the if to fall through the null path

**Rule:** Target prefers `Type* p = NULL; if (cond) p = call();` (init
+ conditional override) over `Type* p; if (cond) p = call(); else
p = NULL;` (explicit else). The init form produces:

```
li r28, 0              ; init NULL
... test ...
bne CALL               ; if non-null, jump to call
b MERGE                ; fall-through past NULL path
CALL: bl getter
mr r28, r3
MERGE:
```

The explicit-else form produces an extra `li r28, 0` in the else
branch:

```
... test ...
beq SETZERO
bl getter
mr r28, r3
b MERGE
SETZERO: li r28, 0
MERGE:
```

The init form drops 1-2 instructions and matches target's typical
layout.

**Where observed:**
- `src/NPC/NpcInbetween.cpp::execMotionBlend` — first occurrence of
  `J3DAnmTransform* old_ptr;` inside the forced-blend branch went
  82.3%→84.3% when rewritten as `J3DAnmTransform* old_ptr = nullptr;
  if (mactor->unkC) old_ptr = ...;`. Note the second occurrence in
  the same function keeps the explicit if/else — so the choice is
  per-call-site, not TU-wide.

### 1-case `switch` defeats the bne-skip optimization for `if (x == K) call();`

**Rule:** Source `if (x == K) call();` produces a 3-instruction
optimized sequence:
```
cmpwi r0, K
bne SKIP
bl call
SKIP:
```
Target sometimes emits the 4-instruction "literal if-then" form instead:
```
cmpwi r0, K
beq DO_CALL
b SKIP
DO_CALL: bl call
SKIP:
```

To force the 4-instruction form, wrap the call in a 1-case switch:
```cpp
switch (x) {
case K:
    call();
    break;
}
```

This is a refinement of the broader rule that `switch` defeats
optimization patterns (see "switch defeats fusion of multiple equality
compares"); even single-case switches can shift codegen.

**Where observed:**
- `src/System/MarDirectorEvent.cpp::movement` — target uses 4-instruction
  `beq;b;bl` layout for `if (mState == 4) movement_game();`. The bare
  `if` form gave 91% (3-instruction `bne;bl`); switching to
  `switch (mState) { case 4: movement_game(); break; }` matched 100%.

### Direct `this->[OFFSET]` field access vs static helper with hit_actor arg

**Rule:** When a non-static method calls a static helper that accesses
`((CastT*)arg)->field`, and the calling method's compiled body needs
to access `this->[OFFSET]` (not `arg->[OFFSET]`), the original source
likely does NOT call the helper — it directly accesses the field via
`this`. Even though the helper's body looks identical, calling
`helper(arg)` produces `lwz r?, OFFSET(r4)` (uses the arg in r4) while
direct `((CastT*)this)->field` produces `lwz r?, OFFSET(r3)` (uses
this).

Pattern: when target's body has `lwz r0, 0x68(r3)` (r3=this) but our
source calls a helper that takes a hit_actor arg, rewrite the body to
access the field directly via `((CastT*)this)->field`.

**Where observed:**
- `src/MoveBG/MapObjLib.cpp::getWaterPos/getWaterSpeed/getWaterPlane`
  — target accessed `this->[0x68]` (the TTakeActor::mHolder field
  slot, treated as a TWaterHitActor::unk68 water-ID). Previously we
  called `getWaterID(hit_actor)` which inlined to use the arg in r4.
  Replacing with `((TWaterHitActor*)this)->unk68` matched all three
  100%.

### 2-case fused switch produces midpoint-excluded bisection

**Rule:** A switch with exactly two case labels falling into one block:
```cpp
switch (x) {
case A:
case B:
    result = true;
}
```
compiles to a "midpoint-excluded" binary search:
```
cmpwi r, MID    ; MID = (A+B)/2
beq END         ; midpoint → false (since MID ∉ {A, B} when A,B differ by 2)
bge UPPER       ; > MID → check B
cmpwi r, A
bge SET         ; ≥ A and < MID → SET (only A matches here)
b END
UPPER:
cmpwi r, B+1
bge END         ; ≥ B+1 → false
SET: li r, 1    ; ≤ B and > MID → true (only B)
END:
```

Critically, this is **NOT** the same as `if (x == A || x == B)`, which
MWCC tends to compile to two flat `cmplwi`/`beq` pairs. The switch form
is what produces the bisection.

When the target asm shows the `cmpwi midpoint; beq END; bge UPPER; ...`
pattern, it's a 2-case switch — even if the two values look like they
could be a range (e.g. 0x88B/0x88D with 0x88C "missing" between them).
The midpoint is mechanically excluded; do not add it as a third case.

**Where observed:**
- `src/Camera/CameraMarioData.cpp::isMarioRocketing` — target had
  `cmpwi r3, 0x88c; beq END; bge UPPER; cmpwi 0x88b; bge SET; b END;
  UPPER: cmpwi 0x88e; bge END; SET: li r31, 1`. Looked like a range
  check 0x88B–0x88D but was actually `case 0x88B: case 0x88D:`. The
  3-case form (with 0x88C added) compiled to a clean range check
  `cmpwi 0x88e; bge END; cmpwi 0x88b; bge SET; b END` (85.4% match);
  removing 0x88C gave the midpoint-excluded bisection → 100%.

### Callee return type `bool` → callers emit `clrlwi r0, r3, 24` on the result

**Rule:** Helper / template functions declared to return `bool` (u8 in this
ABI) cause every caller that consumes the return value in an integer or
BOOL context to mask the result with `clrlwi r0, r3, 24` before testing it.
Target asm typically uses the raw `r3` directly (e.g. `cmpwi r3, 0` or
`mr/cmpw`-style chains), meaning the original declaration was `BOOL`/`int`,
not `bool`.

If multiple callers exist, change both the helper's return type AND any
forwarding function that returns its result. A helper that returns `BOOL`
but is wrapped by a `bool`-returning function still emits a BOOL→bool
convert at the wrapper's `return` (`neg r3, r3; subic r0, r3, 0x1;
subfe r0, r0, r3; clrlwi r3, r0, 24`).

**How to detect:** target shows the call followed by a direct cmpwi/cmpw,
ours shows `bl helper; clrlwi r0, r3, 24; ...`. Or the wrapper function
shows the four-instruction BOOL→bool convert sequence right after a call.

**Where observed:**
- `include/Camera/cameralib.hpp::CLBChaseGeneralConstantSpecifySpeed<T>`
  was declared `bool`. Changing it to `BOOL` (and matching its forwarder
  `CLBChaseSpecialDecrease`) closed `clrlwi` mismatches in
  `TBaseNPC::execTurnToFirstState` (97.98% → 99.04%),
  `TBaseNPC::execUTurn` (97.97% → 98.84%), and
  `CPolarSubCamera::execHeightPan_` (51.4% → 52.3%).
  Confirmed in two TUs (NpcWalkTurn, CameraHeightPan).

### `switch` + `default:` wraps follow-up logic when target jumps past it

**Rule:** When source has the shape

```cpp
switch (x) { case A: case B: result = FALSE; break; }
if (some_other_condition) result = FALSE;
return result;
```

— and target's case bodies end with `b END_OF_FUNCTION` (skipping the
follow-up `if` entirely) — the original source moved the trailing
`if (some_other_condition)` into the `default:` branch. With:

```cpp
switch (x) {
case A: case B:
    result = FALSE;
    break;
default:
    if (some_other_condition) result = FALSE;
    break;
}
return result;
```

the case bodies emit `li r6, 0; b end` instead of `li r6, 0` followed by
fall-through into the always-run condition. This is the difference between
"case sets result, conditionally also runs follow-up" and "case OR
default, never both" — semantically equivalent when the case body sets
result FALSE (since the follow-up only ever sets FALSE too) but
structurally distinct in MWCC's output.

**How to detect:** target's last switch case ends with `li rN, 0; b END;`
where END is past a conditional block your build runs unconditionally.

**Where observed:**
- `TBaseNPC::isTurnToMarioWhenTalk` — switch on 5 actor-type cases
  followed by an `mActionFlag & 0xC01` check. Moving the flag check
  into `default:` closed 96.55% → 100%.
- `TBaseNPC::isTurnToMarioWhenApproach` — range `0x16..0x18` enumerated
  as cases, follow-up flag check (`mActionFlag & 0x7E7F`) moved to
  `default:`. 87.96% → 100%.

### `bool` vs `BOOL` local must match the function's return type

**Rule:** When a function returns `bool` (u8) and uses a local result variable,
declare it as `bool result = false` — NOT `BOOL result = FALSE`. The latter
forces MWCC to emit an int→bool cast at return:
`neg r3, r0; addic r0, r3, -1; subfe r0, r0, r3; clrlwi r3, r0, 24`.
With `bool result = false`, the function returns the local directly
(`mr r3, r0; blr` or equivalent) with no cast.

Symmetrically, a `BOOL`-returning function should use `BOOL result = FALSE` —
mismatched bool↔BOOL in either direction creates a cast at the boundary.

**Where observed:**
- `src/Camera/CameraMarioData.cpp::isMarioGoDown` — function returns `bool`.
  With `BOOL result = FALSE`, ended at 76% (4 extra instructions for the
  cast). Changing to `bool result = false` → 100%.

### Ternary `? true : false` produces intermediate-byte sequence

**Rule:** `return cond ? true : false` (with `bool` literals) compiles to
`li r0, 1 / b set / li r0, 0 / set: clrlwi r3, r0, 24` — an
intermediate-byte materialisation followed by a u8 mask. The cleaner
`if (cond) return TRUE; return FALSE;` (with `BOOL` literals and a
`BOOL`-returning signature) compiles to two separate `li r3, K; blr`
return blocks — direct int returns.

Choose the form by matching the target's exit pattern:
- Target has shared exit with mask → use ternary with `bool`.
- Target has split exit blocks with `li r3, 0/1; blr` directly → use
  `if/return` with `BOOL`.

**Where observed:**
- `src/Enemy/bossgesso.cpp::is2ndFightNow` — `return cond ? true : false`
  gave 78.7%. Switching to `if (cond) return TRUE; return FALSE;` (and
  matching `BOOL` return type behaviour) → 100%.

### `__fabsf` intrinsic survives `#pragma dont_inline on`

**Rule:** Under TU-global `#pragma dont_inline on`, MWCC does NOT inline
the `fabsf(x)` library wrapper from `<math.h>` — it emits a `bl fabsf`
call instead of the single `fabs f1, f1` PowerPC instruction. The
`__fabsf` compiler intrinsic, by contrast, is *not* an inline function
but a builtin: it lowers directly to `fabs f1, f1` regardless of any
inlining pragmas.

When a TU has `dont_inline on` set, always use `__fabsf` rather than
`fabsf` (and likewise `__fabs` for double). Without it, callers gain a
`bl` call plus extra stack frame for the call.

**Where observed:**
- `src/NPC/NpcBase.cpp::isInMadSearchRange`/`isInBodyTurnSearchRange`
  with `fabsf` and `dont_inline on` produced a `bl ` call and 48-byte
  stack frame. Switching to `__fabsf` produced the inline `fabs`
  instruction and dropped to 24-byte stack frame, jumping from ~24% to
  98% match.
- Cross-confirmed in `src/Animal/Bird.cpp` and several Enemy/ files —
  they all use `__fabsf` explicitly, suggesting this is a known
  workaround.

**Related:** This is a special case of the broader rule that `.get()`
accessors on `TParamRT<T>` get blocked from inlining under
`dont_inline on`. Bypass it by accessing `.value` directly.

### TParamRT<T>::value direct access under `#pragma dont_inline`

**Rule:** Under TU-global `#pragma dont_inline on`, the inline
`get()` accessor on `TParamT<T>` cannot be inlined — MWCC emits a
`bl` call to a stub returning the address of `value`. Accessing
`param->mField.value` directly skips the accessor and produces the
expected `lfs f0, OFFSET(rN)` instruction.

**Where observed:**
- `src/NPC/NpcBase.cpp::isInMadSearchRange` — `param->mMadSearchHeight.get()`
  with `dont_inline on` produced a `bl ...` call; switching to
  `param->mMadSearchHeight.value` eliminated the call.

### Intermediate `bool match` local produces target's r0→r31 pattern

**Rule:** When target uses an intermediate result register
(typically r0) for an inner expression's outcome, then `mr r31, r0`
at the end — declaring the result as a SEPARATE local that is
assigned in both branches of an if/else and then copied to the
final result produces this code shape. The simpler
`bool result = false; if (cond) result = true;` form (without the
intermediate temp) produces `li r31, 1` direct stores instead.

**Pattern (target asm):**
```
... bunch of compares + beq to TRUE block ...
TRUE:  li r0, 1; b L_SET
FALSE: li r0, 0
L_SET: mr r31, r0
```

**Source that matches:**
```cpp
bool match;
if (test) {
    match = true;
} else {
    match = false;
}
result = match;
```

**Where observed:**
- `src/Camera/CameraMarioData.cpp::isMarioSlider` — adding the
  intermediate `bool match` after the OR-chain test took the
  function from 95.8% (with direct `result = true` in the
  if-block) to 100%.
- Implicitly used as the standard NPC-predicate pattern in
  `src/NPC/NpcBase.cpp` for all isXxx functions added this tick
  (isPollutionNpc, isChild, isBehaveToWaterNpc, etc).

### Single-equality `switch` defeats subis fusion too

**Rule:** Even when there's only ONE equality check against a
constant with shared high bits (e.g. `mActorType == 0x04000013`),
MWCC will compile `if (x == K) ...` with subis-fusion
(`subis r4, r5, 0x400; cmplwi r4, 0x13`) instead of the target's
direct `addi r0, r4, 0x13; cmpw r5, r0`. Wrapping the single case
in a `switch (x) { case K: ... }` defeats the fusion and produces
the target's two-instruction comparison.

This is a refinement of the broader switch-defeats-fusion rule
below — even single-case switches help when the constant has
high bits that would otherwise trigger fusion.

**Where observed:**
- `src/NPC/NpcBase.cpp` — `isNormalMareW`, `isNormalMareM`,
  `isSpecialMonteW` each check one mActorType value (0x04000013,
  0x0400000E, 0x0400000D). Direct `if` form gave 22.5%–59% match
  after the bool-result rewrite; wrapping in a 1-case switch took
  them all to 100%.

### `switch` defeats fusion of multiple equality compares

**Rule:** When source has multiple equality compares of an int against
distinct constants — whether the high bits share (`subis` fusion) or the
constants are consecutive (`subi + cmplwi range` fusion) — MWCC's optimiser
will fold them into range-bisection arithmetic that doesn't match target's
clean `cmpwi + beq/bge` tree. Rewriting the dispatch as
`switch (x) { case a: case b: ...: result = true; } return result;`
reliably defeats the fusion and produces target's binary-search comparison
pattern.

**Companion rule — initializing a `bool result = false` upfront** hoists
`li r3, 0` to BEFORE the comparison tree and unlocks MWCC's
conditional-return idiom: false paths emit `beqlr`/`bltlr`/`bgelr` instead
of a unified `li r3, 0; blr` block. Target consistently uses this pattern
for predicate functions.

**Where observed:**
- `src/Camera/CameraTalk.cpp::makeMtxForTalk` — high-bits-shared constants
  `0x400001A`, `0x4000007` → `subis r4, r5, 0x400 ; cmplwi r4, 0x1a ; ...
  cmplwi r4, 0x7`. Switch rewrite produced `cmpw + beq + bge`. 74.7% → 84.4%.
- `src/Camera/CameraMode.cpp` — six independent predicate functions
  (`isTalkCameraSpecifyMode`, `isFixCameraSpecifyMode`,
  `isDefiniteCameraSpecifyMode`, `isFollowCameraSpecifyMode`,
  `isTowerCameraSpecifyMode`, `isLButtonCameraSpecifyMode`) all went from
  39–55% → 100% by converting `if (x == A) return true; ...` chains to
  `bool result = false; switch (x) { case A: ...: result = true; } return result;`.
  Consecutive-value fusion (`subi + cmplwi range`) and range checks
  (`mode >= A && mode < B` rewritten as explicit case enumeration) also
  matched after the switch rewrite.

**Consequences:**
- When target shows a binary-search `cmpwi/beq/bge` tree and our build
  emits `subi*+cmplwi range`, the switch rewrite is the correct source
  structure — even when the function "looks like" an if/else chain in
  intent. Range checks `x >= A && x < B` are best expressed as explicit
  `case A: case A+1: ... case B-1:` enumeration if they are part of a
  switch-like dispatch.
- For boolean predicates whose target loads `li r3, 0` upfront and uses
  `*lr` conditional returns, write `bool result = false; ... result = true; return result;`
  rather than `if (...) return true; ... return false;`.

## Hypotheses under investigation

### Inline pointer-arithmetic computes per-iteration; hoisted `(this + OFFSET)` reserves a register

**Hypothesis:** Two equivalent source forms for indexing into a field-offset
array generate different MWCC codegen. Form A hoists `this + OFFSET` into a
callee-saved register before the loop; Form B computes `(OFFSET + i*4)`
inline each iteration without a hoist.

```cpp
// Form A — base-pointer cast then array index:
//   addi rH, r3, OFFSET   ; outside loop
//   lwzx r4, rH, rI       ; inside loop (rI = i*4)
TBoundPane* p = ((TBoundPane**)((u8*)this + OFFSET))[i];

// Form B — single-expression pointer arithmetic:
//   addi r0, rI, OFFSET   ; inside loop
//   lwzx r4, r3, r0       ; inside loop
TBoundPane* p = *(TBoundPane**)((u8*)this + OFFSET + i * 4);
```

Form A consumes one extra callee-saved register (extending `stmw`/`stwu`);
Form B uses no extra register but adds one `addi` per iteration.

**Observed:** `GC2D/Guide::checkPoint` — two loops at `_168` and `_44C`.
Form A → Form B closed codegen 95.44% → 99.80% (only phantom stack
padding left).

**Experiment to confirm/refute:** Find another TU with a small loop indexing
a field-offset array where target's loop body shows
`addi r0, rIdx, OFFSET; lwzx`. Test Form B vs Form A; confirm Form B
matches when target uses the inline form.

### Copy-construct on declaration skips the default constructor's body

**Hypothesis:** When a class with a non-trivial default constructor (e.g.
`JUTRect() { set(0, 0, 0, 0); }`) is used as a local, copy-construction
emits only the copy ctor's body, while default-ctor + assignment emits both.

```cpp
// Form A — emits `bl set__7JUTRectFiiii` (default ctor) then `bl copy`:
JUTRect rect;
rect.copy(*src);

// Form B — emits only `bl copy__7JUTRectFRC7JUTRect`:
JUTRect rect(*src);
```

(This is standard C++ semantics, but the matching lever is worth tracking.)

**Observed:** `GC2D/Guide::checkPoint` — two `JUTRect` locals in two loops.
Switching to Form B removed the upfront `bl set__7JUTRectFiiii` call and
contributed to 87.65% → 99.80%.

**Experiment to confirm/refute:** Find another near-100% function where
target lacks a default-ctor call that our build emits. Switch the local's
declaration to copy-construction; confirm the call drops.

### Function returning its pointer argument: declare the return type even when callers ignore it

**Hypothesis:** When the target's epilogue is `mr r3, rN; blr` where rN is
a callee-saved register holding a pointer arg or pointer field, the source
declared the function with that pointer as its return type. A `void`
declaration generates no such `mr`. C++ mangling doesn't encode return
type, so callers that ignore the return value still match.

**Observed:** `GC2D/Guide::setup(JKRMemArchive*)` — target ends
`mr r3, r31; blr` where r31 = saved `archive` arg. Declaring
`JKRMemArchive* setup(JKRMemArchive*)` with `return archive;` closed
77.09% → 100%. Sole caller `MarDirectorDirect` ignores the return value.

**Experiment to confirm/refute:** Find another `void`-declared function
whose epilogue has `mr r3, rN; blr` with rN holding an arg or
field-of-this. Re-declare and add `return rN;` to all paths; confirm the
function moves toward 100%.

### MWCC -O4,p folds `0.0f * (expr)` to constant 0 in some TUs, not others

**Hypothesis:** Under `-O4,p`, MWCC's constant folder eliminates
multiply-by-zero terms (`0.0f * X`) including the entire subexpression
`X`, even when X is non-trivial. The target's compile of the same TU
sometimes PRESERVES the `0.0 * X` term, emitting `fmadds f0, 0.0, X, acc`
as a no-op addition. The trigger appears TU-local, not just expression-
local.

**Symptom (target vs ours), `move__12TSelectShineFv` spline block:**
Source:
```cpp
splineY = 0.0f * (omt * omt) + amp9 * (2.0f * omt * t) + amp * (t * t);
```
Target asm (preserves all terms):
```
fmuls f2, f2, f2       ; f2 = omt^2
... compute amp9 * (2*omt*t) into f0 ...
fmadds f0, f3, f2, f0  ; f0 = 0.0 * omt^2 + acc   <-- preserves the 0 term
fmadds f0, f5, f4, f0  ; f0 = amp * t^2 + acc
```
Our asm (folds away the 0 term and its dead operand):
```
... compute amp9 * (2*omt*t) into f0 ...
fmadds f0, f2, f1, f0  ; only amp * t^2 + amp9*(2*omt*t)   <-- no omt^2 anywhere
```
The entire `omt * omt` computation is eliminated as dead code.

**What I tried that did NOT change folding:**
- Reordering source terms (amp9 first, 0 term last). Same fold.
- All four spline branches show identical fold behavior.

**Experiment to confirm/refute:** Find another GC2D / Player TU where
target asm shows a literal `0.0 * X` survives constant folding under
`-O4,p`. Compare TU-level pragmas / inline directives. If folding
correlates with `-inline deferred` vs `-inline auto`, write a minimal
test to confirm.

Alternatively: try a non-literal zero, e.g. `(amp - amp)` or a `volatile
const f32 z = 0;` — does that preserve the `fmadds 0, X, acc`?

**Cost:** `move__12TSelectShineFv` capped at 83% partly due to this
(plus the bigger out-of-line `bl add__TVec3` issue). Many of the
mismatched instructions in the spline cascade through to register
coloring downstream.

### `addi rN, rM, OFFSET` field-address caching across calls

**Hypothesis:** When a `this`-relative field at a given offset is accessed
≥ 2 times AND at least one of the accesses follows a `bl` call, MWCC
hoists `addi rN, rM, offset` into a non-volatile register and uses
`offset 0` for subsequent loads/stores. This is net-zero or net-negative
on instruction count (the `addi` setup costs +1 vs. using the immediate
offset directly each time), but consumes a callee-saved register and
inflates the stack frame by 4 bytes per cached address.

**Symptom (target vs ours):** Target reads `lwz rA, 0xOFF(rThis)` two
times across a `bl` boundary. Ours does:
```
addi rN, rThis, 0xOFF       ; cache
lwz rA, 0xOFF(rThis)        ; (still uses immediate offset! quirky)
bl ...
lwz rA, 0x0(rN)              ; uses the cache
```
Net: +1 `addi`, +1 callee-save spill, +8 bytes stack (1 reg save + 1
restore + frame padding). Match drops 1-5pp per cached address.

**Where observed:**
- `src/Camera/CameraDemo.cpp::execDeadDemoProc_` — 4 accesses to
  `(this + 0x27c)`; ours caches into r5 (`addi r5, r3, 0x27c`), target
  doesn't. The cache also indirectly forces shouldReturn/firstMatches
  into different registers (r4/r0 vs target's r4/r5), preventing the
  multi-bool `addi r5, r4, 0` copy pattern.
- `src/Camera/CameraDemo.cpp::endDemoCamera` — 3 accesses to
  `(this + 0x2B4)`; ours caches into r3 (`addi r3, r31, 0x2b4`), target
  doesn't. Cost: ~1.5pp.
- `src/Camera/CameraDemo.cpp::updateGateDemoCamera_` — accesses to
  `(this + 0x2B4)` and `(this + 0x70)` both cached (`addi r31, r29, 0x2b4;
  addi r30, r29, 0x70`); target uses just r31 = this. Cost: ~19pp; stack
  grows from 0x30 → 0x40 to spill the extra regs.
- `src/Camera/CameraDemo.cpp::startGateDemoCamera` — `(this + 0x2B0)`
  and `(this + 0x2B4)` cached. Target uses 2 non-volatile regs; ours
  uses 4 (r28-r31). Cost: ~21pp.
- `src/Camera/CameraDemo.cpp::ctrlNormalDeadDemo_` — most aggressive
  case observed: 5 cached field addresses (`&this[0x3c]`, `&this[0x40]`,
  `&this[0x44]`, `&this[0x10]`, `&this[0x280]`) in r26-r29. Uses `stmw r26`
  to save 6 callee-saved registers; target uses just `stw r30, r31`.

**What I tried that did NOT prevent caching:**
- Replacing `void* save = ...; ... void* save2 = ...;` with reassign
  to the same variable.
- Using scoped `{ }` blocks to limit each local's lifetime.
- Adding a typed cast in the second access.

The cache decision appears to be made before instruction scheduling
and isn't affected by source-level variable lifetimes. It correlates
with **number of accesses ≥ 2** plus **at least one access following a
`bl` call** (so a non-volatile register is what's needed to survive
the call).

**Experiment to confirm/refute:** Reduce one of the `(this + OFFSET)`
access pairs to a single access by hoisting the value into a local
variable that's used across both call sites. If the cache disappears,
the trigger is access count. If it persists, the trigger is something
else (maybe address-of computations in the AST?).

Alternatively: write a minimal test where a struct field is accessed
exactly twice across a call. If MWCC caches in our test but target's
version (if reconstructable) doesn't, we may be hitting a compiler
flag difference (`-inline auto` vs `-inline deferred` heuristic?).

**Consequence if true:** Many Camera/ functions are capped 75-95%
because of this single MWCC quirk. If we find the lever, a sweep
could lift dozens of functions at once.

### MWCC sometimes inlines a call selectively within the same TU

**Hypothesis:** Even with `#pragma dont_inline on` set TU-wide (or the
helper otherwise visible only via the same flag environment), MWCC's
`-inline deferred,auto` (or `-inline deferred` for game code) can inline
*some* call sites of a function while keeping others as `bl` calls.

**Where observed:**
- `mario/Camera/CameraMode::isNormalCameraCompletely` — target has the
  first `isNormalCameraSpecifyMode(mMode)` as `bl`, but the second
  `isNormalCameraSpecifyMode(prevMode)` *inlined* as a jump-table
  switch. With our `#pragma dont_inline on`, both calls become `bl`,
  capping the function at 94.3% match.
- `mario/Camera/CameraMode::isTalkCameraInbetween` and `isLButtonCameraInbetween`
  — target inlines the dispatch helper twice (once for `mMode`, once for
  `prevMode`); with `dont_inline on`, both become `bl` and these
  functions stay at 30% / 0%.
- `mario/Camera/CameraMode::isOverHipAttackSpecifyMode` — target keeps
  both helper calls as `bl` (no inlining); without `dont_inline on`, MWCC
  auto-inlines the now-tiny helpers, dropping it to 20%; with
  `dont_inline on`, matches.

**Experiment to confirm:** Determine what source-level cue distinguishes
inline-here from no-inline-there. Candidates:
- The first call is at the top of the function and is the gating
  condition for an early return; the second is buried deeper. Position-
  in-CFG hint to inliner?
- Calls inside an `if (X)` short-circuit chain might inline more readily
  than calls in standalone `if (X) { ... }` blocks.
- Number of remaining call sites at deferred-inlining time (the first
  one may be on a "hot" CFG path; the second on a "cold" path).

**Consequence if true:** When a TU exhibits this pattern, write the
*inlined* call sites with the dispatch logic in-place (explicit switch
or comparison) rather than calling the helper; keep the *non-inlined*
calls as proper helper invocations. This avoids the `dont_inline`
all-or-nothing trap.

### `#pragma dont_inline` is TU-global, not lexical

**Hypothesis:** `#pragma dont_inline on/off` in MWCC 1.2.5 (GameCube) is applied
to every function in the TU using the **final** pragma state seen by the parser,
not lexically scoped around the function definition. Multiple positions of `on`/`off`
in the same file all produced the same compilation outcome — the state at
end-of-file is what stuck.

**Where observed:**
- `src/JSystem/JDrama/JDRDStageGroup.cpp`. Tried wrapping only `perform()` in
  `#pragma dont_inline on ... off`. Either both functions saw `on` (when the
  file ended with `on`) or both saw `off`. Could not pin one function on and
  another off.
- `src/NPC/NpcBase.cpp` (this tick, supporting evidence). Adding
  `#pragma dont_inline on` at top of file made isMadNpc's call to
  `isNormalMonteW` a `bl` (matching target). The previously-100%-matching
  isXxx predicates (which had no calls) were unaffected. The pragma applied
  TU-wide as expected.

**Experiment to confirm a different scope mechanism:** A TU where two
functions need different inlining decisions (one needs `on`, the other an
auto-generated dtor that needs `off`) and we successfully control them
independently using e.g. `__noinline` attribute, declaration trick, or
moving inline source out of header.

**Consequence if true:** When a TU needs both a `dont_inline on` function and an
auto-generated dtor that inlines empty base dtors, **we cannot have both match**
through pragma alone. A different mechanism (e.g. `__noinline` attribute,
declaration trick, or moving inline source out of header) is required.

## Open questions

_Seeded from the "currently-hard patterns" list in `CLAUDE.md` — promote to *Hypotheses
under investigation* the moment you have a testable theory, and to *Settled* once
confirmed in ≥2 TUs._

- **Unreferenced TVec3 `{0,0,0}` and `{1,1,1}` constants in rodata
  on MapEventSink-family TUs.** Symptom: target's
  `mario/Map/MapEventSink.o` rodata has at `.rodata:0xE0` a 12-byte
  zero block (`@2604`) and at `.rodata:0xEC` a 12-byte block of
  three `0x3F800000` (1.0f) values (`@2606`). Same pair appears in
  `MapEventSirena.o` at the same relative positions (`@2585`, `@2587`).
  Neither constant is referenced from `.text` — they're rodata-only.
  Our builds of both TUs don't emit them. Effect: rodata above 0xE0
  shifts 24 bytes earlier in our build, which breaks the `@<zero>+
  0x140` peephole-trick MWCC uses in watch__19TMapEventSirenaSinkFv
  to address subsequent rodata strings. Costs ~3pp on watch and
  similar on parent TU. Mechanism unknown — likely an inline expansion
  that consumes a TVec3 literal then gets fully eliminated, leaving
  only the rodata constant. Investigation candidates: TPlacement
  (and below) initializer-list inline expansion paths, any
  `JGeometry::TVec3<f>(0.f, 0.f, 0.f)` / `(1.f, 1.f, 1.f)` static-
  const or temporary in a header reachable from MapEventSink.hpp.
  Tick 69.
- **Local-static `$localstatic0$<fn>` mangling on a non-inlined function.**
  Symptom: target's `calcTowerCenterPos___15CPolarSubCameraFP3Vec`
  is a 296-byte OUT-OF-LINE function (not inlined into its sole
  caller) yet its local static is mangled with the
  `<varname>$localstatic0$<funcname>` form — the same suffix used
  by symbols inside *successfully-inlined* functions like
  `drawJetCoasterBalloonMessage_`'s `sFixCameraPos`. In our build,
  the same source produces `<varname>$<counter>` (e.g.
  `sPositionNameTable$562`) for a non-inlined function. Adding
  `inline` to the declaration **does** flip mangling to
  `$localstatic0$<fn>`, but it ALSO causes MWCC to fully inline the
  function into its caller — losing the out-of-line copy and
  regressing the caller's match by ~20pp. `#pragma dont_inline on`
  around the definition does not prevent the inlining when `inline`
  is on the declaration. So we have a forced choice between
  *correct symbol mangling* and *correct codegen layout*. What
  source pattern in the target preserves both? Candidates worth
  trying: `__inline` (MWCC-extension keyword?), inline-then-extern-template,
  defining within an anonymous namespace, or some `-inline` flag we
  haven't tested. Affected TUs: CameraNormal (calcTowerCenterPos +
  caller ctrlNormalOrTowerCamera_, ~20-byte rodata/data mismatch).
- **`TTimeRec::startTimer` callers gain +16 bytes of stack frame.** Symptom: any
  function that calls `startTimer__9TTimeRecFv` (or `endTimer`) inflates its frame by
  exactly 0x10. The offending bytes don't correspond to any local in the source. Is
  this an inline-expansion artifact in MWCC's frame allocator? Does the inflation
  disappear if `startTimer` is reconstructed as a non-inline call?
- **`addi rN, rM, 0` vs `mr rN, rM`.** Two ways to encode the same operation; the
  target picks one, our build often picks the other. What source-level cue drives the
  choice? Is it register-class hinting? Lifetime analysis? Has anyone reproduced the
  flip by changing variable order alone?
- **f30/f31 register swap.** In some floating-point-heavy functions, target and ours
  agree on every instruction but the two callee-saved FPRs are swapped. Likely an
  interaction between expression order, live-range overlap, and MWCC's allocator
  preferences — but the exact lever is unknown.
- **Redundant field reloads under inline expansion.** When an inline accesses a field
  twice, MWCC sometimes emits two loads where common-subexpression elimination should
  have collapsed them. What inhibits CSE here? Is it the inline's parameter aliasing
  assumption? A `this`-pointer barrier?
- **Block ordering in boolean-return functions.** Compiler picks fall-through-through-true
  vs fall-through-through-false. The choice may correlate with branch-prediction hints,
  expression nesting, or the position of the `return` in the source AST. Reduce to a
  rule by varying source structure on a minimal test case.

## Refuted / wrong turns

_(empty — populate when a former hypothesis is disproved)_

---

## Conventions for entries

- **Lead with the rule, not the story.** "MWCC does X when Y" is more useful than "I
  spent 30 min on Z and figured out X". Save the story for the journal.
- **Cite specific symbols.** `SMS_IsMarioOnWire__Fv`, `flip__8TTimeRecFv`, etc. —
  fully mangled names so anyone (human or future agent) can grep the codebase.
- **Quantify when you can.** "+16 bytes of phantom stack inflation" beats "extra
  stack". "lwz then lwz at the same offset" beats "two loads".
- **Distinguish confirmed from suspected.** In _Settled_, only put things observed
  in two or more places. In _Hypotheses_, lead with "Hypothesis:".
- **Link related entries.** When extending an entry, cross-reference rather than
  duplicate. Use `(see also: <heading>)`.
- **Date entries are not needed** — git history serves that purpose. The structural
  order (newest first within each section) gives readers the sense of "what's been
  on the bot's mind recently".
