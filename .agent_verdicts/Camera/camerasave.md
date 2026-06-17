---
unit: mario/Camera/camerasave
source: src/Camera/camerasave.cpp
verdict: equivalent
audited_at: 2026-06-12 4:16pm MNL
---

Verdict: equivalent
Time: 2026-06-13 6:33am MNL

## Summary

`Camera/camerasave.cpp` is functionally equivalent and source-link proven.

The five nonmatching constructors are all TParams/PARAM_INIT chains:

- `TCamSaveKindParam::TCamSaveKindParam(const char*)`
- `TCamSaveEx::TCamSaveEx()`
- `TCamSaveJetCoaster::TCamSaveJetCoaster()`
- `TCamSaveShake::TCamSaveShake(const char*)`
- `TCamSaveNotice::TCamSaveNotice()`

Each constructor initializes the same members at the same offsets, with the same
defaults, the same `TBaseParam` registrations, and the same final
`TParams::load(mPrmPath)` call. Remaining instruction diffs are local string
offset drift, vtable/local-symbol attribution drift, and TParam vtable/data
ownership rows.

## Source Correction

`TCamSaveEx::mInHouseMinFrame` was corrected from `TParamRT<s8>` to
`TParamRT<u8>`. The target asm stores `__vt__11TParamT<Uc>` /
`__vt__12TParamRT<Uc>` and writes the default with `stb`; the signed-char
source emitted `TParamT<signed char>` vtables and failed the source-link proof
because no signed-char `TParamT::load` body is linked.

## Proof

- `python tools/decomp-diff.py -u mario/Camera/camerasave -s missing`: no
  missing symbols.
- `python configure.py --non-matching && ninja`: linked from source after the
  `u8` correction.
- `python configure.py && ninja`: passed and verified `mario.dol: OK`.
- 2026-06-13 6:33am MNL recheck: overview still has no missing target rows,
  and `python configure.py --non-matching && ninja` linked from source.

2026-06-13 11:07am MNL recheck:
- Verdict remains `equivalent`.
- Current overview still has no missing target symbols. Re-read all five
  nonmatching constructors:
  `TCamSaveKindParam`, `TCamSaveEx`, `TCamSaveJetCoaster`,
  `TCamSaveShake`, and `TCamSaveNotice`.
- The constructors still build the same `TParams`/`TBaseParam` chains at the
  same object offsets, use the same defaults and type-specific `TParamRT`
  vtables, and end with the same `load(mPrmPath)` behavior where applicable.
- Remaining differences are local string/SDA label offsets and vtable/data-owner
  labels from rodata placement, not behavioral drift.
