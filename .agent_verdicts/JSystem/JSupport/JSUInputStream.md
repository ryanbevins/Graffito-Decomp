Verdict: equivalent

Date: 2026-06-13 9:44am MNL
Unit: `mario/JSystem/JSupport/JSUInputStream`
Source: `src/JSystem/JSupport/JSUInputStream.cpp`
Classification: `Object(Equivalent, "JSystem/JSupport/JSUInputStream.cpp")`

Reason:
- Re-verified during the audit sweep. The overview has no missing target
  symbols. Three stream functions and all `JSURandomInputStream` methods match
  byte-for-byte; only `JSUInputStream::readString(char*, unsigned short)` and a
  data ownership row remain nonmatching.
- `readString(char*, unsigned short)` preserves the target behavior: it reads
  the two-byte length, sets the stream EOF/error flag and writes a null byte on
  short length-read, reads either the full string or `capacity - 1` bytes,
  null-terminates the destination, skips the unread tail for truncated buffers,
  compares consumed bytes against the encoded length, and sets the same status
  flag on mismatch.
- The remaining text diff is codegen-class saved-register/prologue scheduling.
  Extra `JSUIosBase` destructor/vtable rows are weak owner drift; those symbols
  are target-present in `JSUOutputStream` and the source-link build is clean.
- 9:44am MNL recheck: fresh `--no-collapse` diff for
  `readString(char*, unsigned short)` still shows the same length-read,
  truncate/read/skip, null-termination, consumed-length compare, and EOF flag
  stores. All differences are saved-register/prologue scheduling and stack
  slot names.
- 5:48am MNL safety-net recheck: current `--no-collapse` diff is unchanged in
  behavior. The only text drift is saved-register assignment/prologue order;
  all stream reads, terminators, skip amount, return value, and EOF flag stores
  remain equivalent.
- 5:58pm MNL safety-net recheck: current `--no-collapse` diff for
  `readString(char*, unsigned short)` is still behavior-identical. The
  remaining drift is saved-register assignment/prologue scheduling only; all
  read lengths, truncation, skip amount, null terminators, return value, and EOF
  flag stores match target semantics.

Proof:
- `python configure.py --non-matching && ninja` linked
  `build/GMSJ01/mario.dol` from source.
- `python configure.py && ninja` restored the matching config and passed
  `build/GMSJ01/mario.dol: OK`.

Offending functions: none.
