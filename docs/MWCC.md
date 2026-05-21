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
