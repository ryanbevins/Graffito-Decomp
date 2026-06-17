# mario/Strategic/livemanager

Verdict: equivalent
Date: 2026-06-13 7:25am MNL

Reason: all functions except `TLiveManager::perform()` match exactly. `perform`
has the same perform-flag gate, optional `TTimeRec` start/end timer blocks,
`clipActors`, `setFlagOutOfCube`, and base `TObjManager::perform` call. The
residue is codegen-class: the known `TTimeRec::startTimer` stack-frame
inflation/slot offsets and shifted local branch labels. The remaining `.data`
residue is vtable relocation ownership for target-present weak destructors.

Proof:
- `python tools/decomp-diff.py -u mario/Strategic/livemanager -t function`
- `python tools/decomp-diff.py -u mario/Strategic/livemanager -d "TLiveManager::perform" --no-collapse`
- `python configure.py --non-matching && ninja`
- `python configure.py && ninja`

Reverified in the current audit sweep. The current `TLiveManager::perform`
diff still has identical control flow and calls (`TTimeRec` start/end,
`clipActors`, `setFlagOutOfCube`, base `TObjManager::perform`); residue remains
only the known `TTimeRec` frame/slot inflation and local label shifts. Source
link proof passed in the same batch as the 2026-06-13 7:25am MNL notes refresh.

2026-06-13 10:45am MNL recheck: verdict remains `equivalent`. Re-read the
current `TLiveManager::perform` diff; it still has the same perform-flag gate,
optional `TTimeRec` start/end timer calls, `clipActors`, `setFlagOutOfCube`,
and base `TObjManager::perform` call. The only drift is the known
`TTimeRec`-related frame/slot inflation and branch-label shifts. Proof
refreshed with `python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
