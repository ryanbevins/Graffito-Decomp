# Enemy/DebuTelesa

Verdict: equivalent
Date: 2026-06-12 2:38pm MNL

Reason: all target text functions are present and the remaining five text diffs
are codegen-class: stack size/slot movement in `clipEnemies`, `load`,
`receiveMessage`, and `calcRootMatrix`; saved-register coloring in
`clipEnemies`; and local label ownership for the model-data entry/string/
particle-helper references in `createModelData`, `load`, and `calcRootMatrix`.
No branch condition, call argument, field offset, constant, or side-effect order
diff changes behavior.

Data note: objdiff reports anonymous missing data labels (`@2602`, `@2604`,
`entry$2835`) and source-named extras from `InfectiousStrings` /
`TModelDataLoadEntry`; these are label/ownership residue rather than missing
runtime data for this TU.

Proof: `python configure.py --non-matching && ninja` linked from source, then
plain `python configure.py && ninja` passed and verified `mario.dol: OK`.

2026-06-13 8:13am MNL recheck:
- Overview still has no missing target text functions. The missing
  `@2602`/`@2604`/`entry$2835` rows and source extras remain static-data /
  label-ownership drift.
- `TDebuTelesaManager::clipEnemies`: current full diff preserves the same
  frustum setup, object loop, position copy, other-fast-cube check, clipped
  live-flag set/clear, and loop bound. Residue is stack frame/slot size and
  saved-register coloring.
- `TDebuTelesaManager::createModelData`: current full diff is only the
  model-data table label number; the virtual `createModelDataArray` call and
  arguments match.
- `TDebuTelesaManager::load`: current full diff preserves parameter allocation,
  `TSmallEnemyParams` construction/load, `unk38` assignment, parameter field
  stores, base manager load, and `unk5C = 0`. Residue is stack slot placement
  and temporary reload shape around the newly allocated params pointer.
- `TDebuTelesa::receiveMessage`: current full diff preserves the same message
  switch, false returns for 0/1/12, sound gate/start for message 11, and
  fallback to `TSmallEnemy::receiveMessage`. Residue is stack frame size.
- `TDebuTelesa::calcRootMatrix`: current full diff preserves the taken/dead
  guards, right-hand joint matrix position copy, `mTipPos` stores, and both
  `SMS_EasyEmitParticle` calls with the same effect IDs and scale vectors.
  Direct objdump of target/source objects confirms both particle-call
  relocations match; objdiff's displayed symbol names are label-attribution
  noise. Residue is stack slot placement and local constant labels.
- Proof refreshed in the same audit sweep: `python configure.py --non-matching
  && ninja` linked from source, and normal `python configure.py && ninja`
  verified `build/GMSJ01/mario.dol: OK`.

2026-06-13 11:44am MNL recheck: verdict remains `equivalent`. Current overview
is unchanged: no missing target text functions, same five nonmatching text
rows, and the same anonymous ctor/data labels paired with source-named
infectious/model-entry extras. The prior full-diff review remains valid:
manager clipping/model-data/load, message handling, and root-matrix particle
emission preserve the same calls, stores, constants, and branch semantics.
Shared proof from this tick passed: `python configure.py --non-matching &&
ninja`, then normal `python configure.py && ninja` verified
`build/GMSJ01/mario.dol: OK`.
