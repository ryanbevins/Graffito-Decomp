Verdict: equivalent

Date: 2026-06-13 9:44am MNL
Unit: `mario/JSystem/JKernel/JKRArchivePub`
Source: `src/JSystem/JKernel/JKRArchivePub.cpp`
Classification: `Object(Equivalent, "JSystem/JKernel/JKRArchivePub.cpp")`

Reason:
- Re-verified during the audit sweep together with `JKRArchivePri`.
  `python tools/decomp-diff.py -u mario/JSystem/JKernel/JKRArchivePub` shows
  every function body byte-matching the target.
- The only reported target-side gap is data ownership:
  target `JKRArchivePub` owns `JKRArchive::__vtable`, while the source build
  emits the matching vtable from `JKRArchivePri`. This is paired ownership
  drift, not missing runtime behavior, and the source-link proof succeeds with
  one source-owned definition.
- 9:44am MNL recheck: the unit overview still has every function
  byte-identical. The target-owned `JKRArchive::__vtable` row remains paired
  ownership drift with `JKRArchivePri`, not an undefined source-link symbol.
- 4:47pm MNL stale-Equivalent recheck: current overview is unchanged. All text
  functions are still byte-identical; `python tools/decomp-diff.py -u
  mario/JSystem/JKernel/JKRArchivePub -s missing` reports only the paired
  `JKRArchive::__vtable` / `.data-0` ownership rows.

Proof:
- `python configure.py --non-matching && ninja` linked
  `build/GMSJ01/mario.dol` from source.
- `python configure.py && ninja` restored the matching config and passed
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

2026-06-14 5:58pm MNL safety-net recheck: verdict remains `equivalent`.
Current overview still has every text function byte-identical. The only
reported missing rows are the paired `JKRArchive::__vtable` / `.data-0`
ownership drift against `JKRArchivePri`; the current source-link proof from
this tick covers the object.
