
#include <Map/MapCollisionEntry.hpp>
#include <JSystem/JGeometry/JGVec3.hpp>

// NOTE: file with debug stuff?

static JGeometry::TVec3<f32> cDeformedTerrainCenter(0.0f, 5000.0f, 0.0f);

inline void TMapCollisionBase::setUp() { offFlag(1); }

void dummy(TMapCollisionBase* col) { col->setUp(); }
