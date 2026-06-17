Verdict: matching
Time: 2026-06-13 6:57am MNL
Unit: mario/Camera/CamShakeDefine
Source: src/Camera/CamShakeDefine.cpp

Reason:
- Data-only TU; there are no `.text` functions.
- `TCameraShake::mCamShakeNameSave`, the shake-name path strings, and the
  target `.data` row match exactly.
- Objdiff reports the two infectious strings as missing `@1490`/`@1526` and
  extra `dummyMactorStringValue1`/`SMS_NO_MEMORY_MESSAGE`; these are the same
  local bytes under source labels, not behavior-bearing external symbols.
- `python configure.py --non-matching && ninja` linked successfully after
  promoting the object to `Equivalent`; `python configure.py && ninja`
  restored the normal config and passed the DOL hash check.
- 2026-06-13 6:30am MNL recheck: overview still shows only the known
  anonymous-vs-source infectious rodata labels, and
  `python configure.py --non-matching && ninja` linked from source.
- 2026-06-13 6:57am MNL recheck: overview remains unchanged. Raw
  `powerpc-eabi-objdump -s -j .rodata` shows target and source rodata bytes are
  identical from the two infectious strings through the shake-name strings;
  `.data` still matches the `mCamShakeNameSave` pointer table. Current
  `--non-matching` source-link proof and normal DOL hash proof both passed in
  this audit tick.
- 2026-06-13 8:08am MNL promotion: changed the row from `Object(Equivalent, ...)` to `Object(Matching, ...)`; the normal `python configure.py && ninja` build passed and verified `build/GMSJ01/mario.dol: OK`.
