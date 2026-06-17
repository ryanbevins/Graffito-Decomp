# mario/GC2D/ProgSelect

Verdict: equivalent  
Date: 2026-06-13 11:59am MNL

`GC2D/ProgSelect.cpp` is functionally identical and links from source.

- `TProgSelect::TProgSelect(...)` and the destructor are byte-identical.
- `TProgSelect::perform(...)` preserves the same pulsing timer updates,
  selection input handling, timeout counter math, progressive/interlace message
  setup, `OSSetProgressiveMode` calls, text color/alpha updates, and draw path.
- The inline `thing()` helper is marked fabricated in the header, but its
  emitted behavior is visible in `perform`: increment `unk128`, divide by
  `mRefreshRate`, and compare against `10.0f`.
- The explicit `char trahs[0x10]` frame helper and remaining diffs are
  stack/local-slot/data-owner byte debt, not behavioral differences.
- Proof: temporary `Object(Equivalent, "GC2D/ProgSelect.cpp")` promotion passed
  `python configure.py --non-matching && ninja`.
