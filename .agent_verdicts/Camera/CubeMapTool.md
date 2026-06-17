# mario/Camera/CubeMapTool

Verdict: equivalent
Date: 2026-06-13 8:39am MNL

Reverified current `Object(Equivalent, ...)` row during the audit sweep.

Reason:
- `python tools/decomp-diff.py -u mario/Camera/CubeMapTool` still reports no
  missing target symbols; vtables, `.data`, and `.sdata2` match exactly.
- Full diffs for `TCubeStreamInfo::load`, `TCubeCameraInfo::load`, and
  `TCubeGeneralInfo::load` show identical read order, field offsets, scale
  conversion, key-code lookup, and final field writes. Residue is stack
  frame/save-slot offsets around temporary reads and int-to-float conversion.
- Extra `searchF(...)` and `TCubeGeneralInfo` destructor bodies are unreferenced
  helper ownership drift.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored normal config and verified
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

2026-06-13 12:55pm MNL recheck:
- Current overview still has no missing target symbols.
- Re-read `TCubeStreamInfo::load`, `TCubeCameraInfo::load`, and
  `TCubeGeneralInfo::load`; read order, field offsets, scale conversion,
  key-code lookup, and final field writes remain behavior-identical. Residue is
  stack frame/save-slot layout around temporary reads and int-to-float
  conversion slots, plus unreferenced helper ownership. Reused this tick's
  successful source-link and normal DOL proof batch.

---

Verdict: equivalent
Date: 2026-06-13 4:42am MNL

Reason:
- Re-verified during the AUDIT sweep. `python tools/decomp-diff.py -u
  mario/Camera/CubeMapTool` reports no missing target symbols. Vtables,
  `.data`, and `.sdata2` all match exactly.
- `TCubeStreamInfo::~TCubeStreamInfo()` and `TCubeCameraInfo::~TCubeCameraInfo()`
  match exactly.
- `TCubeGeneralInfo::load`, `TCubeCameraInfo::load`, and
  `TCubeStreamInfo::load` are exact-size. The refreshed full diffs have
  identical read order, field offsets, scale conversion, key-code lookup, and
  final field writes. The remaining diffs are stack frame / save-slot offsets
  around temporary reads and int-to-float conversion slots.
- The extras are unreferenced helper bodies:
  `TNameRefAryT<TCameraMapTool, JDrama::TNameRef>::searchF(...)` and
  `TCubeGeneralInfo::~TCubeGeneralInfo()`.
- Promoting `Camera/CubeMapTool.cpp` to `Object(Equivalent, ...)` linked
  cleanly under `python configure.py --non-matching && ninja`; retained as
  `Equivalent` pending the batch source-link proof.

Offending functions: none.
