# Map/PollutionLayer Audit

## 2026-06-14 4:43am MNL - equivalent

Verdict: `equivalent`.

`mario/Map/PollutionLayer` is behavior-equivalent and source-links under
`--non-matching`.

Fix made during this audit:
- `TPollutionLayer::initTexImage(const char*)` now follows the target clear
  condition. For map 9 only, boundary texels are cleared. All maps clear
  zero-depth and prohibited texels. Otherwise the target stores depth, or
  `depth - edgeDegree * TPollutionManager::mEdgeAlpha` when
  `getEdgeDegree(x, y) != 0`.
- The previous source had the logic inverted: non-map-9 layers fell into the
  zero-fill path, and map-9 prohibited/zero-depth cases took the depth-copy
  path.

Review summary:
- `initTexImage` is behavior-aligned after the fix. Fuzzy score dropped
  `83.2% -> 78.6%` because the current source materializes a `shouldClear`
  bool, but the branch conditions and stores now match the target behavior.
- Existing nonmatching functions (`stamp`, wall stamps, `cleaned`,
  `isPolluted`, `initGX`, `draw`, and joint-model init paths) were rechecked at
  audit level as codegen-class or label/temporary drift: stack/FPR/GPR layout,
  equivalent arithmetic ordering, helper-boundary ownership, static local
  labels, and sdata2 constant-label drift.
- No target symbols are missing. `.rodata`, `.data`, and `.sdata` are exact;
  remaining `.sdata2` drift is byte debt.

Proof:
```bash
python configure.py --non-matching && ninja
python configure.py && ninja
```

The `--non-matching` build linked `Map/PollutionLayer.cpp` from source, and the
normal matching build ended with `build/GMSJ01/mario.dol: OK`.
