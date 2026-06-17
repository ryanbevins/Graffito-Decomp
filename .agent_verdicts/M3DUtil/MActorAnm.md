# mario/M3DUtil/MActorAnm

Verdict: equivalent
Date: 2026-06-13 8:27am MNL

Reason: all target behavior-bearing symbols are present and the two
nonmatching emitted functions are codegen-only:

- `MActorAnmBtp::checkUseMaterialIDInit(unsigned short*)`: instruction stream
  and branch structure are identical; only the target frame is 8 bytes larger
  (`0x80` vs `0x78`) and the `stmw`/`lmw` stack offsets follow from that.
- `MActorAnmBck::setAnmFromIndex(int, unsigned short*)`: instruction stream and
  branch structure are identical; only the target frame is 8 bytes larger
  (`0x40` vs `0x38`) and saved-register restore offsets follow from that.

The nonmatching data row and extra weak/template/vtable symbols are ownership
and label drift only; the overview has no missing target rows.

Proof:
- `python configure.py --non-matching && ninja` linked from source.
- `python configure.py && ninja` passed afterward and verified
  `build/GMSJ01/mario.dol: OK`.

2026-06-13 12:36pm MNL recheck: verdict remains `equivalent`.
Fresh `--no-collapse` diffs for `MActorAnmBtp::checkUseMaterialIDInit` and
`MActorAnmBck::setAnmFromIndex` still show the same material-table scans,
name comparisons, copied material ids, motion-blend keep/set calls,
frame-controller init, frame-rate store, and branch conditions. Remaining drift
is stack-frame/save-slot size plus helper/vtable ownership extras. Proof
refreshed with `python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
