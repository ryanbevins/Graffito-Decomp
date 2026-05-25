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

### Two-term sum `unk3C * sinf(...) + unk40 * sinf(...)` — split each term into a named local to defeat fused fmadds

**Rule.** When target asm computes a sum-of-two-products as

```
fmuls fX, fA, fSin1       ; X = unk3C * sin1 (saved to NV-FPR)
... second sinf call ...
fmuls f0, fB, fSin2        ; 0 = unk40 * sin2
fadds f1, fX, f0          ; final = X + 0
```

but our build collapses the second multiply-add into a single
`fmadds f1, fB, fSin2, fX`, force separation by giving **both**
products an explicit local name:

```cpp
// BEFORE — MWCC fuses the second multiply into fmadds:
return unk3C * sinf(unk24 * (invTwoPi * x) + unk64)
     + unk40 * sinf(unk28 * (invTwoPi * z) + unk68);

// AFTER — separate fmuls + fadds:
f32 wave1 = unk3C * sinf(unk24 * (invTwoPi * x) + unk64);
f32 wave2 = unk40 * sinf(unk28 * (invTwoPi * z) + unk68);
return wave1 + wave2;
```

**Why.** With `a + b*c` MWCC's peephole sees a fused multiply-add
opportunity and emits `fmadds`. Naming `b*c` as a local (`wave2`)
materializes the multiplication as a distinct subexpression that
must be stored to its named slot first; the final `+` then has no
multiply opportunity left to fuse.

**Important:** naming only ONE term (e.g. `wave1`) is insufficient
— MWCC still fuses the unnamed second product. Both must be named
locals.

**Citations.**

- `MoveBG/MapObjWave::getWaveHeight` (tick 106): wave1+wave2 split
  took 94.69% → **100%**. Target uses `fmuls f31, f4, f1; ... bl
  sinf; fmuls f0, f0, f1; fadds f1, f31, f0`. Single-named wave1
  alone left fmadds (96-ish%); naming both wave1+wave2 cleaned the
  full sequence.
- `MoveBG/MapObjWave::getHeight` (tick 106): same split, 73.45 →
  78.37%. Side-benefit: also dropped a spurious `+ height` term
  and reduced FPR saves from 4 to 3 (stack 0x48 → 0x40).
- `MoveBG/MapObjWave::draw` (tick 106): split applied to both
  `y0` and `y1` inner-loop sin sums, 89.12% → 93.73%.

**Where to try it next.** Any function computing `coeff_a * f(...) +
coeff_b * g(...)` where target asm has `fmuls + fadds` but ours has
`fmadds`. Common in physics integrators, transform compositions, any
2-term linear blend. Even more likely if both terms are sin/cos.

### Inverted `||` form for shared-true assignment dedups duplicate assignment blocks

**Rule.** When source has the pattern "in two separate cases, run
the same assignment block; in the remaining case, run the ratio
branch", do **not** write it as nested `if-else` with duplicated
assignment blocks, and do **not** write it as `if (cond_a &&
cond_b) { ratio } else { assign }`. Target uses the
**disjunctive** form with the "either case" combined via `||`:

```cpp
// BEFORE — duplicates the assign block, generates 3 basic blocks
// (assign-inline, ratio, assign-fallthrough):
if (dist >= 0.0f) {
    if (bgType == 0x700) {
        unk3C = unk2C; unk40 = unk30;    // assign
    } else {
        ratio compute;                   // ratio
    }
} else {
    unk3C = unk2C; unk40 = unk30;        // assign (duplicate)
}

// AFTER — single assign block, target's exact basic-block layout:
if (dist < 0.0f || bgType == 0x700) {
    unk3C = unk2C; unk40 = unk30;        // assign (ONE place)
} else {
    ratio compute;                       // ratio
}
```

**Why.** The disjunctive `||` form short-circuits: `blt assign;
cmplwi bgType, 0x700; beq assign; b ratio; assign: ...; b end;
ratio: ...; end:`. The duplicated-assign form lays out two
separate assignment blocks, increasing function size and producing
a `bne` (combined `cror eq, gt, eq` for the `>= AND ==` test)
which is wrong shape. Note the **branch direction is inverted**:
`>=` becomes `<`, `==` becomes equality short-circuit to true
side, etc.

**How to apply.** Look at the target asm's first comparison after
the cond setup. If it's `blt label` jumping to the assignment
block, the source must start with `dist < 0.0f || ...` (the
disjunction with the FAIL side of `>=`). If it's `bge`, source
starts with `dist >= 0.0f || ...`.

**Citations.**

- `MoveBG/MapObjWave::updateHeightAndAlpha` (tick 106): rewriting
  the dist+bgType branch as `if (dist < 0.0f || bgType == 0x700) {
  assign } else { ratio }` (instead of the original `if (dist >=
  0.0f && bgType == 0x700) { assign } else { ratio }`) jumped from
  82.37% → 87.57%. The original used wrong logic (matching the
  WRONG branch as the assign side) AND wrong shape. Same fix
  applied to the alphaDist branch.

**Where to try it next.** Any function with target asm pattern
`fcmpo + blt asgn_label` followed by a second comparison whose
match-equal branch ALSO jumps to `asgn_label`. The shared label is
the smoking gun for `cond_a || cond_b`. Also flag any "elseif
chain with duplicated body" — likely should be `||`.

### Static-init order is reverse-of-include-order under `-inline deferred`

**Rule.** In a TU compiled with `-inline deferred`, the order MWCC
emits per-template-instantiation `__sinit` blocks is **reverse** of
the order in which the templates' classes are first declared
(typically by header includes). To make `JALList<X>::smList` (or
any other template static) initialize **first** in `__sinit`, move
the header that declares class `X` to the **last** position in the
include block.

```cpp
// BEFORE — JALList<MSBgm>::smList initialized LAST in __sinit
#include <MSound/MSoundBGM.hpp>    // declares MSBgm (first)
#include <MSound/MSSetSound.hpp>   // declares MSSetSoundGrp/MSSetSound
#include <JSystem/JAudio/JALibrary/JALModSe.hpp>   // declares JALSeMod* (last)

// AFTER — JALList<MSBgm>::smList initialized FIRST in __sinit
#include <MSound/MSSetSound.hpp>
#include <JSystem/JAudio/JALibrary/JALModSe.hpp>
#include <MSound/MSoundBGM.hpp>    // moved to last → its template fires first
```

**Why.** With `-inline deferred`, MWCC defers compilation of
template instantiations until after the TU body is parsed, then
processes them in reverse declaration-encounter order. The result
is that the LAST template referenced gets emitted FIRST in
`__sinit`.

**How to verify.** If `__sinit` is < 100% match with the diff
showing template names shifted in a rotation pattern (e.g. our
build's first template is target's second, target's last is ours
N-1, etc.), the order is rotated. Move the "missing-from-start"
template's header to the end of the include block.

**Citations.**
- `MSound/MSModBgm` (tick 104): moving `<MSound/MSoundBGM.hpp>` from
  position 2 to position 5 (last) of the include block flipped
  `JALList<5MSBgm>::smList` from `__sinit` end to start. `__sinit`
  97.64% → 100%, TU 91% → 97.2% fuzzy. Confirmed for a TU with
  `-inline deferred` set via the `MSound` lib block in `configure.py`.

**Where to try it next.** Any TU with `__sinit` matching < 100%
where the diff shows a one-off rotation in template instantiation
order. Common in MSound, Player, Strategic, System TUs (all
`-inline deferred` libs per `configure.py`).

### Bit-mask test (`if (v & MASK)`) vs extract-then-test (`if (b != 0)`) — prefer mask for direct CR0-only test

**Rule.** When target tests a byte/bitfield non-zero with `clrrwi.`
or `rlwinm.` (mask the bits, set CR0, branch) and our build emits
`srwi.`/`extrwi.` (shift/extract into register, set CR0, branch), the
source should test the mask directly rather than naming the extracted
byte first. Move the byte extraction inside the if-body where it's
actually used.

```cpp
// BEFORE — extract then test (emits extrwi. r5, r4, 8, 8):
u8 b2 = (v >> 16) & 0xff;
if (b2 != 0) {
    use(b2 + 1);
}

// AFTER — mask test only (emits rlwinm. r0, r4, 0, 8, 15):
if (v & 0x00FF0000) {
    use(((v >> 16) & 0xff) + 1);  // extraction deferred into body
}
```

Same for the high byte:

```cpp
// BEFORE — `srwi. r5, r4, 24`:
u8 b3 = (v >> 24) & 0xff;
if (b3 != 0 && otherCond) {
    u32 mod = b3 + 1;
    ...
}

// AFTER — `clrrwi. r0, r4, 24`:
if ((v & 0xFF000000) && otherCond) {
    u32 b3  = v >> 24;
    u32 mod = b3 + 1;
    ...
}
```

**Why.** Extract-form keeps the extracted value live in a register
across whatever follows, which constrains MWCC's allocator. Mask-form
only sets CR0 (the dot-suffix), leaving registers free; MWCC can
schedule the lazy extraction independently when the body needs it,
often producing better register coloring downstream.

**Citations:**

- `MSound/MAnmSoundNPC::startAnimSound` (tick 102): both b2 and b3
  tests rewritten to mask-form; 70.3% → 72.9%. Target asm: `clrrwi.
  r0, r4, 24` for high byte, `rlwinm. r0, r4, 0, 8, 15` for middle
  byte. Our extract-form was emitting `srwi. r5, r4, 24` and `extrwi.
  r5, r4, 8, 8` respectively.

(Second citation pending — applies wherever target's mask-test asm
matches the extract-form C source. Sweep targets: any `u8 b = (x >> N)
& 0xff; if (b != 0)` pattern where the value is only used once inside
the if.)

### Zero-fmadds rotation pattern: explicit `+ 0 * sin/cos` terms force fmadds with zero addend

**Rule:** When target's asm computes a 2D rotation as

```
lfs   f0, @zero@sda21     ; f0 = 0.0
fmuls f2, f0, fSin        ; f2 = 0 * sin (zero addend)
fmuls f0, f0, fCos        ; f0 = 0 * cos (zero addend)
fmadds f2, fA, fCos, f2   ; f2 = a * cos + (0 * sin)
fmadds f0, fB, fSin, f0   ; f0 = b * sin + (0 * cos)
```

instead of the simpler `fmuls f2, fA, fCos; fmuls f0, fB, fSin`, the
source has explicit `+ 0 * sin/cos` terms. Write the source as:

```cpp
f32 zero = 0.0f;
*outX = a * cosV + zero * sinV;
*outZ = -b * sinV + zero * cosV;
```

(or `pos.x += a * cosV + zero * sinV;` if accumulating into a Vec3).
MWCC does NOT constant-fold the `0 * x` away when written this way;
instead it emits both the zero-multiply and the fmadds, exactly
matching target's instruction sequence.

This is a real lever for matching the rotation-style code that
appears in chain physics (TSphereLink), particle systems, and any
"rotate vector by Z angle" helper that ends up looking like a
general 2D rotation matrix.

**Why:** MWCC sees the source `a * cos + 0 * sin` and treats the
multiplication of a runtime variable (sin) by literal 0 as a real
floating-point op (since 0 * NaN != 0 etc.). The constant-fold
pass keeps it. The `fmadds` opcode (a*b + c) is the natural way to
write `a*cos + (0*sin)` since the 0*sin term has already been
computed into a register.

**Citations:**
- `Enemy/BossHanachanSub.cpp::BHSCalcRevisionDistXZByRotateZ` —
  89.74% → 92.60% (tick 94). Pattern: `*outX = c * cosV + zero * sinV;`,
  `*outZ = -c * sinV + zero * cosV;`. Bare `*outX = c * cosV;` produces
  just fmuls with no fmadds — won't match.
- `Enemy/BossHanachanSub.cpp::setDegreeZAndRevisionPosXZ` —
  71.07% → 79.59% (tick 94). Same `+ zero * sinV/cosV` pattern in
  the position-update at function end. Combined with manual MsWrap
  expansion (separate lever) for the larger gain.

**Where to try it next:** Any rotation calculation in chain physics,
2D rotation helpers in particle systems (e.g., effect TUs), or any
`pos.x += a * cosV; pos.z += b * sinV` pair where target emits
fmadds instead of fmuls. Inspect asm for `lfs frX, @zero@sda21`
followed by `fmuls frY, frX, frSin` — that's the signature.

### Manual expansion of `MsWrap` while loops defeats forced bl-emit

**Rule:** When `MsAngleWrap(angle)` (which calls inline template
`MsWrap<f>(angle, 0.0f, 360.0f)`) gets emitted as `bl MsWrap<f>__Ffff`
in our build but target inlines the while loops, manually inline
the body in source:

```cpp
// Before — emits bl MsWrap<f>__Ffff + weak instance:
baseAngle = MsAngleWrap(baseAngle);

// After — inlined while loops match target:
while (baseAngle >= 360.0f)
    baseAngle -= 360.0f;
while (baseAngle < 0.0f)
    baseAngle += 360.0f;
```

MWCC inlines the manually-written while loops but apparently chose
not to inline the template instance through `MsAngleWrap` →
`MsWrap<f>`. Manual expansion produces identical inlined code to
target's preference and removes the weak `MsWrap<f>__Ffff` symbol
from the TU's .text.

**Citations:**
- `Enemy/BossHanachanSub.cpp::setDegreeZAndRevisionPosXZ` —
  71.07% → 76.81% (tick 94, before zero-fmadds polish). The remaining
  `bl MsWrap<f>__Ffff` and the weak `MsWrap<f>__Ffff` body in our TU
  both went away. Target's setDegreeZ has the wrapping while loops
  inlined at the same location.

**Where to try it next:** Any TU showing a weak `MsWrap<f>__Ffff` or
`MsWrap<l>__Flll` symbol in its `.text` while target asm has neither.
Common in nerve-execute functions that wrap an angle and in chain
physics that wrap rotation degrees.

### Weak destructor emission in MarNameRefGen TUs requires correct base class + inline derived ctor

**Rule:** When a TU emits a `bl __ct__Foo` for `new Foo(...)` (where
`Foo` has out-of-line ctor declared but inline-defined body, e.g. in
its header as `Foo(const char* name) : Bar(name) {}`), MWCC also
emits `__dt__FooFv` as a weak symbol in that TU. The inline ctor's
visible inheritance is the trigger — MWCC needs the parent's vtable
slot for the derived `Foo`'s vtable, which forces emission of the
derived dtor weak.

Conversely, when `Foo` is a header-declared class whose ctor is
out-of-line (declared `Foo(const char*);` only) AND `Foo` has no
derived classes with inline ctors in the current TU, MWCC will NOT
emit `__dt__FooFv`. The call `bl __ct__Foo` and any vtable refs
become external dependencies.

**To trigger weak emission of a base class's dtor:** add a derived
class with inline ctor in the same TU. The simplest pattern is a
local placeholder class:

```cpp
class TMewManager : public TAnimalManagerBase {
public:
    TMewManager(const char* name) : TAnimalManagerBase(name) {}
};
// ... and use it for `new TMewManager(...)`.
```

If the derived class exists in a header but with an out-of-line
ctor, promote the ctor to inline body in the header (verify
symbols.txt has no `__ct__Derived` symbol first — if absent, the
original code was inline).

**Important:** if the local class uses `: TSpineEnemy(name) {}` but
the TARGET's asm calls `bl __ct__11TSmallEnemyFPCc` after a vtable
write to TSmallEnemy, the placeholder must inherit TSmallEnemy
(NOT TSpineEnemy). The vtable cascade in the dtor reveals the
correct base chain — read the dtor's `lis r3, __vt__X@ha` sequence.

**Citations:**
- `System/MarNameRefGen_BossEnemy` — TBEelTears dtor emitted by
  switching TOilBall (TNameRef → TBEelTears) with inline ctor. TU
  88.21% → 93.54% in tick 87, then 93.54% → 96.41% in this tick.
- `System/MarNameRefGen_Enemy` — TPakkun dtor 81.82% → 100% by
  changing base from TSpineEnemy → TSmallEnemy (match dtor cascade).
- `System/MarNameRefGen_Enemy` — TTobiPuku family (4 dtors) +
  MoePuku derived chain. TU 87.35% → 89.60%.
- `System/MarNameRefGen_Enemy` — TNameKuriManager dtor via inline
  TDiffusionNameKuriManager ctor promotion (NameKuri.hpp). TU →
  90.83%.
- `System/MarNameRefGen_Enemy` — TGesso dtor via inline TSurfGesso
  / TLandGesso ctors (Gesso.hpp). TU → 92.72%.
- `System/MarNameRefGen_Enemy` — TAnimalManagerBase dtor via inline
  TMewManager ctor (AnimalManager.hpp). TU → 93.79%.
- `System/MarNameRefGen_Enemy` — TSimpleEffect dtor via inline
  TSimpleEffect/TEffectPinnaFunsui/TEffectBiancoFunsui ctors
  (EffectObj.hpp). TU → 96.96%.

**Where to try it next:** Every other `MarNameRefGen_*` TU
(MarNameRefGen_MapObj, _NPC, _Map) has many missing weak dtors with
the same pattern. Survey the strcmp branches → for each `new T(...)`
where `T` has a header-out-of-line ctor + no derived class in TU,
check target asm for which derived classes are constructed (look for
`bl __ct__TBase` immediately followed by `lis r3, __vt__TDerived`).
Add a local placeholder OR promote header ctor to inline.

### `TVec3<f32>::set<f32>(x, y, z)` template form batches float loads

**Rule:** When source needs to copy three float values from a base
pointer (e.g. `mtx[i][3]`) into a `TVec3<f32>` member, writing it as

```cpp
mTipPos.set<f32>(mtx[0][3], mtx[1][3], mtx[2][3]);
```

(template `set` with **explicit `<f32>` argument**) produces the
batched pattern:

```
lfs f2, 0x2c(rN)    ; load z first (reverse-order load)
lfs f1, 0x1c(rN)    ; load y
lfs f0, 0xc(rN)     ; load x
stfs f0, X+0(rDest) ; then batched stores
stfs f1, X+4(rDest)
stfs f2, X+8(rDest)
```

The three lfs go to f0/f1/f2 separately so MWCC's scheduler can
interleave them with surrounding ALU work, and stores follow in
order. This contrasts with the **component-by-component** form

```cpp
mTipPos.x = mtx[0][3];
mTipPos.y = mtx[1][3];
mTipPos.z = mtx[2][3];
```

which compiles to interleaved `lfs f0; stfs f0` x3 using a single
FPR, with NO scheduler reordering. Target's batched form is the
common case when 3 floats are copied from a base pointer.

**How to identify:** Target asm shows 3 `lfs frX, off(rN)` to three
different FPRs (f0/f1/f2 typically), in reverse offset order, then
3 `stfs frX, off(rDst)` in normal order. Component-by-component
source produces a single FPR (`lfs f0; stfs f0` x3) and won't
match.

**Why the `<f32>` template arg matters:** `TVec3<f32>::set` is a
template:
```cpp
template <class TY> void set(TY x_, TY y_, TY z_) {
    x = x_; y = y_; z = z_;
}
```
Without the explicit `<f32>`, deduction picks `TY` from arg types.
The 3-arg `set()` overload (non-template `set(f32, f32, f32)`)
does NOT exist — only the template. So `mTipPos.set(a, b, c)`
deduces `TY=f32` and produces the same template instance as the
explicit form. The two forms SHOULD be identical — pending check
of whether the explicit-template-arg form is necessary or just a
stylistic preference. (Open question.)

**Caveat — supersedes CLAUDE.md claim:** CLAUDE.md "TVec3 / Vector
Codegen Patterns" says ".set(x, y, z) (3-arg form) — generates
lfs/stfs like component assignment". That is INCORRECT for the
mtx-load case. The 3-arg template `set` produces BATCHED loads,
not interleaved. The component-by-component form is what produces
interleaved loads. Test with both before committing.

**Citations:**
- `Enemy/DebuTelesa.cpp::calcRootMatrix` — 83.82% → **99.88%**
  (tick 82). Switched `mTipPos.x = mtx[0][3]; mTipPos.y = ...;
  mTipPos.z = ...;` to `mTipPos.set<f32>(mtx[0][3], mtx[1][3],
  mtx[2][3])`. Combined with `getCurrentNerve` → `getLatestNerve`
  fix that restored 4 inlined-fallback instructions. Remaining
  0.12% is just stack-temp slot allocation order (different
  unsolved MWCC quirk).

**Where to try it next:** Any `TVec3` member being filled from
three matrix-cell or struct-field reads using component assignment,
where target shows batched lfs to multiple FPRs. Especially in
`calcRootMatrix`, position-emit, and matrix-derived position
patterns across the Enemy module.

### `bool` return type (not `BOOL`) for pointer-nonnull tail when target lacks `clrlwi` narrow

**Rule:** When a function's tail in target asm is the branchless
pointer-to-int idiom

```
lwz   r0, OFF(rThis)
neg   r3, r0
subic r0, r3, 0x1
subfe r3, r0, r3        ; result directly in r3 — NO clrlwi
blr
```

the source returns **`bool`**, not `BOOL`. Declaring the function
as `BOOL` makes MWCC route the bool intermediate (`p != nullptr`)
through a clrlwi-narrowing step:

```
subfe r0, r0, r3        ; into r0
clrlwi r3, r0, 24       ; narrow into r3  <-- extra insn
blr
```

The C++ semantics are identical (both return 0/1), but the
return-type's "width semantics" controls whether MWCC bothers
narrowing the bool intermediate before placing it in the
return-value register.

**Mirror rule:** When target's tail HAS the `clrlwi r3, r0, 24`
(e.g. `beakHeld__10TBossGessoCFv`), the source returns `BOOL`.
Both forms exist in the original — pick the one whose tail matches.

**Why:** MWCC treats `bool` as "byte-wide" but propagates that bit
directly when the destination is also bool-typed. Casting `bool` →
`BOOL` (int) inserts an explicit "narrow-to-byte then zero-extend"
sequence (the clrlwi). When the destination is `bool`, no
narrow needed — the subfe writes directly to r3.

**How to identify:** look for any function currently declared
`BOOL` that returns a runtime bool expression (`p != nullptr`,
`x != 0`, `(BOOL)(...)` cast on bool). Diff vs target — if target's
tail is `subic; subfe r3, r0, r3; blr` (no clrlwi), flip the
declared return type to `bool` in both header and `.cpp`.

**Citations:**
- `Enemy/BathtubBinder.cpp::init` — 98.58% → **100%** by changing
  `BOOL init(...)` to `bool init(...)`. Body was
  `return (BOOL)(mBathtub != nullptr);`. Removed clrlwi.
- `Enemy/Amenbo.cpp::isCollidMove` (already matching) — declared
  `bool`, body `return param_1 != this;`. Target tail is
  `subic; subfe r3, r0, r3; blr` — confirms the rule.
- `Enemy/bossgesso.cpp::beakHeld` (already matching) — declared
  `BOOL`, body `return !!mBeak->mHolder;`. Target tail HAS
  `clrlwi r3, r0, 24` — confirms the mirror.

**Where to try it next:** any other `BOOL fn() { return expr;`
where `expr` is a bool-typed runtime expression (pointer-nonnull,
inequality test) currently sitting near-100% with a single
trailing clrlwi diff.

**Caveat:** Only applies when the bool expression is computed at
runtime. Functions whose body is `return true;` / `return false;`
already emit `li r3, 0x1` / `li r3, 0x0` without any narrow, so
the return type doesn't matter for matching there.

### Explicit `Base::method()` qualifier in fabricated forwarders suppresses virtual dispatch

**Rule:** A fabricated forwarder method like

```cpp
TFooParams* getSaveParam2() const {
    return (TFooParams*)TSpineEnemy::getSaveParam();   // direct call
}
```

is compiled by MWCC to a single `bl getSaveParam__11TSpineEnemyCFv`.
Dropping the explicit qualifier:

```cpp
TFooParams* getSaveParam2() const {
    return (TFooParams*)getSaveParam();                // virtual call
}
```

restores virtual dispatch and emits the 4-instruction vtable
sequence `lwz r12, 0(this); lwz r12, VTOFFSET(r12); mtlr r12; blrl`.

This is just C++ semantics (explicit qualification suppresses
virtual), but in matching contexts it's the most impactful one-line
lever found this quarter: many "fabricated" save-param accessors
across the Enemy module use the qualifier-disabled form while target
asm uses the virtual form.

**Secondary effect — CSE behavior flips:** MWCC treats direct calls
to `const` methods as pure and CSE's repeated calls in the same
expression. Virtual calls are NOT CSE'd (the dispatch could differ).
So `getSaveParam2()->a + getSaveParam2()->b` becomes 1 call with
direct, 2 calls with virtual. If target shows the 1-call form
(direct + CSE) but the caller you're editing now has multiple
calls (post-flip), cache to a local:

```cpp
TFooParams* params = getSaveParam2();
x = params->a + params->b;
```

**Citations:**
- `Enemy/launcher::resetLaunchTimer` 82.86 → **100%** (tick 76). Same
  4-line vtable diff vanished. Plus init, stateLaunch, stateDie,
  CommonLauncher::stateDie all reached 100%.
- `Enemy/walkerEnemy` TU 97.75 → **99.71%** (tick 76). moveObject,
  reset, isResignationAttack, behaveToFindMario, multiple
  TNerveWalker::execute fns improved.
- `Enemy/smallEnemy` TU 90.99 → **92.91%** (tick 76). init 78→92%,
  reset 65→77%. Two callers needed the local-cache fix per CSE
  caveat above.

**Where to try it next:**
- Any other `(TFoo*)TBase::method()` forwarder in `include/`.

**Don't apply if** target asm shows the direct `bl funcName` form
(no vtable lookup). Verify before flipping.

**Refuted on Hinokuri2** (tick 78): Hinokuri2.hpp:165 had a forwarder
named `getSaveParam` (same name as the base virtual). Renamed to
`getSaveParam2`, dropped qualifier, updated 29 call sites in
hinokuri2.cpp + 6 in the header. **Net regression:** TU 99.23% →
99.18%. Per-fn breakdown: 3 functions gained (perform_Mask +0.18,
moveObject +0.05, execute_PrePol +0.79), 6 functions lost (reset
-0.08, execute_GraphWander/Pollute/Damage -0.04 each,
execute_Burst -2.46, execute_Stamp -0.07). Reverted. The Burst
regression came from register-coloring reorder (r30↔r31 swap),
not from CSE/local-cache issues — caches wouldn't fix it. Conclusion:
in TUs with very many call sites (~30+) and complex nerve-execute
schedules, the qualifier-drop's NV-register-allocation side effect
becomes a coin-flip per function. Apply this only where ALL or
MOST callers visibly need virtual dispatch AND you've audited
each affected function for register-order survivability — or skip
the TU entirely. Hinokuri2 is now in the "skip" category.

---

### `BOOL` (typedef int) return type on inline helpers avoids `clrlwi.` narrowing and changes instruction scheduling

**Rule:** Changing an `inline` helper's return type from `bool` to
`BOOL` (typedef int) eliminates MWCC's bool-narrowing cast at the
caller and can simultaneously re-schedule surrounding loads. Two
distinct symptoms appear together:

1. **Bool-narrowing test:** When the caller does
   `if (inlineHelper(...)) { ... }`, target emits `cmpwi rN, 0x0; beq`
   after the helper's final `mfcr/extrwi` sequence; our `bool`-return
   version emits `clrlwi. rN, rN, 24; beq` (clear-left-24 with record).
   Both work but only the `cmpwi` form matches target.
2. **Instruction-scheduling side-effect:** Changing the return type
   *also* lets MWCC delay one or more `lfs` loads past intervening
   `fsubs` operations. Confirmed on `insideCylinder` (areacylinder):
   the `lfs f0, 0x18(r3)` load for `cyl->unk18` moved from BEFORE
   `fsubs f2, f3, f2` (dx computation) to AFTER it — matching target
   schedule — purely by flipping `bool` → `BOOL` on the helper.

**How to identify the target pattern:**
- Target asm shows `cmpwi rN, 0x0; beq` after `mfcr; extrwi rN, rN, 1, 2`.
- Our build shows `clrlwi. rN, rN, 24; beq` at the same position.
- Helper currently declared as `bool` and called from `if (helper(...))`.

**Source that matches (BOOL form):**
```cpp
static inline BOOL insideCylinder(...) {
    if (cond) return FALSE;
    ...
    return (someCmp);  // bool->BOOL implicit conversion
}
```

**Where observed:**
- `Enemy/areacylinder.cpp::contain__20TAreaCylinderManagerFRCQ29JGeometry8TVec3<f>`
  — 94.72% → **99.82%** by changing `static inline bool insideCylinder(...)`
  to `static inline BOOL insideCylinder(...)` and `return false;` to
  `return FALSE;`. Both the `clrlwi.` → `cmpwi` flip AND the `lfs f0, 0x18`
  scheduling flip happened together.
- `Enemy/areacylinder.cpp::getCylinderContains__20TAreaCylinderManagerFRCQ29JGeometry8TVec3<f>`
  — 94.68% → **99.88%** by the same change to the same helper.

**Mechanism (hypothesis):** MWCC's bool type triggers an extra
"narrow to bit" semantic at every implicit `bool` → `int` conversion
point. The narrowing inserts a `clrlwi.` *and* counts as an extra
node in the dependency graph, perturbing the instruction scheduler's
choice for surrounding `lfs` loads. `BOOL` (typedef int) skips both.

**Where to try it:** Any inline helper currently returning `bool`
whose target asm shows `cmpwi rN, 0x0; beq` after `mfcr/extrwi` while
our build shows `clrlwi. rN, rN, 24; beq`. Especially common in
small predicate helpers called from `if (helper(...))` loops.

### Disjunction (`||`) merges two `return false` early-exits into one shared fail block

**Rule:** When a helper has multiple sequential guard clauses each
producing `return false`, MWCC emits a separate `li rN, 0; b end`
fail block for each. Merging the guards into a single `||`
disjunction produces ONE shared fail block reached by `blt`/`bge`
from each compare — matching target's typical schedule for inline
predicate helpers.

```cpp
// ours: 2 fail blocks
if (cond1) return false;
if (cond2) return false;

// target shape: 1 shared fail block
if (cond1 || cond2) return false;
```

**Where observed:**
- `Enemy/areacylinder.cpp::insideCylinder` y-range check — 90.6% → 94.7%
  on the two outer functions (contain, getCylinderContains) by merging
  `if (pos.y < cyl->unk14) return false; if (cyl->unk14 + cyl->unk20 < pos.y) return false;`
  into `if (pos.y < unk14 || unk14 + unk20 < pos.y) return false;`.

**How to identify:** Target shows `blt fail_label` falling straight
into the next test, where `fail_label` is the SAME `li rN, 0; b end`
block reached by both `blt` from the first test AND `bge` from the
second. Our build has two distinct fail blocks instead.

### Bind a global pointer to a typed local before a method call to emit `lwz rTMP; mr r3, rTMP` instead of `lwz r3, gFoo@sda21`

**Rule:** When source writes `Foo::sInstance->method(args);` directly,
MWCC emits `lwz r3, sInstance@sda21; bl method` — loading the global
straight into the arg register. Two source patterns produce the
*indirect* form `lwz rTMP, sInstance@sda21; mr r3, rTMP; bl method`
(one extra `mr` instruction):

1. **Explicit typed local:** `Foo* p = Foo::sInstance; p->method(args);`
2. **getInstance() wrapper:** `Foo::getInstance()->method(args);`
   when `getInstance()` is an inline `static T* getInstance() { return sInstance; }`.

As a *secondary effect* the first lever frequently also flips the
encoding of the preceding `this`-save from `mr r31, r3` to
`addi r31, r3, 0x0`. The two effects usually appear together — when
the target uses `addi rN, r3, 0` for the this-save AND has the extra
`mr r3, rTMP` pattern, declare the local.

**How to identify the target pattern:**
- Target asm contains `lwz rN, gFoo@sda21; mr r3, rN; bl method` where
  `rN` is *not* r3 (typically r0 or another scratch).
- For the typed-local lever, target's stack-prolog uses
  `addi r31, r3, 0x0` rather than `mr r31, r3` to save the implicit `this`.

**Source that matches (typed-local form):**
```cpp
TFlagManager* fm = TFlagManager::smInstance;
if (fm->getBool(0x10384)) { ... }
```

**Where observed:**
- `Map/MapEventDolpic.cpp::watch__22TDolpicEventBiancoGateFv` — 88.93% → **100%**
  by declaring `TFlagManager* fm = TFlagManager::smInstance;` before the
  `if (fm->getBool(0x10384))`. Both the extra `mr r3, r0` and the
  `mr r31, r3` → `addi r31, r3, 0x0` flip happened in lockstep.
- `GC2D/CardLoad.cpp::setupScoreScreen__9TCardLoadFv` — already matches
  via the `getInstance()` form (`TFlagManager::getInstance()->getFlag(0x40000)`),
  emitting `lwz r0, smInstance@sda21; mr r3, r0; bl getFlag` as expected.
  Confirms the rule from the second source pattern.

**Caveat 1 — context dependent:** Watch__Ricco in the same TU uses
`if (!TFlagManager::smInstance->getBool(unk2C))` with target emitting
the *direct* `lwz r3, smInstance@sda21` pattern — no local needed.
Only apply when the target's asm shows the indirect `lwz rN; mr r3, rN`
shape.

**Caveat 2 — the addi/mr flip is a side-effect, NOT a control knob.**
The typed-local extraction couples `mr r31, r3 → addi r31, r3, 0` to
the lwz-indirect change. They flip together. Some target functions
have `lwz r0; mr r3, r0` AND `mr r31, r3` (the standard `this`-save).
Applying the local-extract lever to such a function will flip the
`mr r31, r3` to `addi r31, r3, 0` — *regressing* the encoding. Verify
the target's `this`-save form before applying:
- target `addi r31, r3, 0x0` + indirect-lwz → apply lever
- target `mr r31, r3` + indirect-lwz → SKIP (lever breaks encoding)

Observed in `JSystem/JAudio/JALibrary/JALModSe.cpp::processModDistVolume`:
the indirect lwz was already produced *without* the local. Adding the
typed local extraction regressed match from 97.56% → 95.12% by
flipping `mr r31, r3` → `addi r31, r3, 0x0`. Reverted.

**Applied successfully** (committed wins this rule has produced):
- `Map/MapEventDolpic.cpp::watch__22TDolpicEventBiancoGateFv` 88.93→100%
- `MoveBG/Item.cpp::taken__9TCoinBlueFP9THitActor` 96.63→99.86%
- `MoveBG/MapObjMare.cpp::calc__9TMareFallFv` 93.44→99.90%
- `Enemy/smallEnemy.cpp::changeOut__11TSmallEnemyFv` 89.85→95.49%
- `Enemy/hamukuri.cpp::attackToMario__13THaneHamuKuriFv` 88.75→99.82%
- `Player/MarioSound.cpp::startSoundActor__6TMarioFUl` 90.72→99.72%
- `System/Application.cpp::checkAdditionalMovie__12TApplicationFv` 99.06→99.93%
  (this one extracted a struct field, not a global — the lever generalises
  to any single-use value that flows into an arg register)
- `GC2D/Guide.cpp::resetObjects__6TGuideFv` 89.89→92.50%
  (partial: only the 4 ternary-RHS smInstance refs converted to getInstance();
  the other 7 non-ternary refs stay direct because target wants them direct)

### `inline static void dummy(Vec* v) { *v = (Vec){...}; }` emits a `Vec` rodata constant *without* a `.text` symbol

**Rule:** A compound-literal `(Vec){a,b,c}` inside an `inline static`
function body causes MWCC to emit `a`, `b`, `c` as a 12-byte
rodata constant — *even though the inline function is never called
and its body is fully DCE'd from `.text`*. Use this as the canonical
"infectious" lever for materialising unreferenced `Vec`/`TVec3` rodata
constants that the target ships but our build is missing.

The form `static` (without `inline`) ALSO emits the rodata constant
but additionally leaves the dummy as a visible local function in
`.text`, which is the wrong shape for matching most TUs.

```cpp
// emits {0,0,0} into .rodata; no .text symbol
inline static void dummy(Vec* v)  { *v = (Vec){ 0.0f, 0.0f, 0.0f }; }
// emits {1,1,1} into .rodata; no .text symbol
inline static void dummy2(Vec* v) { *v = (Vec){ 1.0f, 1.0f, 1.0f }; }
```

**Where observed:**
- `MoveBG/MapObjManager.cpp:33-34` — original use of the trick;
  ships `{0,0,0}` (`@2760`) and `{1,1,1}` (`@2762`) in target's
  rodata at `.rodata:0xE0/0xEC`, unreferenced from `.text`. Note:
  the source there uses plain `static` (not `inline static`); our
  build does also emit a visible `dummy/dummy2` function as a
  result. Worth a future cleanup test (does adding `inline`
  break MapObjManager's match?).
- `Map/MapEventSirena.cpp` — adopted **`inline static`** form,
  added after `M3DUtil/InfectiousStrings.hpp` include. Emits
  `{0,0,0}`/`{1,1,1}` at `.rodata:0xE0/0xEC` matching target,
  with NO dummy/dummy2 symbols in `.text`. Promoted from Open
  questions (tick 70 closed the mystery).

**How it works (mechanism, partly confirmed):**
The compound literal `(Vec){a,b,c}` is treated by MWCC as a Vec
*aggregate constant* — it materialises the 12 bytes into rodata
at parse time. `*v = literal` then becomes `lwz/stw` copies from
the rodata block. When the function is `inline` and never called,
MWCC DCEs the body but keeps the rodata (since the rodata isn't
attributable to the function symbol; it sits in the TU's anonymous
rodata pool).

**Caveats:**
- Position of dummies in source does NOT cleanly determine rodata
  ordering. MWCC numbers rodata symbols by an internal scheme
  that depends on context (parsing event order, possibly per-TU
  state), not strictly by source line. Attempting to use this in
  `MapEventSink.cpp` (added dummies after `InfectiousStrings.hpp`,
  same as MapObjManager.cpp/MapEventSirena.cpp) placed the new
  zero/ones constants at `.rodata:0xC/0x18/0x24/0x30` —
  *before* the `MtxCalcType` strings — making the layout wrong
  for that TU. The same source-text-level recipe does NOT
  guarantee the same rodata ordering across different TUs.
- For TUs where the dummies land in the wrong rodata position,
  the lever is *not* useful as-is — you need a different way to
  push the constants past the MtxCalcType strings, or a separate
  IMPLEMENTATION strategy.

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

### Comparison operand order schedules static-init guard inline expansion

**Hypothesis:** When a `!=` (or `==`) comparison contains an inline call
that expands to a static-init guard block (e.g. `DEFINE_NERVE`'s
`theNerve()` accessor), MWCC schedules the guard at the source position
of the call. Swapping the operands of the comparison can move the
guard relative to the other operand's evaluation. This is independent
of the C++ "evaluation order is unspecified" rule — MWCC consistently
places the inline expansion at the source position.

**Observed:** `Enemy/seal::receiveMessage` had
`mSpine->getLatestNerve() != &TNerveSealDie::theNerve()`. Target's
asm shows theNerve's init guard BEFORE the getLatestNerve load. Our
build had the guard AFTER, with getLatestNerve's result cached in r30
across the guard (extra lwz/cmplwi/beq/b/lwz). Swapping operands
(`&theNerve() != mSpine->getLatestNerve()`) moved our guard before
getLatestNerve, lifting match 87.91% → 97.25% (+9.34pp).

**Counter-example:** `Enemy/Amenbo::forceKill` has the same pattern
(`mSpine->getLatestNerve() != &TNerveSmallEnemyDie::theNerve()`) and
matches at 100% with the ORIGINAL operand order. So the swap is
context-specific, not universal.

**Experiment to confirm/refute:** Sweep all `getLatestNerve() != &T::theNerve()`
callsites in non-matching functions. For each, check whether target's
asm shows the init-guard-before or init-guard-after pattern. If
before, try the swap. If the rule holds, this should be a reliable
+5-15pp lever in similar cases. If it doesn't (e.g. the swap regresses
some), need a finer rule for what triggers MWCC to put the guard
first.

**Cost when applied:** 1 instruction difference (the `cmplw` operand
order flips), but trades that for 5-15 instructions of scheduling
match. Usually worthwhile.

**Cited:** `Enemy/seal::receiveMessage` (this file, tick 92).
Counter-example: `Enemy/Amenbo::forceKill` (matched at 100% without).

### `new T(args)` expression may spill the result to a stack-0x10 scratch slot

**Hypothesis:** When a `T* p = new T(args)` expression is followed by
method calls that take `p` as `this`, MWCC sometimes emits a
spill-and-reload sequence using a stack slot at +0x10 even though `p`
already lives in a callee-saved register (e.g. r29):

```
mr.  r29, r3        ; r29 = new() return value
beq  SKIP
stw  r29, 0x10(r1)  ; spill to stack 0x10
... arg setup ...
lwz  r3,  0x10(r1)  ; reload to r3 — but r29 already has it!
bl   __ct__T...
lwz  r3,  0x10(r1)  ; reload again
... use ...
bl   method
SKIP:
```

Our build for the same source skips the spill and uses
`mr r3, r29` / `addi r3, r29, 0x0` directly. This costs us 5pp
match in `Enemy/DebuTelesa::load` (94.57% with no spill, can't
reach 100% without it). Adds +8 bytes to the stack frame (slot
0x10 reserved).

**Observed symptoms:**
- Target stack frame is +8 bytes vs ours
- Target has `stw rN, 0x10(r1)` immediately after the `beq` of the
  null-check
- Two subsequent `lwz r3, 0x10(r1)` reloads before the ctor and
  before the method call
- After the method call, target uses `r29` directly for field
  accesses (so r29 was always live; the spill is redundant)

**Why might MWCC do this:** Possibly exception-safety paranoia
(if ctor throws, the unconstructed pointer must remain reachable
for cleanup) — but SMS has exceptions off. Or possibly a specific
source-level pattern triggers it.

**Experiment to test:** Find another TU where this `new T(...)` spill
pattern appears in target and try source variants:
1. Cast the new result to base class: `TParams* p = new TFoo(...)`.
2. Use `void* mem = ::operator new(sizeof(T)); new(mem) T(...);
   T* p = static_cast<T*>(mem); ...`
3. Move the local declaration outside the function (impossible).
4. Add a redundant `volatile` qualifier to the local.
5. Place the call in a try-block (if exceptions can be locally
   enabled).

Citation: `Enemy/DebuTelesa::load` 94.57% (stuck — spill not
reproducible from any straightforward source pattern tested in
tick 82).

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

- **Vtables in target asm get per-vtable @ha/@l individual relocations;
  ours coalesces multiple vtables under one `...data.0@ha` base + offset.**
  Symptom: dtors at 86-89% diff at the vtable-chain unwind:
  target `lis r3, __vt__TDerived@ha; addi r3, r3, __vt__TDerived@l; stw r3, 0(this); addi r0, r3, 0x24; stw r0, 0x20(this); ... lis r3, __vt__TParent@ha; ...`
  ours: `lis r3, ...data.0@ha; addi r3, r3, ...data.0@l; addi r0, r3, 0x714; stw r0, 0(this); addi r0, r3, 0x738; ...; addi r0, r3, 0x10; ...`
  Both objects have `.obj __vt__TFence, global` etc. as separate symbol
  declarations. The difference is the relocation TYPE chosen by the
  compiler: target uses `R_PPC_ADDR16_HA` + `R_PPC_ADDR16_LO` per-vtable;
  ours uses section-relative `...data.0@ha`. Also target's vtables are
  ordered RailFence, WaterH, Water, Inner, Outer, TFence in `.data`;
  ours: TFence, Outer, Inner, Water, WaterH, RailFence. Affected: 4
  MapObjFence dtors at 86-89%, likely affects MANY other dtor-heavy
  TUs. Suspect cause: section attribute (each vtable should go to its
  own COMDAT-style group). Lever search: tried no source-level fixes
  yet. Worth investigating whether a per-class flag, a `__weak__`
  attribute, or a specific class structure forces separate sections.
- **`MsWrap<f>__Ffff` template instantiation inlined despite all defeat
  attempts.** Symptom: target asm `.fn "MsWrap<f>__Ffff", local` with
  4 callers via `bl`. Our build inlines the body at each call site,
  no out-of-line emission. Tried: (a) `#pragma dont_inline on/off`
  around caller (controlWall); (b) `#pragma dont_inline on/off` around
  explicit specialization `template <> f32 MsWrap<f32>(...)` at TU
  scope; (c) calling with explicit template arg `MsWrap<f32>(...)`.
  All ineffective — MWCC continues to inline. Hypothesis: the explicit
  specialization is dead-code-stripped because no external reference
  forces emission. Possible fix to try: `static MsWrap_fn dummy = &MsWrap<f32>;`
  taking the address forces emission. Affected: `MoveBG/MapObjFence::controlWall`
  (loses ~5pp from 4 inlined calls). Other TUs that use MsWrap inline
  it too (e.g. `Enemy/enemy.cpp::moveTo*`) and DO get the weak-emit
  pattern — so something about the *caller* context controls it.

- **`JGeometry::gekko_ps_copy12` inlined in our build but called via `bl` in
  target.** Symptom: target asm has `bl gekko_ps_copy12__9JGeometryFPvPv`,
  ours emits 12 `psq_l`/`psq_st` instructions at the call site. Confirmed
  on `MoveBG/MapObjFlag::draw` and `MoveBG/MapObjBall::calcCurrentMtx`;
  asm files for MapObjBall/Block/Cloud/Fence/Flag/Monte/Pinna/RailBlock/Tree
  all contain `bl` calls; MapObjBlock emits the weak symbol. The function
  is `inline` in `include/JSystem/JGeometry/JGMatrix34.hpp` and uses an
  `asm {}` block with `register f32` locals. Our build inlines despite
  `-inline deferred` flag for the `MoveBG` lib. Tried (all no effect):
  (a) `#pragma inline_max_size(0)` + `inline_max_total_size(0)` before
  draw definition; (b) wrapping arg as `j3dSys.getViewMtx()` to add an
  inline forwarder; (c) explicit `(void*)` casts at call site. Hypotheses:
  (1) MWCC under `-inline deferred` decides per-TU based on something
  about parse order that we're not replicating; (2) a specific local-decl
  ordering before the call inhibits inlining; (3) the original source uses
  a non-inline wrapper we haven't found. Affected: `MoveBG/MapObjFlag::draw`
  at 71.82%, `MoveBG/MapObjBall::calcCurrentMtx` at 62.32%. Risk: if the
  lever is found, it could lift 8+ TUs simultaneously.
- **MAnmSoundNPC::startAnimSound: `lwzx` indexed-load vs target's
  `add+lwz`.** Target loads v as `add r3, r31, r5; lwz r3, 0x18(r3)`
  (mData held in NV r31, regular lwz at constant offset 0x18). Ours
  emits `slwi r3, r0, 5; addi r0, r3, 0x18; lwzx r5, r4, r0` (mData
  in temp r4, indexed lwzx). Tried: (a) `u8* base = (u8*)mData +
  mDataCounter*0x20; u32 v = *(u32*)(base+0x18)` — MWCC still picks
  lwzx; (b) `u8 (*entries)[0x20] = ...; *(u32*)(entries[idx]+0x18)`
  — also still lwzx. Both forms get re-merged. The lwzx pattern
  prevents mData from being NV-promoted to r31 (target saves r26..r31,
  6 regs; ours saves r27..r31, 5 regs). 70.3% → 72.9% after b2/b3
  mask-test rewrite (the remaining 27% gap is mostly this + cascading
  reg-coloring downstream).
- **MAnmSound::startAnimSound switch fall-through body order.** Target
  asm orders switch case bodies as: `case 0; case 7; default`. Target's
  case 0 body ends with `bne default_label; b end_label` — it's an
  explicit goto-to-default, not a fall-through. With C++98 + no-goto
  constraint, this can't be reproduced exactly. Tried in tick 102:
  duplicating the default body inline in case 0 (instead of fall-through
  to default) — this produces order `case 0 + duplicate-body; case 7;
  default` (78.9% → 85.5%). MWCC does NOT tail-merge the duplicate bl,
  so case 0 emits its own `addi r3, r28; addi r4, r29; ... bl
  startSoundActorInner` instead of jumping to default. Last ~14% gap
  is this duplication + cascading frame-size delta. Currently-hard.
- **MWCC per-call-site inline decision is *not* TU-wide.** Symptom: in
  `Enemy/BossHanachanSub::moveHead`, target inlines `TVec3::add` and
  `TVec3::scale` in loop1 (`mPos.y += g; mPos.add(mVel)`) and the first
  half of loop2's `dir.scale()`, but emits `bl sub`/`bl add`/`bl scale`
  for the same TVec3 helpers in loop2's `diff = mPos - mPrev`, loop2's
  `newPos.add(dir)`, and loop3's `(mPos-mPrev)*m08`. Our build inlines
  ALL of them, losing the bl-with-stack-temp pattern target uses. The
  same TU's `setDegreeZAndRevisionPosXZ` also has `bl sub` for its one
  `delta.sub(p.mPos)`. No `#pragma dont_inline` in source. Other TUs
  (cameralib, bossgesso, amiNoko) call out-of-line `sub` without any
  pragma either. Hypotheses worth testing: (a) MWCC's inline-cost
  budget is *per function* and gets exhausted at later call sites once
  enough size has been emitted (loop1 came first, was cheap → inlined;
  loop2/3 added bl's because the budget was used up); (b) register
  pressure at the call site shifts the budget calculation; (c) the
  decision is influenced by what's on the live-range list at that
  point. A clean experiment: take a single function with two identical
  `vec.add(other)` calls; insert dummy bloat between them; check
  whether the second flips from inline → bl. Affected TUs:
  `Enemy/BossHanachanSub::moveHead` at 61.74%, `setDegreeZ` at 79.59%
  (tick 100).
- **Dead-code preservation in `Hino2Landing::execute` (`fctiwz; stfd;
  lwz` without consumer).** Symptom: target asm contains the canonical
  `lfs; fctiwz; stfd; lwz` f32→s32 conversion sequence with the loaded
  int going into `r0` which is immediately overwritten by the next
  unrelated `lwz` — i.e. the conversion result is dead but emitted.
  Same with an inline `checkLiveFlag(LIVE_FLAG_HIDDEN)` expansion
  (`lwz; rlwinm.`) whose flag result is unused. Our source has bare
  `self->getMActor()->getFrameCtrl(0)->getFrame();` and
  `self->checkLiveFlag(LIVE_FLAG_HIDDEN);` (called but result
  discarded); our build DCE's them. Adding `(s32)` cast alone is
  insufficient. Adding `volatile s32 frame = (s32)...; (void)frame;`
  emits both ops AND match size goes from 168 → 192 (matching target),
  but volatile forces an extra `stw r0, 0x2c(r1)` write-back that
  target doesn't have. What source pattern produces the in-register
  conversion *without* a write-back? Likely candidates: register-only
  storage class (not supported in MWCC C++), an inline empty function
  taking the value by value, or a missing `JUT_ASSERT_F` whose body got
  optimized but the eval kept. Affected TU: `Enemy/hinokuri2`
  `execute__TNerveHino2LandingC...` at 87.40% (tick 100).
- **Multi-stream-register-copy in stream-read loops.** Symptom: target's
  `load__17TMarioPositionObjFR20JSUMemoryInputStream` copies the stream
  parameter `r4` into SIX callee-saved registers (r24..r29 all = r4)
  before entering its read loop, then dispatches the 10 per-iteration
  `stream.read(...)` calls across all six registers in a non-obvious
  pattern (24,24,26,29,24,25,28,24,24,27). Our build with plain
  `stream.read(...)` calls keeps `stream` in a single register
  (matching JDRActor::load, TCubeStreamInfo::load which are non-looped
  and match cleanly). Hypotheses to test: (a) the source had an
  inlined `TVec3<f32>::read(JSUInputStream&)` helper that rebound
  `stream` to a fresh reference at each expansion, forcing the
  allocator to spill into fresh callee-saved regs; (b) chained
  `operator>>(JSUInputStream&, JGeometry::TVec3<f32>&)` returning
  fresh refs at each link; (c) some macro that captured stream
  into a local. Need a minimal test case. Affected TUs:
  `Player/MarioPositionObj` load at 87.48% (filed tick 99) — see
  `state/notes/MarioPositionObj.md`.
- **Dead-but-kept TMatrix34 stack stores under MWCC -O4,p.** Symptom: target
  `watch__26TDolpicEventRiccoMammaGateFv` (and equivalent code shapes in other
  TUs) emits 12 stfs stores at `r1+0x60..+0x8C` initialising a `TMatrix34`
  identity + `mMtx[1][1] = scale`, but no subsequent read or pass-by-reference
  occurs — the next instruction is `lwz r3, 0x24(r31); ... vtbl[6] = setUp()`,
  which takes no args. The matrix is purely dead from source view. Yet MWCC
  emits the stores. Our build of the same source pattern DCE's the stores
  entirely. What forces MWCC to keep them? Theories worth testing: (1) the
  matrix variable was the `this` of an inlined helper whose body got DCE'd
  but the prologue/epilogue ctor stores survived (e.g. `auto coll = ...; (void)
  coll;` with `coll`'s ctor non-trivial); (2) `volatile` somewhere in the
  matrix's source path; (3) the matrix was passed-by-reference to an inline
  empty body and the compiler kept the prep stores. Affected TU:
  `mario/Map/MapEventDolpic` watch__Ricco at 77.55% (filed tick 71).
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
