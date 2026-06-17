# Map/MapStaticObject audit

Verdict: `equivalent`

Updated 2026-06-14 12:27am MNL in AUDIT mode.

`Map/MapStaticObject.cpp` is behavior-equivalent and now links from source
under `--non-matching`.

Functional fixes made during audit:

- `TMapStaticObj::init(const char*)` now searches `actor_data_table` until
  `strcmp(name, entry->unk0) == 0`; the previous source stopped on the first
  nonmatching row.
- `actor_data_table` entries for `TopOfCorona` and
  `BiancoBossEffectLight` now set `unk3C == 1`. The previous initializer used
  `0x1000000` for a `u8` field, truncating to zero and skipping particle
  loading/emission.

Behavior review:

- `TMapObjSoundGroup::perform(...)`: same dummy/perform-bit gates, graph point
  traversal, camera-position projection through `MsPerpendicFootToLineR`,
  skip-on-single-connection behavior, and `MSSceneSE::frameLoop` arguments.
  Remaining diff is stack/register layout and vector construction/copy shape.
- `MsPerpendicFootToLineR(...)`: same formula,
  `start + clamp(dot(camera - start, end - start) / |end - start|^2, 0, 1) *
  (end - start)`. Remaining diff is temporary materialization/FPR scheduling.
- `TMapStaticObj::init(...)`: after the fixes above, same actor-data search,
  hit actor init, optional model/collision/particle setup, parent-group list
  insertion, screen texture replacement, `initUnique`, and mirror draw-buffer
  registration. Remaining diff is stack/frame shape, branch layout, inlined
  `initMapCollision`, and JGadget iterator construction shape.
- `TMapStaticObj::initModel(...)`, `initUnique(...)`, and `perform(...)` are
  behavior-identical; residue is local label drift, frame/register shape,
  helper ownership, and equivalent branch layout.
- `actor_data_table` and `sound_info` were checked with pointer relocations
  masked; numeric fields now match the target. Remaining data drift is
  relocation/owner/label layout.

Proof:

- `python configure.py --non-matching && ninja` linked successfully with this
  TU from source.
- `python configure.py && ninja` passed afterward with
  `build/GMSJ01/mario.dol: OK`.
