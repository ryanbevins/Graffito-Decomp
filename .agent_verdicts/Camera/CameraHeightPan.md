# mario/Camera/CameraHeightPan

Verdict: equivalent
Date: 2026-06-13 1:07pm MNL

Reason:
- Reverified current `Object(Equivalent, "Camera/CameraHeightPan.cpp")` again
  during the audit-only sweep.
- `CPolarSubCamera::execHeightPan_()` matches the target behavior. The large diff is function/order/register drift: source and target both chase height-pan offset, test the same camera/Mario predicates, update flags at `0x64`, chase field `0x84`, and reset/lock against `0x9C` using the same conditions.
- `CPolarSubCamera::killHeightPanWhenChangeCamMode_()` is equivalent. The source emits a duplicate `bgt` after the `mMode - 8 <= 0x39` range test, but the condition register is unchanged, so the second branch cannot alter behavior.
- The `.data` mismatch is jump-table relocation/layout residue for the two switch tables. `killHeightPan_()` and `isNotHeightPanCamMode_()` byte-match.

Proof:
- `python tools/decomp-diff.py -u mario/Camera/CameraHeightPan`
- `python tools/decomp-diff.py -u mario/Camera/CameraHeightPan -d "execHeightPan" --no-collapse`
- `python tools/decomp-diff.py -u mario/Camera/CameraHeightPan -d "killHeightPanWhenChangeCamMode" --no-collapse`
- Raw `build/GMSJ01/asm/Camera/CameraHeightPan.s` confirms target `execHeightPan_()` calls `isNotHeightPanCamMode_()`; decomp-diff's pretty label in that spot is misleading.
- Shared proof: `python configure.py --non-matching && ninja` passed, then `python configure.py && ninja` restored the normal matching config and verified `build/GMSJ01/mario.dol: OK` at 2026-06-13 1:07pm MNL.
