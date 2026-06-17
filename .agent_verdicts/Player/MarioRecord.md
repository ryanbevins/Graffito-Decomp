# mario/Player/MarioRecord

Verdict: equivalent
Date: 2026-06-13 8:39am MNL

Reverified current `Object(Equivalent, ...)` row during the audit sweep.

Reason:
- `python tools/decomp-diff.py -u mario/Player/MarioRecord` still reports no
  missing or extra symbols; only `TMarioInputReplay::init(unsigned char*)`
  remains nonmatching at 99.9%.
- Full `--no-collapse` diff plus raw target/source relocations confirm
  `init()` calls the same five `reset()` specializations in order: float,
  short, unsigned short, unsigned char, unsigned char. The misleading
  decomp-diff call labels are weak-symbol layout drift.
- The replay header writes, duration/value pointer setup, and manager resets
  are identical. Remaining residue is only stack-frame size.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored normal config and verified
  `build/GMSJ01/mario.dol: OK`.

Functions checked:
- `TMarioInputReplay::init(unsigned char*)`

2026-06-13 12:48pm MNL recheck: verdict remains `equivalent`. Fresh full diff
for `TMarioInputReplay::init(unsigned char*)` still shows the same replay
header zeroing, duration/value pointer setup, and five manager reset call
sites. Raw relocations in both target and rebuilt objects again confirm the
same reset sequence despite misleading diff labels: `float`, `short`,
`unsigned short`, `unsigned char`, `unsigned char`. The residue is frame size
plus weak-symbol layout. Proof reused from this tick:
`python configure.py --non-matching && ninja`, then normal `python configure.py
&& ninja` with `build/GMSJ01/mario.dol: OK`.

---

Verdict: equivalent
Date: 2026-06-13 4:11am MNL

Reason:
- `python tools/decomp-diff.py -u mario/Player/MarioRecord` reports no missing
  or extra symbols; only `TMarioInputReplay::init(unsigned char*)` remains
  nonmatching at 99.9%.
- The old `not_equivalent` verdict came from misleading decomp-diff call-label
  rendering after weak-function order drift. Raw target asm shows
  `TMarioInputReplay::init()` calls the five `reset()` instantiations in order:
  float, short, unsigned short, unsigned char, unsigned char. Source-object
  relocations show the same `reset__...` targets at each callsite.
- `init()` writes the same replay header fields, duration/value pointers, and
  manager reset calls. The remaining residue is stack-frame size and weak-symbol
  layout/codegen drift, not behavioral.
- `python configure.py --non-matching && ninja` linked successfully with this
  object sourced; `python configure.py && ninja` restored the normal config and
  passed the DOL hash check.

Functions checked:
- `TMarioInputReplay::init(unsigned char*)`
