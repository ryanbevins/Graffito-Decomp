verdict: equivalent
date: 2026-06-13 5:17pm MNL
unit: mario/Map/Shimmer
source: src/Map/Shimmer.cpp
classification: Object(Equivalent, "Map/Shimmer.cpp")

Certified after the `TYoshi::onYoshi()` owner split removed the prior
source-link blocker.

Behavior review:
- `TShimmer::TShimmer(const char*)` differs only by relocation/owner labels
  around the same base/member construction, vtable setup, defaults, and
  `J3DFrameCtrl` allocation/init.
- `TShimmer::perform(unsigned long, JDrama::TGraphics*)` keeps the same Mario
  state early return, frame update, cap/ground-plane shimmer position choice,
  light-effect texture matrix, transform/matrix concatenation, model
  `setBaseTRMtx`/`calc`/`viewCalc`, and conditional `entry()` behavior.
- Remaining drift is stack/frame size, matrix temporary layout, float-vs-word
  copies of identical position bits into `J3DTransformInfo`, and helper/data
  owner labels.
- `TYoshi::onYoshi()` no longer appears as an extra in this TU.

Proof:
- `python configure.py --non-matching && ninja` linked successfully from source.

verdict: needs_impl
date: 2026-06-13 3:10pm MNL
unit: mario/Map/Shimmer
source: src/Map/Shimmer.cpp

Reason:
- The previous behavior blocker in `TShimmer::perform(unsigned long,
  JDrama::TGraphics*)` is fixed: after setting the base transform matrix,
  source now calls `J3DModel::calc()` then `J3DModel::viewCalc()` (target
  vtable slots `0x10` and `0x14`), and the `0x200` path now calls
  `J3DModel::entry()` (slot `0x0c`).
- Do not promote yet. `python configure.py --non-matching && ninja` fails to
  link because source-linked `Shimmer.o` emits a strong
  `TYoshi::onYoshi()` from the `Player/MarioMain.hpp` / `Player/Yoshi.hpp`
  include chain, which conflicts with `MarioAction.o`.

Current state:
- `TShimmer::perform()` remaining diffs are codegen/data-owner class:
  stack/frame size, matrix temporary layout, float-vs-word copies of the same
  `mPosition` bits into `J3DTransformInfo`, and helper owner labels.
- No target symbols are missing in the object diff.

Source-link blocker:
- `onYoshi__6TYoshiFv` multiply-defined in `Shimmer.o`; previously defined in
  `MarioAction.o`.
- A future fix should remove or narrow the `Player/MarioMain.hpp` dependency
  without changing `perform()` behavior/codegen, or otherwise resolve
  `TYoshi::onYoshi()` ownership correctly.
- Do not try adding `inline` to the `Yoshi.hpp` body: tested at 3:18pm MNL,
  and MWCC stopped emitting the owning `MarioMove` weak symbol.

Proof:
- `python configure.py && ninja` passes with the source fix while the TU
  remains `NonMatching`.
- Attempted `python configure.py --non-matching && ninja` after a temporary
  `Equivalent` promotion; link failed with the duplicate `TYoshi::onYoshi()`
  error above, so the promotion was reverted.
