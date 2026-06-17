# JSystem/JAudio/JASystem/JASSeqParser.cpp

Verdict: equivalent
Time: 2026-06-12 9:58pm MNL

Build proof:
- `python configure.py --non-matching && ninja`
- `python configure.py && ninja`

No missing symbols.

Data/object proof:
- `JASystem::Arglist` is byte-matching.
- `JASystem::TSeqParser::sCmdPList` is byte-matching.
- `.data`, `.rodata`, and `.sdata2` objects are byte-matching in the overview.

Reviewed nonmatching functions:
- `JASystem::TSeqParser::cmdPrintf(TTrack*, unsigned long*)`
  - Equivalent. Reads the format string, handles escape/newline conversion, classifies `%d/%x/%s/%r/%R/%t`, reads/register-resolves arguments, and returns zero.
  - Residue is stack-frame/local-array placement.
- `JASystem::TSeqParser::cmdNoteOff(TTrack*, unsigned char)`
  - Equivalent. Handles the `0xf9` extended note form, register exchange, optional release-time byte, release scaling for values above `0x64`, and calls `noteOff`.
  - Residue is predicate expression shape and register allocation.
- `JASystem::TSeqParser::cmdNoteOn(TTrack*, unsigned char)`
  - Equivalent. Parses key/velocity/gate encodings, register-backed operands, note length, time conversion, legato/sweep flags, `gateOn`/`noteOn`, note state updates, and key sweep target.
  - Residue is stack byte placement and GPR allocation.
- `JASystem::TSeqParser::mainProc(TTrack*, TSeqCtrl*)`
  - Equivalent. Reads sequence bytes, handles condition checks, delay commands, note-on/off dispatch, time/register parameter writes, regular command processing, and return-code handling.
  - Residue is saved-register allocation and helper-label owner drift (`cmdNoteOff`/`cmdWait` labels on the same branch target).
- `__sinit_JASSeqParser_cpp`
  - Equivalent. The initialized `Arglist` and command pointer-to-member table data are byte-matching; residue is the copy strategy and null pointer-to-member source slots used to populate `sCmdPList`.

The lone extra text symbol is the source-emitted `cmdWait(TTrack*, unsigned char)` owner variant; the actual global `cmdWait(TTrack*, unsigned long*)` exists and matches.

2026-06-13 9:24am MNL recheck: verdict remains `equivalent`. Full diffs for
`cmdPrintf`, `cmdNoteOff`, `cmdNoteOn`, `mainProc`, and
`__sinit_JASSeqParser_cpp` were re-read. The command handlers keep the same
format parsing, note on/off operand decoding, register exchange, time
conversion, gate/note dispatch, state stores, and return-code behavior. Raw
relocations confirm the noisy `mainProc` call label is source owner drift, not a
wrong dispatch. `Arglist`, `sCmdPList`, and aggregate data are still
byte-matching in the overview; the static init drift is copy strategy/null
pointer-to-member source-slot layout. Shared proof passed:
`python configure.py --non-matching && ninja`, then `python configure.py &&
ninja` verified `mario.dol: OK`.

2026-06-13 1:21pm MNL recheck: verdict remains `equivalent`. Current overview
still has no missing rows and `Arglist`, `sCmdPList`, `.data`, `.rodata`, and
`.sdata2` are byte-matching. Full diffs for the five nonmatching functions show
the same format-string parser, note-off extended flag/release handling, note-on
operand/time/tie/sweep dispatch, main command dispatch/return-code loop, and
static command table values. Residue is stack-array placement, saved-register
coloring, the source-owned `cmdWait(TTrack*, u8)` helper label, and `__sinit`
copy-source choices among identical pointer-to-member/null slots. Proof from
this tick passed with `python configure.py --non-matching && ninja`, then
normal `python configure.py && ninja` verified `build/GMSJ01/mario.dol: OK`.
