# Player/SplashManager audit

Verdict: `equivalent`

Checked: 2026-06-14 6:31pm MNL in AUDIT safety-net mode.

## Result

Reverified existing `Object(Equivalent, "Player/SplashManager.cpp")`.

Current nonmatching functions remain behavior-equivalent:

- `TSplashManager::load(JSUMemoryInputStream&)` preserves resource lookup,
  texture allocation/setup, manager constants, gravity computation, color
  assignment, pool/list initialization, quad buffer creation, init life, and
  `gpSplashManager = this`. Residue is operand order, color-store lowering,
  local-label, and source-owned helper/data debt.
- `TSplashManager::makeDL(JDrama::TGraphics*) const` preserves active-list
  traversal, `PSMTXMultVec`, depth culling/life clear, alpha and size ratio
  math, quad vertex construction, `requestCol`, and final `setEnd`. Residue is
  frame/register/FPR and stack-slot layout.

No missing target symbols remain. The old source-link blocker was undefined
`gpSplashManager`; current source owns it in this TU.

## Verification

- `python tools/decomp-diff.py -u mario/Player/SplashManager -s missing`
  reports no symbols.
- This tick's `python configure.py --non-matching && ninja` linked all current
  `Equivalent` objects successfully.
- This tick's `python configure.py && ninja` passed with
  `build/GMSJ01/mario.dol: OK`.
