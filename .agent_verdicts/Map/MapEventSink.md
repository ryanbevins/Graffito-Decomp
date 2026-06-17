# Audit verdict: equivalent

Date: 2026-06-14 9:03pm MNL
Mode: AUDIT
Unit: `mario/Map/MapEventSink`
Source: `src/Map/MapEventSink.cpp`

Verdict: `equivalent`

Fixed before certification:
- `TMapEventSinkBianco::loadAfter()` now calls
  `TMapEventSinkInPollution::initBuriedBuilding()` before the Bianco-specific
  alive/kill loop. The old source called the pollution `loadAfter()` path,
  registering pollution-counter objects instead of restoring already-cleaned
  buildings.
- Scoped `#pragma dont_inline` around `initBuriedBuilding()` so the Bianco
  caller keeps the target helper call boundary. Object relocation confirms the
  call targets `initBuriedBuilding__24TMapEventSinkInPollutionFv`; objdiff's
  right-side label drift is only symbol-order noise.

Reason:
- No target text symbols are missing. Remaining missing/extra rows are local
  rodata/data-label and source-only helper/destructor ownership debt.
- Rechecked the nonmatching functions after the fix. Remaining diffs are
  behavior-neutral stack/register/FPR and helper-boundary residue in the base
  event sink, pollution sink, reset sink, Bianco, and Shadow Mario paths.

Proof:
- `python configure.py && ninja` passed after the source edit.
- `python configure.py --non-matching && ninja` linked with
  `Map/MapEventSink.cpp` sourced.
- `python configure.py && ninja` restored matching config and passed
  `build/GMSJ01/mario.dol: OK`.

Byte-debt to leave for INVESTIGATION:
- Local rodata/data label drift, including target `@3151` versus source
  label/owner placement.
- Source-only weak/destructor/helper owners such as `TMapEvent::~TMapEvent`,
  `TEventWatcher::~TEventWatcher`, `TMapCollisionBase::setUpTrans`, and
  infectious string pointer extras.
