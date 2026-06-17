# Strategic/question audit

Verdict: equivalent  
Date: 2026-06-13 6:45am MNL
Status: reverified current source-link proof

Proof: `python configure.py --non-matching && ninja` links cleanly with
`Strategic/question.cpp` built from source. All target `.text` functions,
`TQuestionManager` vtable, and `gpQuestionManager` are present. A follow-up
plain `python configure.py && ninja` restored the matching configuration and
verified `build/GMSJ01/mario.dol: OK`.

Reason: Non-100%
function diffs are codegen-class stack-frame/slot and register/FPR-coloring
residue.

Reviewed functions:
- `TQuestionManager::perform(unsigned long, JDrama::TGraphics*)`: same
  silhouette gates, quad reset, `makeDL`, flag updates, counter reset, and
  draw gate; residue is stack frame/slot size.
- `TQuestionManager::makeDL(JDrama::TGraphics*) const`: same request loop,
  view-matrix transform, quad vertex construction, `TDLTexQuad::request`, and
  `setEnd`; residue is stack placement for vector temporaries.
- `TQuestionManager::request(JGeometry::TVec3<float>, float)`: same request cap,
  Mario X/Z distance test, request copy, radius store, increment, and bool
  return; residue is FPR coloring for X/Z delta evaluation.

Notes: source emits extra unused weak `TDLTexQuad::reset`,
`JDrama::TViewObj::~TViewObj`, and `JDrama::TViewObj` vtable ownership. The
source-link build succeeds and behavior-visible symbols are present.

2026-06-13 10:45am MNL recheck: verdict remains `equivalent`. Re-read the
current diffs for `perform`, `makeDL`, and `request`. `perform` keeps the same
silhouette gates, quad reset, make-DL call, flag/counter updates, and draw gate.
`makeDL` keeps the same request loop, view transform, quad vertex construction,
`TDLTexQuad::request`, and `setEnd`. `request` keeps the same cap guard,
Mario X/Z distance test, request vector copy, radius store, count increment,
and bool return. Residue is stack/vector temp placement and X/Z FPR coloring.
Proof refreshed with `python configure.py --non-matching && ninja`, then normal
`python configure.py && ninja` with `build/GMSJ01/mario.dol: OK`.
