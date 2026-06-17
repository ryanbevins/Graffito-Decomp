# mario/JSystem/JAudio/JASystem/JASDvdThread

Verdict: equivalent
Status: equivalent
Time: 2026-06-15 4:36pm MNL

## Verdict

Re-verified the existing `Object(Equivalent, ...)` classification. This TU is
source-linkable and behavior-identical; remaining drift is codegen/helper
boundary only.

## Evidence

- Current overview has no missing or extra symbols.
- All data sections are byte-identical.
- All text functions are exact except `JASystem::Dvd::loadFileDvdT(char*, void*)`
  and `JASystem::Dvd::updateBuffer()`.
- `loadFileDvdT` is behavior-equivalent: the target inlines the same
  `loadToDramDvdT(0, path, buffer, 0, 0, &done, nullptr)` queue setup that the
  source performs through the exact helper, then both spin on the completion word
  and map `0xffffffff` to `0`.
- `updateBuffer` is codegen-class residue. Raw target asm at
  `build/GMSJ01/asm/JSystem/JAudio/JASystem/JASDvdThread.s:1167` confirms the
  function guards on `nextBuffers`, copies `nextBuffers`/`nextBufferSize` into
  `buffers`/`buffersize`, fills `audioDvdBuffer[i]` from
  `nextBufferTop += nextBufferSize`, then clears `nextBuffers` and
  `nextBufferTop`. The displayed local-symbol names in decomp-diff are label
  drift; the memory operations are the same.
- Proof reused from this tick after `Enemy/tamaNoko`: `python configure.py
  --non-matching && ninja` linked all current `Equivalent` objects from source,
  and plain `python configure.py && ninja` passed `build/GMSJ01/mario.dol: OK`.
