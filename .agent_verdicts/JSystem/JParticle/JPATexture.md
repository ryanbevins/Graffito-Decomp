# mario/JSystem/JParticle/JPATexture

Verdict: equivalent
Date: 2026-06-13 8:39am MNL

Reverified current `Object(Equivalent, ...)` row during the audit sweep.

Reason:
- `JPADefaultTexture::initialize`, `JPATexture::~JPATexture`, the vtable, and
  constants still match.
- Full `--no-collapse` diff for `JPATexture::JPATexture(const u8*, JKRHeap*)`
  performs the same `JPADataBlock` base construction, vtable install,
  `JUTTexture` zero/init stores, raw-data `+0x20` pointer calculation, and
  `storeTIMG` call. The constructor residue is independent load/store
  scheduling before `storeTIMG`; the reordered load from `4(r31)` does not
  alias the stores to `0x30/0x58`.
- The nonmatching 12B `.data` row and weak `JPADataBlock` helper extras are
  owner drift, not target-absent runtime state.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored normal config and verified
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

2026-06-13 12:55pm MNL recheck:
- Current overview remains target-complete aside from weak owner drift.
- Re-read `JPATexture::JPATexture(const u8*, JKRHeap*)`; it still performs the
  same base construction, vtable install, `JUTTexture` zero/init stores,
  raw-data `+0x20` pointer calculation, and `storeTIMG` call. Residue is
  independent load/store scheduling and paired `JPADataBlock` owner drift.
  Reused this tick's successful source-link and normal DOL proof batch.

---

Verdict: equivalent
Date: 2026-06-13 4:42am MNL

Reason:
- Re-verified during the AUDIT sweep. `JPADefaultTexture::initialize`,
  `JPATexture::~JPATexture`, the `JPATexture` vtable, and constants match.
- `JPATexture::JPATexture(const u8*, JKRHeap*)` performs the same
  `JPADataBlock` base construction, vtable install, `JUTTexture` zero/init
  stores, raw-data `+0x20` pointer calculation, and `storeTIMG` call. The
  remaining constructor diff is codegen-class scheduling of independent
  loads/stores before the `storeTIMG` call.
- The nonmatching 12B `.data` row is paired with the source-owned
  `JPADataBlock::__vtable` extra; this is weak owner drift, not
  target-absent runtime state.
- The source object additionally emits weak `JPADataBlock::~JPADataBlock()`.
  Both helper extras are target-present weak symbols owned outside this TU.
- The existing `Object(Equivalent, ...)` classification linked successfully
  under the 4:42am batch `python configure.py --non-matching && ninja` proof;
  the plain matching build then passed with `mario.dol: OK`.

Offending functions: none.
