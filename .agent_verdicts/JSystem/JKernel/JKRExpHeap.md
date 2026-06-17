# JSystem/JKernel/JKRExpHeap.cpp

Verdict: equivalent
Time: 2026-06-13 7:10am MNL

Build proof:
- `python configure.py --non-matching && ninja`
- `python configure.py && ninja`

No missing symbols.

Reviewed nonmatching functions:
- `JKRExpHeap::destroy()`
  - Equivalent. Non-root heaps destroy themselves through the virtual dtor and free their backing allocation to the parent heap; root heaps only destroy themselves.
  - Residue is stack-frame size/slot layout.
- `JKRExpHeap::allocFromHead(unsigned long, int)`
  - Equivalent. The target raw asm matches the source's best-fit scan, alignment offset calculation, split-before-aligned-content path, small-offset path, zero-offset path, debug `whatdo2`/DB stores, free-list relinking, used-list append, and null failure.
  - Residue is register allocation plus misleading helper labels in objdiff caused by source/target symbol-owner drift; raw asm confirms calls to `allocFore`, `setFreeBlock`, `removeFreeBlock`, and `appendUsedList` where expected.
- `JKRExpHeap::recycleFreeBlock(JKRExpHeap::CMemBlock*)`
  - Equivalent. Raw asm matches alignment back-merge, empty-list insertion, head insertion, tail insertion, interior insertion, and adjacent-block coalescing through `joinTwoBlocks`.
  - Residue is the known `clrlwi`/`clrlwi.` expression shape and inline/free-list helper ownership drift.
- `JKRExpHeap::joinTwoBlocks(JKRExpHeap::CMemBlock*)`
  - Equivalent. It computes the current block end and adjusted next-block address, reports/panics when overlapping, and coalesces exactly adjacent blocks by extending size and relinking through `setFreeBlock`.
  - Residue is stack-frame/register allocation.
- `JKRExpHeap::dump_sort()`
  - Equivalent. It locks, checks the heap, reports used blocks in address order, reports free blocks, computes used percentage, unlocks, and returns the check result.
  - Residue is stack slot/register allocation and argument setup scheduling for `JUTReportConsole_f`.

Extra symbols (`freeGroup`, `isEmpty`, `removeUsedBlock`, `genData`) are source-emitted unused/helper functions and debug string carrier code; they do not indicate missing target behavior for the linked source object.

2026-06-13 7:10am MNL recheck:
- Full current diffs for `destroy`, `allocFromHead(unsigned long, int)`,
  `recycleFreeBlock`, `joinTwoBlocks`, and `dump_sort` still match the
  behavior described above.
- Remaining residue is frame/save-slot size, GPR coloring, local label
  attribution, and helper-owner label noise. The allocation and free-list paths
  still perform the same best-fit scan, split/remove/set/append calls, debug
  counter stores, block coalescing, dump/report loops, and panic/report paths.
- `python configure.py --non-matching && ninja` linked from source, then
  `python configure.py && ninja` restored the matching build and passed with
  `build/GMSJ01/mario.dol: OK`.

2026-06-13 11:07am MNL recheck:
- Verdict remains `equivalent`.
- Current overview still has no missing target symbols. Re-read the five
  nonmatching functions: `destroy`, `allocFromHead(unsigned long, int)`,
  `recycleFreeBlock`, `joinTwoBlocks`, and `dump_sort`.
- `destroy` still has identical root/non-root destruction behavior; residue is
  frame size and save-slot placement.
- `allocFromHead` still performs the same best-fit scan, split-before-content
  paths, small-offset/zero-offset paths, debug counter stores, free-list
  relinking, used-block append, and null failure. The displayed
  `recycleFreeBlock`/`removeUsedBlock` call-label drift occurs on matching
  branch instructions and source-owned helper labels, not a different call
  sequence.
- `recycleFreeBlock` and `joinTwoBlocks` still coalesce and relink the same
  blocks; residue is one `clrlwi.` expression shape, register coloring, and
  frame slots.
- `dump_sort` still locks, checks, reports used blocks, reports free blocks,
  computes used percentage, unlocks, and returns the check result. Residue is
  report-argument scheduling and stack/register layout.
