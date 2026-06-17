# mario/GC2D/MovieSubtitle

Verdict: equivalent
Date: 2026-06-13 10:31am MNL

Reason: target-visible functions are exact except
`TMovieSubTitle::setupResource`, which has the same movie-height table scan,
`J2DSetScreen` allocation/selection, hide call, text-box lookups, blank buffer
initialization, message-loader allocation, BMG path construction, message load,
and index reset. The residue is an 8-byte stack-frame/slot offset. Extra
methods (`movement`, `draw`, `show`, `hide`, `getCurEntry`, `makeBmgName`) are
target-absent out-of-line copies of logic inlined into the matching visible
functions.

Proof:
- `python tools/decomp-diff.py -u mario/GC2D/MovieSubtitle -t function`
- `python tools/decomp-diff.py -u mario/GC2D/MovieSubtitle -d "setupResource" --no-collapse`
- `python tools/decomp-diff.py -u mario/GC2D/MovieSubtitle -t object`
- `python configure.py --non-matching && ninja`
- `python configure.py && ninja`

Reverified in the current audit sweep. The current `setupResource` diff still
preserves the long-height movie lookup, screen allocation choice, hide/search
sequence, blank text-buffer setup, message-loader allocation, `.bmg` path
construction, message-data load, and index reset. Residue remains the 8-byte
frame/stack-buffer offset and extra out-of-line helper ownership.

2026-06-13 10:31am MNL recheck: overview/full diff unchanged; reused the
current proof batch: `python configure.py --non-matching && ninja`, then
normal `python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
