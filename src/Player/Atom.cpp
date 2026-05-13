#include <Map/MapCollisionEntry.hpp>
#include <JSystem/JGeometry.hpp>

static JGeometry::TVec3<f32> cDeformedTerrainCenter(0.0f, 5000.0f, 0.0f);

void TMapCollisionBase::setUp() { unk5C &= ~1; }
