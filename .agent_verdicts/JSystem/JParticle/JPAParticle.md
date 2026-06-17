Verdict: equivalent
Time: 2026-06-13 6:30am MNL
Unit: mario/JSystem/JParticle/JPAParticle
Source: src/JSystem/JParticle/JPAParticle.cpp

Reason:
- All target functions match exactly except
  `JPAParticle::checkCreateChildParticle()`.
- Full `--no-collapse` diff for `checkCreateChildParticle()` shows identical
  guard logic, float comparisons, modulo test, result stores, calls, and return
  value. The only residue is stack frame/save-slot offsets (`0x58` target vs
  `0x48` source), so it is codegen-class equivalent.
- Vtables and all target data rows match exactly. Objdiff extras are
  unreferenced standalone helper/destructor/accessor bodies.
- Re-verification of existing `Object(Equivalent, ...)` linked cleanly under
  `python configure.py --non-matching && ninja`.
- 2026-06-13 6:30am MNL recheck: overview still has no missing target rows,
  and `python configure.py --non-matching && ninja` linked from source.
- 2026-06-13 9:57am MNL recheck: overview still has no missing target rows.
  Full `--no-collapse` diff for `checkCreateChildParticle()` still shows the
  same emitter-info guard, lifetime ratio calculation, threshold comparison,
  modulo child-spawn gate, return value, and all constants/stores. Residue is
  stack-frame/save-slot offsets only. Proof batch passed:
  `python configure.py --non-matching && ninja`, then `python configure.py &&
  ninja` with `mario.dol: OK`.
- 2026-06-13 1:21pm MNL recheck: current overview still has no missing rows.
  `checkCreateChildParticle()` remains behavior-identical: same emitter enable
  guard, lifetime ratio/default path, threshold compare, child interval modulo
  test, and returned bool. Only frame/save-slot offsets differ; extra text rows
  are helper/destructor/accessor ownership. Source-link and normal proof reruns
  passed.
