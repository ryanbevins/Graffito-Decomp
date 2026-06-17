# JSystem/JKernel/JKRAram Audit

Verdict: equivalent
Time: 2026-06-13 7:10am MNL

## Verdict
equivalent — 2026-06-12 7:24pm MNL

## Reason
`JSystem/JKernel/JKRAram.cpp` links from source under
`python configure.py --non-matching && ninja`, and the normal matching build
still verifies `mario.dol: OK`.

Reviewed all four nonmatching text functions:
- `JKRAram::mainRamToAram(u8*, u32, u32, JKRExpandSwitch, u32, JKRHeap*, int)`
- `JKRAram::aramToMainRam(u32, u8*, u32, JKRExpandSwitch, u32, JKRHeap*, int, u32*)`
- `JKRAram::aramToMainRam(JKRAramBlock*, u8*, u32, u32, JKRExpandSwitch, u32, JKRHeap*, int, u32*)`
- `firstSrcData()`

The ARAM transfer/decompression behavior matches: alignment panics, compression
classification, expand-size clamping, heap allocation/free paths, group-id
updates, DMA/decompression calls, returned size writes, block-offset bounds,
and source-buffer cursor updates all use the same calls, constants, offsets,
stores, and branch conditions. Remaining residue is stack frame/slot placement,
callee-saved register coloring, local static label attribution, and `mr` versus
`addi r,r,0` encoding in `firstSrcData()`. No missing target symbols were
reported; extra rows are source-owned helpers/statics that link cleanly.

2026-06-13 6:29am MNL recheck: overview still has no missing target rows, and
`python configure.py --non-matching && ninja` linked from source.

2026-06-13 7:10am MNL recheck: full current diffs still show codegen-only
residue. The two `aramToMainRam` overloads preserve the same alignment checks,
allocation/free paths, compression cases, size-output writes, and recursive
block-offset call. `firstSrcData()` reads and writes the same transfer globals;
the apparent symbol-name changes are relocation-label drift, with only
`mr`/`addi` and epilogue scheduling residue. Source-link proof and plain
matching proof both passed.

2026-06-13 11:41am MNL recheck: verdict remains `equivalent`. Full current
diffs for `mainRamToAram`, both `aramToMainRam` overloads, and `firstSrcData`
again show the same alignment panics, compressed-size detection, heap
allocation/free, group-id byte stores, DMA/decompression calls, output-size
writes, block-offset bounds, and transfer cursor updates. Remaining differences
are stack size/slot placement, GPR coloring, local label attribution, and
`mr`/`addi` encoding. Shared proof passed with `python configure.py
--non-matching && ninja`, then normal `python configure.py && ninja` verified
`build/GMSJ01/mario.dol: OK`.

2026-06-13 1:21pm MNL recheck: verdict remains `equivalent`. Re-read current
full diffs for all four nonmatching functions. The ARAM copy/decompression
paths still preserve the same panics, size clamps, heap allocation/free paths,
DMA/decompress calls, returned-size writes, recursive block-offset logic, and
source cursor updates. The confusing `firstSrcData()` global labels are still
local-label attribution drift; raw asm/source agree on `srcAddress`,
`fileOffset`, `readCount`, and `srcLimit` behavior. Proof rerun passed with
`python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` verified `build/GMSJ01/mario.dol: OK`.
