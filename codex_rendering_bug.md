# Task: find why 3D rendering is broken in this SMS decomp build

You are investigating a Super Mario Sunshine decompilation (GMSJ01 JPN) at this repo. A full-source build
runs in Dolphin but **nothing 3D renders** — on the title screen AND in-game the entire 3D world is gray;
only the 2D UI (logo, text, fader) draws. Your job: find the exact source-level cause and propose a fix.
You CANNOT run the game. Use static analysis (objdump, decomp-diff, reading source) + airtight reasoning.

## PROVEN by TU bisection (ground truth — trust this)
There is a harness: `bisect_retail.txt` (one TU path per line) makes `configure.py` link the retail object
(`build/GMSJ01/obj/<path>.o`) instead of our compiled source for those TUs. Rebuild with
`python configure.py --non-matching && ninja build/GMSJ01/mario.dol`.

Forcing ONLY `System/MarNameRefGen.cpp` to retail makes ALL 3D rendering work. Our compiled source version of
this one TU breaks all 3D rendering. So the defect is in the difference between our `build/GMSJ01/src/System/MarNameRefGen.o`
and retail `build/GMSJ01/obj/System/MarNameRefGen.o`. (Forcing the whole 3D framework — JDrama/J3D/M3DUtil/MarioUtil —
to retail did NOT fix it; only MarNameRefGen.o does.)

## What is ALREADY established (don't redo; build on it)
- The scene loads completely. `TMarNameRefGen::getNameRef(const char*)` is the object factory; I logged it: it is
  called ~362 times for a full level (Map, Sky, Mario, MapObjManager, DrawBufObj, GroupObj, PerformList, ...), and it
  returns NULL for NONE of them. So 3D object CREATION fully succeeds. The bug is in per-frame DRAW/wiring, not creation.
- Every non-matching function in MarNameRefGen.o is, on inspection, behaviorally EQUIVALENT to retail — the diffs are
  register allocation, +0x10 stack-frame inflation, and instruction scheduling only:
    - getNameRef 92.3% — the only real diff is 3 commented-out stage-specific branches (MapEventSirenaSink,
      MareEventBumpyWall, MareEventWallRock), irrelevant to the title/first level.
    - TViewObjPtrListT<THitActor>::perform 97.3% / searchF 94% / loadAfter 93% — frame-size cosmetic; iterate+testPerform / iterate+searchF logic matches.
    - TVector<...>::InsertRaw 58.7% (TCameraMapTool) — pure register-allocation permutation; capacity/copy math identical.
    - TNameRefAryT<...>::load 99% — scene loads fine.
  vtables are IDENTICAL slot-for-slot (the .data reloc *record order* differs but offset->function is identical).
  `__sinit_MarNameRefGen_cpp` is 100% match. .data/.rodata effectively identical (ours emits 3 extra weak base
  vtables TViewObj/TCharacter/JSUIosBase that fold at link — a dtk-split artifact, believed harmless).
- ours .text = 0x6ad8 vs retail 0x5d84 (ours ~3412 bytes bigger, from the +0x10 frames / extra inlining).
- A previous fix removed a debug guard (`BADARY`/`BADPTRARY` magic-pointer-range check that did `OSReport(); break;`)
  from `include/Strategic/NameRefAry.hpp` and `include/Strategic/NameRefPtrAry.hpp` searchF templates. That made those
  searchF instantiations 0%->100% match, but did NOT fix rendering. (Note other copies of that guard still exist in
  src/Enemy/conductor.cpp and src/Strategic/objmanager.cpp, but those TUs are NOT the bisected blocker.)

## The paradox to crack
Swapping the whole MarNameRefGen.o to retail fixes rendering, yet every function/vtable/data/__sinit in it appears
behaviorally identical to retail. Likely explanations to investigate:
  (a) MarNameRefGen.o is the LINK-OWNER (winning weak copy) of some shared template/inline instantiation (defined in a
      header) that is behaviorally WRONG in our headers, so it infects the whole game's per-frame draw; swapping the TU
      replaces that winning copy with retail's correct one. Find which weak symbol, defined in which header, is wrong.
  (b) A "cosmetic" diff is actually behavioral (e.g. the +0x10 frame inflation, or the JSUMemoryInputStream-vs-TList-iterator
      call seen in the perform/searchF diff, indicates a real structural difference in iteration). Prove or refute.
  (c) Something about the per-frame perform/testPerform/draw dispatch or the object draw-pass FLAGS set during load.
This earlier had an exact analog: a different TU's runtime-built dispatch table (sCmdPList in JASSeqParser) was wrong
even though its .data looked identical — the table was built at runtime. Consider analogous runtime/representation effects.

## Tools
- `build/binutils/powerpc-eabi-objdump.exe` -d/-t/-r/-h  (symbols mangled; our .o vs build/GMSJ01/obj/.../MarNameRefGen.o)
- `python tools/decomp-diff.py -u mario/System/MarNameRefGen [-d MANGLED_SYM] [--no-collapse] [-C n] [-s nonmatching]`
- Source: src/System/MarNameRefGen.cpp ; headers under include/JSystem/JDrama/ (JDRViewObjPtrList.hpp, JDRNameRefPtrList.hpp,
  JDRNameRef.hpp, JDRDStage.hpp, JDRSmJ3DScn.hpp), include/Strategic/NameRefAry.hpp, NameRefPtrAry.hpp.
- decomp.me-style: this is a functional-equivalence project; low match % does NOT mean broken — find a real BEHAVIORAL
  divergence. The compiler is Metrowerks CodeWarrior 1.2.5 for GameCube PPC.

## Deliverable
Identify the single behavioral defect (which symbol, which header/source, what is wrong vs retail, why it breaks the 3D
draw while leaving 2D and object creation intact) and give a concrete source fix. Be concrete: symbol names, file:line,
old->new. If you find the weak-symbol link-owner mechanism, name the exact instantiation and the header bug.
