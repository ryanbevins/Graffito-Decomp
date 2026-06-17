# Audit verdict: equivalent

- Date: 2026-06-13 7:10am MNL
- TU: `mario/JSystem/JKernel/JKRArchivePri`
- Source: `src/JSystem/JKernel/JKRArchivePri.cpp`
- Verdict: `equivalent`

## Reason

This duplicate stale note previously recorded an early failed solo promotion.
The current `configure.py` already classifies the TU as
`Object(Equivalent, ...)`, and the coordinated archive ownership now links from
source. Re-audit of the current object found only stack/local-label/vtable-owner
drift; raw objdump confirms the recursive `findDirectory` call target is
correct despite objdiff label noise.

## Proof

- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored the matching config and passed with
  `build/GMSJ01/mario.dol: OK`.

## Recheck — 2026-06-16 12:11am MNL

Verdict remains `equivalent`. Current overview still has no missing target
symbols; non-exact rows are the constructor frame delta, CArcName local-slot
placement in `findDirectory` / `findTypeResource` / `findFsResource` /
`findNameResource`, and source-owned `isSameName`, `findResType`, and vtable
rows. The recursive resource lookup paths still compare the same hashes/names,
advance the same file-entry loops, return the same directory/file-entry
pointers, and use the same null/found exits. Today's
`python configure.py --non-matching && ninja` proof linked all current
`Equivalent` rows from source, including this TU; normal matching build passed
`build/GMSJ01/mario.dol: OK`.
