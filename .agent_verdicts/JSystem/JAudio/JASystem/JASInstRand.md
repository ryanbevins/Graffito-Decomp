# mario/JSystem/JAudio/JASystem/JASInstRand

Verdict: equivalent
Date: 2026-06-13 8:39am MNL

Reverified current `Object(Equivalent, ...)` row during the audit sweep.

Reason:
- `python tools/decomp-diff.py -u mario/JSystem/JAudio/JASystem/JASInstRand`
  still reports no missing or extra symbols.
- Full `--no-collapse` diff for `JASystem::TInstRand::getY(int, int) const`
  shows the same static random initialization, LCG update, synthesized float,
  and return math. The only residue is scratch stack slot and local label
  numbering.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` restored normal config and verified
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.

2026-06-13 12:43pm MNL recheck: verdict remains `equivalent`. Fresh full diff
for `JASystem::TInstRand::getY(int, int) const` still shows the same static
`TRandom_fast_` guard/constructor, LCG update constants, synthesized float
bit-pattern, scale/subtract math, `unkC` multiply, `unk8` add, and return. The
only differences are local label numbering and the stack scratch slot used to
reload the synthesized float. Proof reused from this tick: `python configure.py
--non-matching && ninja`, then normal `python configure.py && ninja` with
`build/GMSJ01/mario.dol: OK`.

---

Verdict: equivalent
Date: 2026-06-13 4:15am MNL

Reason:
- `python tools/decomp-diff.py -u mario/JSystem/JAudio/JASystem/JASInstRand`
  reports no missing or extra symbols.
- `JASystem::TInstRand::getY(int, int) const` is 100.0% and exact-size. The
  full `--no-collapse` diff shows identical static random initialization,
  linear-congruential RNG update, float synthesis, and return math. The only
  operand difference is the stack scratch slot used for the synthesized float
  (`r1+0x2c` target vs `r1+0x24` source), plus local label numbering.
- `python configure.py --non-matching && ninja` linked successfully with this
  object sourced; `python configure.py && ninja` restored the normal config and
  passed the DOL hash check.

Offending functions: none.
