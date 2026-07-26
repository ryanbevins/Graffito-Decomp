#ifndef MAP_MAP_COLLISION_ENTRY_INLINE_HPP
#define MAP_MAP_COLLISION_ENTRY_INLINE_HPP

#include <Map/MapCollisionEntry.hpp>

inline void
TMapCollisionBase::setUpTrans(const JGeometry::TVec3<f32>& param_1)
{
	JGeometry::TVec3<f32> vec3;
	JGeometry::TVec3<f32> vec2;
	vec3.set((Vec) { 0.0f, 0.0f, 0.0f });
	vec2.set((Vec) { 1.0f, 1.0f, 1.0f });
	MsMtxSetTRS(unk20, param_1.x, param_1.y, param_1.z, vec2.x, vec2.y, vec2.z,
	            vec3.x, vec3.y, vec3.z);
	setUp();
}

#endif
