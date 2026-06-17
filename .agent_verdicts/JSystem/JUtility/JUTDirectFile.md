## mario/JSystem/JUtility/JUTDirectFile

Verdict: equivalent  
Date: 2026-06-13 12:06pm MNL

`JSystem/JUtility/JUTDirectFile.cpp` is functionally identical and links from
source.

- `JUTDirectFile::fgets(void*, int)` performs the same open/length/null guards,
  refill calculation, `DVDReadAsyncPrio`/status wait, buffer-copy loop,
  newline termination, position advance, and return-count behavior.
- The rebuilt TU emits an extra standalone `JUTDirectFile::fetch32byte()` body,
  while the target has the refill logic inlined into `fgets`. This is helper
  ownership/byte debt; the source-linked object has no undefined reference and
  the callable path used by the game is represented.
- Proof: temporary `Object(Equivalent, "JSystem/JUtility/JUTDirectFile.cpp")`
  promotion passed `python configure.py --non-matching && ninja`.
