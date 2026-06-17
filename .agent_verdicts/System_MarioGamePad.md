## Verdict: equivalent

Date: 2026-06-13 9:44am MNL

Reason: legacy duplicate note refreshed to match the canonical
`audit/System/MarioGamePad.md` verdict. Current audit sweep rules explicitly
treat the explicit stack-allocation local in `TMarioGamePad::updateMeaning()`
as byte/codegen debt once the emitted operations are verified. This TU links
from source and the reviewed diffs are behavior-equivalent.

Function audit notes:

- `TMarioGamePad::read()`: behavior-equivalent; stack slot/register allocation
  and local-label numbering only.
- `TMarioGamePad::updateMeaning()`: full `--no-collapse` diff has zero
  opcode/insert/delete markers. Remaining differences are the frame size,
  stack slot offsets, register coloring, and local constant labels.
- `.sdata2`: target order starts `1.0f, 0.0f, 0.25f, 0.5f`; rebuilt order
  starts `0.25f, 0.5f, 0.0f, 1.0f`. References are local and behavior-neutral.
- Proof: `python configure.py --non-matching && ninja` linked, then
  `python configure.py && ninja` passed `build/GMSJ01/mario.dol: OK`.
- 9:44am MNL recheck: canonical `audit/System/MarioGamePad.md` was refreshed
  with current full/ranged diffs and a fresh source-link plus normal hash proof.
