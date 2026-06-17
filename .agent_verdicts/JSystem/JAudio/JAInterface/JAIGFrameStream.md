# JSystem/JAudio/JAInterface/JAIGFrameStream

Verdict: equivalent
Date: 2026-06-13 3:04am MNL

Reason: verified functionally identical and source-linkable. The six nonmatching
text functions preserve behavior; remaining diffs are codegen/SDA-label noise,
stack/register allocation, local static init guard placement, and helper-owner
label drift from empty inline helper extras.

Reviewed functions:
- `JAIBasic::checkPlayingStream()` — stream release/stop paths, parameter move
  loops, volume/pitch/pan writes, flag clearing, and final frame increment match.
  Apparent call-label drift is caused by local helper symbol ownership; raw asm
  calls the expected StreamLib helpers.
- `JAInter::StreamLib::Play_DirectPCM(...)` — DSP buffer setup, mixer init loop,
  bus connections, pitch, playStart, and flush match; diff is saved-register
  coloring only.
- `JAInter::StreamLib::__DecodePCM()` — PCM deinterleave, loadup sample update,
  and both DCStoreRange calls match; objdiff SDA labels are misleading.
- `JAInter::StreamLib::__DecodeADPCM()` — L/R history reset, loop-skip handling,
  ADPCM predictor/filter decode, ring-buffer writes, playside wrap, DC stores, and
  shift_sample update match. Remaining residue is expression/register shape and
  an equivalent unreachable tail check.
- `JAInter::StreamLib::__start()` — header read/copy, header struct copy, stream
  state reset, DVD pause/load kickoff, DSP channel free/nulling, and dealloc flag
  match.
- `JAInter::StreamLib::callBack(void*)` — start gating, pending volume/pitch/pan
  commits, DSP allocation/failure, DVD pause/resume, decode scheduling, initial
  playback setup, stop handling, mixer/pitch updates, loop restart, stop, and
  LoadADPCM paths match behaviorally.

Proof:
- `python tools/decomp-diff.py -u mario/JSystem/JAudio/JAInterface/JAIGFrameStream`
  reported no missing target text symbols.
- `python configure.py --non-matching && ninja` linked `mario.dol` from source.

## 2026-06-13 11:24am MNL recheck

Refreshed during the stale-Equivalent sweep. The rebuilt source object has only
expected external SDK/JAudio references. Full diffs rechecked
`JAIBasic::checkPlayingStream()` and `JAInter::StreamLib::__DecodeADPCM()`, and
the large callback diff was reviewed with context. The stream release paths,
volume/pitch/pan parameter loops, ADPCM predictor/history updates, ring-buffer
writes, DCStoreRange calls, DSP allocation/free/pause/flush paths, and playback
state updates remain behavior-identical. Remaining drift is stack/register
allocation, local static-label placement, SDA symbol labels, and equivalent
loop/controller shape.

Proof passed:
- `python configure.py --non-matching && ninja`
- `python configure.py && ninja` (`build/GMSJ01/mario.dol: OK`)
