# THPPlayer/THPAudioDecode.c

Verdict: equivalent
Date: 2026-06-13 9:22pm MNL

Unit: `mario/THPPlayer/THPAudioDecode`
Source: `src/THPPlayer/THPAudioDecode.c`
Classification: `Object(Equivalent, "THPPlayer/THPAudioDecode.c")`

## Review

- All target-visible functions are byte-identical except
  `AudioDecoderForOnMemory(void*)`.
- `AudioDecoderForOnMemory` preserves behavior: initializes frame to zero,
  seeds `readSize` from `ActivePlayer.initReadSize`, decodes each synthetic
  `THPReadBuffer`, computes wrap position from `frame + initReadFrame` modulo
  `header.numFrames`, loops back to `movieData` when `playFlag & 1`, suspends
  the decode thread otherwise, advances the read pointer by the previous
  `readSize`, updates `readSize` from the frame header, and increments frame.
- Residue is register allocation/scheduling. The diff label on the decode call
  is misleading; source calls `AudioDecode`, and the surrounding exact
  `AudioDecode` body confirms the same decode path.
- Source-only `PopFreeAudioBuffer` / `PushDecodedAudioBuffer` bodies are the
  queue helpers inlined into exact target-visible users; they are weak/owner
  byte debt, not a source-link behavior blocker.

## Validation

- Shared proof from this tick: `python configure.py --non-matching && ninja`
  linked successfully with current `Equivalent` rows.
- Normal `python configure.py && ninja` passed with `build/GMSJ01/mario.dol:
  OK`.
