# Audit: M3DUtil/MActorData

Verdict: equivalent
Updated: 2026-06-13 6:51pm MNL

Unit: `mario/M3DUtil/MActorData`

Certification:
- Promoted `M3DUtil/MActorData.cpp` to `Object(Equivalent, ...)`.
- Proof passed: `python configure.py --non-matching && ninja` linked from
  source.
- Normal build gate passed after regenerating matching config:
  `python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.

Reviewed nonmatching functions:
- `MActorAnmData::addFileTable(const char*)`: same six extension probes,
  simple-name allocation/copy, key-code loop, table stores, and count bumps.
  Residue is frame size, saved-register coloring, stack slots, and sdata label
  ownership.
- `MActorAnmData::init(const char*, const char**)`: fixed behavior is present.
  Both `findFirstFile` scans use the normalized base path buffer, both file-list
  passes perform the same extension counting/table fill work as target, and all
  `loadAnmPtrArray` calls use the slash-appended path buffer. Residue is stack
  frame/slot offsets, GPR coloring, misleading helper labels from local symbol
  drift, and extra emitted helper owners.
- `MActorAnmData::partsNameToIdx(const char*)`: same list walk, `strcmp`, index
  return, and `-1` fallback. Residue is iterator temporary stack-slot offsets.
- `MActorAnmData::MActorAnmData()`: same member/list initialization and zero
  stores. Residue is frame size only.

Data / symbol notes:
- Current overview shows all `.sdata2` entries byte-identical and no missing
  target symbols.
- Extras `MActorAnmData::addFileNum`, `JKRFileFinder::~JKRFileFinder`,
  `strcmp_ignore_case`, `to_upper_hack`, and the `JKRFileFinder` vtable are
  owner/byte debt only; the source-link proof has no undefined-symbol blocker.
