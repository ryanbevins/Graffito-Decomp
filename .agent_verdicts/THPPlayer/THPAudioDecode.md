# THPPlayer/THPAudioDecode audit

Verdict: equivalent
Status: equivalent
Updated: 2026-06-15 11:15pm MNL

Safety-net recheck for current
`Object(Equivalent, "THPPlayer/THPAudioDecode.c")`.

Behavior review:
- `PopDecodedAudioBuffer`, `PushFreeAudioBuffer`, `AudioDecode`,
  `AudioDecoder`, `AudioDecodeThreadCancel`, `AudioDecodeThreadStart`, and
  `CreateAudioDecodeThread` are exact or differ only by symbol-label display.
- `AudioDecoderForOnMemory(void*)` preserves the target behavior: initialize
  frame to 0, use `ActivePlayer.initReadSize`, build the local
  `THPReadBuffer`, call `AudioDecode`, compute
  `(frame + ActivePlayer.initReadFrame) % ActivePlayer.header.numFrames`, wrap
  to `ActivePlayer.movieData` when `playFlag & 1`, otherwise suspend the audio
  decode thread, and on non-wrap frames advance by the prior read size while
  loading the next size from the buffer. Remaining differences are
  register-coloring and local-slot choices.
- decomp-diff labels the `AudioDecode` branch target in the loop poorly because
  of address drift; raw target asm shows `bl AudioDecode__FP13THPReadBuffer`.

Data / extra-symbol review:
- No missing target symbols.
- Extra `PopFreeAudioBuffer` and `PushDecodedAudioBuffer` are source-owned
  helper bodies; their logic is inlined at target call sites and source-linking
  has no unresolved reference.
- Extra `__THPJpegNaturalOrder`, `__THPAANScaleFactor`, and `.rodata-0` rows
  are unreferenced data ownership from included THP/JPEG code, not behavior
  differences in this TU.

Validation:
- `python tools/decomp-diff.py -u mario/THPPlayer/THPAudioDecode`
- `python configure.py --non-matching && ninja`
- `python configure.py && ninja` passed with `build/GMSJ01/mario.dol: OK`.
