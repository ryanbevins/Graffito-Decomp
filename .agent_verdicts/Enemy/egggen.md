# mario/Enemy/egggen

Verdict: equivalent
Date: 2026-06-13 5:13pm MNL

Certified `Object(Equivalent, "Enemy/egggen.cpp")` after the
`TYoshi::onYoshi()` owner split removed the prior source-link blocker.

Behavior review:

- `TEggGenManager::createModelData()` differs only by the label/name of the
  static `TModelDataLoadEntry` table passed to the same virtual
  `createModelDataArray` call; table contents are equivalent.
- `TEggGenerator::init(TLiveManager*)` performs the same manager registration,
  `TMActorKeeper` allocation/construction, `"gene_egg_model1.bmd"` actor
  creation, hit actor initialization, BCK setup, and `MsWrap` rotation
  normalization. Remaining displayed differences are local data-label/owner
  drift around the same constants and loops.
- `TEggGenerator::control()` is byte-identical after the inline-result
  materialization work from implementation mode.

Proof:

- `python tools/decomp-diff.py -u mario/Enemy/egggen --search
  "TYoshi::onYoshi"` reports no duplicate weak-owner row.
- `python configure.py --non-matching && ninja` linked successfully from source.

Verdict: needs_impl  
Date: 2026-06-13 11:55am MNL

Do not promote yet. The visible behavior is effectively reconstructed:
`TEggGenerator::control()` is byte-identical, `createModelData()` calls the same
model-data virtual with an equivalent static entry table, and `init()` differs
only by label/owner drift around the same actor setup, BCK setup, hit actor
radii, and `MsWrap` rotation normalization.

Current blocker is source-link ownership, not runtime logic:

- A temporary `Object(Equivalent, "Enemy/egggen.cpp")` promotion failed
  `python configure.py --non-matching && ninja`.
- Linker error: multiply-defined `TYoshi::onYoshi()` in `egggen.o`, previously
  defined in `MarioAction.o`.

Keep this `NonMatching` until the weak/helper ownership for `TYoshi::onYoshi()`
is corrected or suppressed naturally in this TU.
