# mario/JSystem/JKernel/JKRSolidHeap

Verdict: equivalent
Date: 2026-06-13 7:10am MNL

Reason: all functions except `JKRSolidHeap::state_register` match exactly.
`state_register` writes the same state id, calls the same virtual
`getTotalFreeSize`, stores the same used size, and computes the same check code
from `mCurEnd * 3 + mCurStart`; residue is stack-frame size/slot offsets and a
temporary result register. Vtable/data residue is weak inherited stub ownership
for `dump_sort`, `changeGroupID`, and `getCurrentGroupId`, whose source-linked
bodies are equivalent no-op/accessor stubs.

Recheck at 7:10am: the same verdict holds. Object diffs are still limited to
`state_register` frame/temp-register drift plus inherited weak-stub vtable/data
ownership. Source-link proof passed in the 7:10am `--non-matching` build, and
the plain matching build passed with `mario.dol: OK`.

Proof:
- `python tools/decomp-diff.py -u mario/JSystem/JKernel/JKRSolidHeap -t function`
- `python tools/decomp-diff.py -u mario/JSystem/JKernel/JKRSolidHeap -d "state_register" --no-collapse`
- `python tools/decomp-diff.py -u mario/JSystem/JKernel/JKRSolidHeap -t object`
- `python configure.py --non-matching && ninja`
- `python configure.py && ninja`

2026-06-13 10:47am MNL recheck: verdict remains `equivalent`.
`state_register` still writes the same state id, calls virtual
`getTotalFreeSize`, stores the same used size, and computes the same check code
from `mCurEnd * 3 + mCurStart`. The only current diff is stack/save-slot size
and the temporary register holding the final add. Proof refreshed with
`python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
