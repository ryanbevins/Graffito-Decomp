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
