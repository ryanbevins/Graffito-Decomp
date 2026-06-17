# MSound/MSoundBGM Audit

Verdict: equivalent
Date: 2026-06-13 11:04pm MNL

Certified as behavior-equivalent and promoted to `Object(Equivalent,
"MSound/MSoundBGM.cpp")`.

Evidence:
- Current overview has no missing symbols. The only extra rows are static-list
  destructor owners (`JSUList<...>::~JSUList()` and `JSULink<MSBgm>::~JSULink()`),
  which are byte/ownership debt only because the TU source-links cleanly.
- `MSBgm::setStageBgmYoshiPercussion(bool)` differs only by stack-frame size,
  register copy form, and local rodata label naming. It keeps the same track-0
  null guard, `getBstSwitch(sound->unk8)` call, `0x10000000` bit test,
  `getSeqParameter()` / `TrackMgr::handleToSeq()` lookup, child-track load, and
  `unk3C2 = param ? 0 : 1` byte store.
- `MSBgm::stopTrackBGM(u8,u32)` is structurally identical; the diff is only a
  smaller rebuilt frame and corresponding stack offsets.
- `MSBgm::startBGM(u32)` keeps the same `JALListS<MSBgm,u32>::search(param &
  0x3ff)` lookup, special-demo BGM stop loop, `demoModeIn(0x16f,false)`,
  `startSoundActor`, `setVolume(smMainVolume,0,8)`, track-table write,
  `getSceneNo` filter (`-1`/`0x210` skipped), and `setWaveReadMode` call.
  Label names in decomp-diff are shifted by local text ownership, not behavior.
- `MSBgm::init()` now calls the recovered `JALList<MSBgm>::JALList(MSBgm*,bool)`
  constructor for each allocated `MSBgm`, then stores `unk10` and clears `unk14`,
  matching target behavior; remaining diff is frame size/register operands.
- Proof: `python configure.py --non-matching && ninja` linked from source, then
  `python configure.py && ninja` restored the matching config and passed with
  `build/GMSJ01/mario.dol: OK`.
