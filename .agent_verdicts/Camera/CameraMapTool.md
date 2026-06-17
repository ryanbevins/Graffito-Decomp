# mario/Camera/CameraMapTool

Verdict: equivalent
Date: 2026-06-13 8:39am MNL

Reverified current `Object(Equivalent, ...)` row during the audit sweep.

Reason:
- `python tools/decomp-diff.py -u mario/Camera/CameraMapTool` still reports no
  missing target symbols. `TCameraMapTool::calcPosAndAt(...) const` matches
  exactly.
- Full `--no-collapse` diff for
  `TCameraMapTool::load(JSUMemoryInputStream&)` shows identical stream reads,
  field offsets, negative clamp, and branch structure; residue is stack
  frame/save-slot offsets only.
- The source-owned extra destructor is weak-helper ownership drift; the target
  vtable and data symbols match exactly.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored normal config and verified
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

2026-06-13 12:46pm MNL recheck: verdict remains `equivalent`. Fresh full diff
for `TCameraMapTool::load(JSUMemoryInputStream&)` still has identical base load,
nine field reads, negative clamp on the stored field at `0x28`, and branch
predicate. `calcPosAndAt` remains byte-exact; the source-owned destructor is
weak-helper ownership drift. Proof refreshed with `python configure.py
--non-matching && ninja`, then normal `python configure.py && ninja` with
`build/GMSJ01/mario.dol: OK`.

---

Verdict: equivalent
Date: 2026-06-13 4:42am MNL

Reason:
- Re-verified during the AUDIT sweep. `python tools/decomp-diff.py -u
  mario/Camera/CameraMapTool` reports no missing target symbols.
- `TCameraMapTool::calcPosAndAt(JGeometry::TVec3<float>*,
  JGeometry::TVec3<float>*) const` matches exactly.
- `TCameraMapTool::load(JSUMemoryInputStream&)` is exact-size and 99.9%.
  The refreshed full `--no-collapse` diff shows identical stream reads,
  field offsets, negative clamp, and branch structure; the only residue is
  stack frame / save-slot offsets (`0x38` target vs `0x18` source).
- The only extra is an unreferenced `TCameraMapTool::~TCameraMapTool()` helper;
  the target vtable and data symbols match exactly.
- Promoting `Camera/CameraMapTool.cpp` to `Object(Equivalent, ...)` linked
  cleanly under `python configure.py --non-matching && ninja`; retained as
  `Equivalent` pending the batch source-link proof.

Offending functions: none.
