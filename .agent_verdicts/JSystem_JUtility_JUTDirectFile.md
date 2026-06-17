# Audit verdict: equivalent

- Date: 2026-06-13 11:30pm MNL
- TU: `mario/JSystem/JUtility/JUTDirectFile`
- Source: `src/JSystem/JUtility/JUTDirectFile.cpp`
- Verdict: `equivalent`

## Reason

Current overview has no missing target symbols. `JUTDirectFile::JUTDirectFile`,
`~JUTDirectFile`, `fopen`, and `fclose` match exactly. The only nonmatching
target text is `JUTDirectFile::fgets(void*, int)` at 98.8%, plus a rebuilt-only
standalone `JUTDirectFile::fetch32byte()` helper.

`fgets` preserves the same behavior:
- identical early returns for closed file, zero/one length, null buffer, and EOF;
- identical inlined fetch path when `mToRead == 0`: compute remaining bytes
  from aligned `mPos`, clamp to `0x800`, bracket `DVDReadAsyncPrio` with
  interrupt enable/restore, wait on `DVDGetCommandBlockStatus`, and map failed
  reads to `-1`;
- identical chunk-size clamp to `len - readCount - 1`;
- identical byte copy loop, newline break, null terminator stores, `mToRead`
  reset at buffer boundary, `mPos`/read-count updates, and final EOF terminator.

The extra `fetch32byte()` body is byte debt only: the target inlines the same
body into `fgets`, while the source also emits the standalone helper.

## Proof

- This tick's `python configure.py --non-matching && ninja` source-linked the
  current `Equivalent` set successfully.
- `python configure.py && ninja` restored the normal matching config and passed
  with `build/GMSJ01/mario.dol: OK`.
