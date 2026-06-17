verdict: needs_impl
date: 2026-06-13 3:46am MNL
tu: mario/MarioUtil/ShadowUtil
source: src/MarioUtil/ShadowUtil.cpp

Reason:
- Not functionally equivalent yet. Overview still has missing target text
  symbols for the local display-list setup classes
  `TSetup1$2172ShadowUtil_cpp` through `TSetup5$2216ShadowUtil_cpp`,
  `TCylinder$2171ShadowUtil_cpp`, their destructors, and their `makeDL()`
  bodies.
- `GDOverflowCheck` is missing.
- `TBGCheckData::isWaterSurface() const` is missing as a target-owned weak
  text symbol.
- Notes state `TMBindShadowManager::drawShadowGD()` is currently a functional
  fallback that calls the immediate `drawShadow()` path, while the original
  uses GD display-list helper classes. That is a real implementation gap, not
  codegen-only drift.

Offending functions/symbols:
- `TMBindShadowManager::drawShadowGD(unsigned long, JDrama::TGraphics*)`
- `TMBindShadowManager::TSetup1$2172ShadowUtil_cpp::makeDL()`
- `TMBindShadowManager::TCylinder$2171ShadowUtil_cpp::makeDL()`
- `TMBindShadowManager::TSetup2$2190ShadowUtil_cpp::makeDL()`
- `TMBindShadowManager::TSetup3$2195ShadowUtil_cpp::makeDL()`
- `TMBindShadowManager::TSetup4$2207ShadowUtil_cpp::makeDL()`
- `TMBindShadowManager::TSetup5$2216ShadowUtil_cpp::makeDL()`
- `GDOverflowCheck`
- `TBGCheckData::isWaterSurface() const`
