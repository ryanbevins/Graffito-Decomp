# Task: find why 3D rendering is broken in this Super Mario Sunshine decomp build

You are picking up a hard, well-scoped bug in a Super Mario Sunshine decompilation (GMSJ01, JPN rev 0) at
`C:\Users\ryana\Documents\sms`. The compiler is Metrowerks CodeWarrior 1.2.5 (PowerPC/Gekko). This is a
FUNCTIONAL-EQUIVALENCE project: a low per-function "match %" does NOT mean a function is wrong — only a real
BEHAVIORAL divergence matters. You CANNOT run the game; the human operator runs it in Dolphin and reports back.
Work by static analysis + reasoning, and by proposing builds/experiments the operator can run.

## Symptom
A full-source build boots in Dolphin but **renders no 3D at all** — on the title screen the background is flat
gray (only the 2D logo/PRESS-START/copyright draw), and in-game the entire 3D world is gray (only 2D UI draws).
After pressing Start there is an invalid write to 0x00000004 and Mario "dies" instantly, all consistent with a
broken-but-still-running 3D scene. 2D (GC2D/J2D) is fine.

## GROUND TRUTH — TU bisection (trust this completely)
There is a bisection harness (see Tooling). Forcing exactly ONE translation unit, `System/MarNameRefGen.cpp`, to
link from the original retail object instead of our compiled source makes ALL 3D rendering work correctly. Our
compiled-source version of that one TU breaks all 3D rendering. So the defect is in the difference between our
`build/GMSJ01/src/System/MarNameRefGen.o` and the retail `build/GMSJ01/obj/System/MarNameRefGen.o`, OR in a
weak/COMDAT symbol that this TU link-owns game-wide. Forcing the entire 3D framework (JDrama/J3D/M3DUtil/MarioUtil
draw TUs) to retail did NOT fix it — only MarNameRefGen.o does. `TMarNameRefGen` is the master object factory:
`getNameRef(const char* name)` maps a scene object's name string to a `new`-ed C++ object; the scene loader calls
it for every named object in a stage.

## What has ALREADY been established / ruled out (do NOT redo; build on it)
1. **Object creation is fine.** Logging `getNameRef` shows it is called ~362 times for a full level (Map, Sky,
   Mario, MapObjManager, DrawBufObj, GroupObj, IdxGroup, PerformList, ...) and returns NULL for NONE of them. The
   whole scene graph is created and loads to completion (load ends normally with the 7 PerformList objects). The
   break is in PER-FRAME draw/wiring, not creation.
2. **Every function in MarNameRefGen.o is behaviorally identical to retail** as far as could be determined:
   - `getNameRef` 92.3% — the only real source difference was 3 commented-out factory branches
     (MapEventSirenaSink ~L246, MareEventBumpyWall ~L261, MareEventWallRock ~L264). THOSE HAVE NOW BEEN
     UNCOMMENTED (see Current state) — `getNameRef` is now 94.5% and the build links `MapEventSirena.o` +
     `MapEventMare.o` (previously dead-stripped). **This did NOT fix rendering.**
   - `TViewObjPtrListT<THitActor>::perform` (97.3%) / `searchF` (94%) / `loadAfter` (93%) — raw disassembly shows
     IDENTICAL bl-targets (iterator ctor + testPerform/searchF), only frame-size (retail -0x88 vs ours -0x78) and
     `addi r3,r3,8; lwz r3,0(r3)` vs `lwz r3,8(r3)` (equivalent). These are link-owned by MarNameRefGen.o and used
     by scene containers like `TIdxGroupObj : public TViewObjPtrListT<THitActor>`.
   - `TVector<...>::InsertRaw` (58.7% for TCameraMapTool) — pure register-allocation permutation; identical math.
   - vtables IDENTICAL slot-for-slot ours vs retail (including `__vt__TViewObj` base, slots
     __dt/getType/load/save/loadAfter/searchF matching canonical `JDRActor.o`). The .data reloc *record order*
     differs but offset->function is identical.
   - `__sinit_MarNameRefGen_cpp` is 100% match.
3. **A debug guard was found and removed, then REVERTED.** `include/Strategic/NameRefAry.hpp` and
   `include/Strategic/NameRefPtrAry.hpp` `searchF()` templates contained a hand-injected `BADARY`/`BADPTRARY`
   pointer-range/alignment guard (`if (ptr<0x80003100 || ptr>=0x81800000 || (ptr&3)) { OSReport(...); break; }`,
   added in commit `0c219984` "Runtime bring-up"). Removing it made those searchF instantiations 0%->100% match,
   but did NOT fix rendering, so it was reverted (the guards are currently present in the headers again). NOTE
   identical guards still exist in `src/Enemy/conductor.cpp` and `src/Strategic/objmanager.cpp` (BADCOND/BADOBJM).
4. **Linked-binary diff (THE strongest tool used).** Built two ELFs — `orig_source.elf` (gray) and
   `orig_retail.elf` (renders, = MarNameRefGen forced to retail) — disassembled both and compared functions with
   addresses/registers normalized. Result: of 12718 common functions, the ONLY substantive (non-cosmetic) `.text`
   differences are the 3 `TViewObjPtrListT<THitActor>` functions above, plus the set of Sirena/Mare functions that
   exist only in the retail build because retail's getNameRef referenced them (now fixed by uncommenting — didn't
   help). Everything else differed only by frame-size/regalloc.
   **CRITICAL LIMITATION of that diff: it compared `.text` instruction MNEMONICS only. It did NOT compare `.data`,
   `.rodata`, `.sdata`, `.sbss`, `.sdata2`, or operand-level differences (different constants/offsets with the same
   mnemonic).** So the real divergence is very likely in DATA, or an operand-level code difference, or a
   weak-symbol link-ownership effect that a mnemonic diff cannot see.

## The paradox / where to look next (your job)
After accounting for the Sirena/Mare link (now matched) the two builds' `.text` is cosmetically equal, yet one
renders and one is gray. Strong candidate explanations not yet disproven:
- A **DATA difference**: a vtable, a global pointer, a static table, an sdata/sdata2 constant, or a runtime-built
  table that differs. (Precedent in this project: a different TU, JASSeqParser, had a render-equivalent of this —
  its `sCmdPList` dispatch table was zero in `.data` and built at runtime by `__sinit` from a SOURCE ARRAY whose
  nullptr entries were at wrong indices; `.data` looked identical but the runtime table was wrong. Look for an
  analogous runtime-built or representation-level data bug here.)
- An **operand-level code difference** the mnemonic diff hid (same mnemonics, different constant/offset/register
  that matters) — re-diff the suspicious functions with full operands, normalized only for absolute addresses.
- A **weak/COMDAT symbol** that MarNameRefGen.o link-owns whose winning copy changes a per-frame draw path
  game-wide (e.g. a draw-pass flag, a `testPerform`/draw predicate, a perform-list dispatch).
- The **per-frame draw / perform-list** path: the scene draws via `TPerformList` objects (System/PerformList.cpp,
  loaded from `/data/PerformLists.bin`) which reference draw helpers by name (`SMS Draw Init`,
  `J3D System Set View Mtx`, `AlphaCatch`, `ZBufferCatch`, ...). Confirm those draw passes actually run and find
  every object whose draw-pass FLAGS or name-resolution could differ.
Recommend instrumenting the per-frame draw with OSReport (the operator can capture it) to localize where actors
stop being drawn — e.g. log child counts in `TViewObjPtrListT::perform`, the pass mask reaching `testPerform`,
and whether the draw-helper passes (ZBufferCatch/AlphaCatch/SMSDrawInit/J3DSysSetViewMtx) execute.

## TOOLING (all on Windows; use git-bash-style paths or PowerShell)
### Build
- `python configure.py --non-matching`  → regenerates `build.ninja`. The `--non-matching` flag makes "Equivalent"
  TUs build from our source (without it they link retail objects — see linking below).
- `ninja build/GMSJ01/mario.dol`  → links the final DOL. `ninja build/GMSJ01/mario.elf` for the ELF.
- `ninja build/GMSJ01/src/<path>.o`  → compile a single object. (Do NOT rely on `python -m ninja`; it can hang on
  this box. Plain `ninja` is fine. A full header change rebuilds ~230 TUs but still completes in seconds.)
- `build/GMSJ01/report.json` has per-function match %.
- Deploy to test: copy `build/GMSJ01/mario.dol` to `C:\Users\ryana\Downloads\SunshineJPExtract\sys\main.dol`,
  operator runs it in Dolphin. OSReport prints are captured via EXI to
  `C:\Users\ryana\AppData\Roaming\Dolphin Emulator\Logs\dolphin.log` (Logger has OSREPORT + WriteToFile on).

### Linking Matching / Equivalent / NonMatching TUs (the bisection lever)
In `configure.py`, every TU is declared `Object(<status>, "<path>.cpp")` inside `config.libs`. The status maps to
the internal `obj.completed` boolean (tools/project.py): `link_built_obj = obj.completed`.
- `Matching` (= True): always compiled from our source AND linked. (Asserted byte-identical to retail.)
- `NonMatching` (= False): NOT built from source; the link uses the original retail split object
  `build/GMSJ01/obj/<path>.o` instead.
- `Equivalent` (= `config.non_matching`): compiled-from-source when configured `--non-matching`, otherwise links
  the retail object. So a full `--non-matching` build is "all source"; a no-flag build is "mostly retail".
- Retail/original objects (dtk splits of the real game DOL) live under `build/GMSJ01/obj/...`. Our compiled
  objects live under `build/GMSJ01/src/...`.
- **Bisection harness (already added to configure.py):** create a file `bisect_retail.txt` with one TU path per
  line (e.g. `System/MarNameRefGen.cpp`); configure.py forces each listed TU's `completed=False`, so it links the
  retail object while everything else stays source. Then `python configure.py --non-matching && ninja
  build/GMSJ01/mario.dol`. Delete the file (or empty it) to go back to full-source. This is exactly how the bug
  was localized to MarNameRefGen.cpp; you can keep bisecting (e.g. to split a TU you can build two ELFs and diff).

### Disassembly / symbols
- `build/binutils/powerpc-eabi-objdump.exe`  flags: `-d` (disasm), `-dr` (disasm+relocs), `-t` (symbol table),
  `-r --section=.data` (relocs of a section), `-h` (section sizes), `-s -j .rodata` (raw section bytes),
  `--no-show-raw-insn`, `--start-address`/`--stop-address`. Symbols are MANGLED (CodeWarrior mangling; no
  demangler available — match by mangled name).
- Compare our vs retail object directly: `build/GMSJ01/src/System/MarNameRefGen.o` vs
  `build/GMSJ01/obj/System/MarNameRefGen.o`.

### decomp-diff (per-function diff vs retail)
- `python tools/decomp-diff.py -u mario/System/MarNameRefGen [-d <MANGLED_SYM>] [--no-collapse] [-C <ctx>]
  [-s nonmatching] [-t function|object] [--range HEX-HEX]`. In its output LEFT = retail target, RIGHT = our build.
  Beware: its instruction aligner can mis-pair symbols across an inserted/removed instruction (it once showed a
  bogus `JSUMemoryInputStream::~` call that raw `objdump -dr` proved was actually the TList iterator ctor) — verify
  surprising call-target diffs against raw `objdump -dr`.

### Two-ELF linked diff (build it yourself; this is what cracked it down to the Sirena/Mare + 3 funcs)
1. `rm -f bisect_retail.txt; python configure.py --non-matching; ninja build/GMSJ01/mario.elf` → copy to A.
2. `echo System/MarNameRefGen.cpp > bisect_retail.txt; python configure.py --non-matching; ninja
   build/GMSJ01/mario.elf` → copy to B.
3. `objdump -d --no-show-raw-insn` both, parse into functions keyed by `<symbol>`, compare. For the REAL signal,
   ALSO diff `.data/.rodata/.sdata/.sdata2/.sbss` (objdump -s and -r) symbol-by-symbol, normalized for absolute
   addresses — this was NOT done yet and is the most promising untried step.

## Current repo state (so you know what's already changed)
- `src/System/MarNameRefGen.cpp`: the 3 factory branches are now UNCOMMENTED (`MapEventSirenaSink`,
  `MareEventBumpyWall("MareEventBumpyWall")`, `MareEventWallRock("MareEventWallRock")`), and
  `#include <Map/MapEventMare.hpp>` was added. getNameRef is now 94.5%. (This is a correct change but did NOT fix
  rendering.)
- `include/Strategic/NameRefAry.hpp`, `NameRefPtrAry.hpp`: ORIGINAL (the BADARY/BADPTRARY guards are present).
- `configure.py`: contains the bisect_retail.txt override block (the harness). Keep it.
- No `bisect_retail.txt` currently (full-source build).
- An unrelated bug already FIXED earlier this session: `JASSeqParser.cpp` `sCmdPList` (audio). Untouched here.

## Deliverable
Find the exact behavioral difference between our MarNameRefGen.o (or a weak symbol it link-owns, or a data/table
it contributes) and retail that breaks the per-frame 3D draw while leaving 2D and object creation intact. Give a
concrete source fix (file:line, what's wrong, old->new), and an experiment the operator can run (a build + Dolphin
check, or an OSReport instrumentation) to confirm it. The two-ELF DATA diff and per-frame draw instrumentation are
the two most promising untried directions.
