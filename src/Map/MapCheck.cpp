#include <Map/MapCollisionData.hpp>
#include <Map/MapCollisionPlane.hpp>
#include <Map/MapData.hpp>
#include <math.h>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

// fabricated
static inline f32 skewProduct(f32 x1, f32 y1, f32 x2, f32 y2)
{
	return x1 * y2 - y1 * x2;
}

// either this or have to use gotos
inline static bool someUnknownInline(TBGCheckData* r31, TBGWallCheckRecord* r29)
{
	f32 radius = r29->mRadius;

	u32 flags = r29->mFlags;

	if ((flags & TBGWallCheckRecord::IGNORE_WATER_THROUGH)
	    && r31->isWaterThrough())
		return false;

	if ((flags & TBGWallCheckRecord::IGNORE_WATER_SURFACE)
	    && r31->isWaterSurface())
		return false;

	f32 cx = r29->mCenter.x;
	f32 cy = r29->mCenter.y;
	f32 cz = r29->mCenter.z;

	f32 nx = r31->getNormal().x;
	f32 ny = r31->getNormal().y;
	f32 nz = r31->getNormal().z;
	f32 d  = r31->getPlaneDistance();

	f32 signedDist = d + (cx * nx + cy * ny + cz * nz);

	if (signedDist < -radius || radius < signedDist)
		return false;

	f32 y1 = r31->getPoint1().y;
	f32 y2 = r31->getPoint2().y;
	f32 y3 = r31->getPoint3().y;

	if (r31->checkFlag(0x8)) {
		if (nx > 0.0f) {
			cz = -cz;

			f32 z1 = -r31->getPoint1().z;
			f32 z2 = -r31->getPoint2().z;
			f32 z3 = -r31->getPoint3().z;

			if (skewProduct(y1 - cy, z1 - cz, y2 - y1, z2 - z1) > 1.0f)
				return false;

			if (skewProduct(y2 - cy, z2 - cz, y3 - y2, z3 - z2) > 1.0f)
				return false;

			if (skewProduct(y3 - cy, z3 - cz, y1 - y3, z1 - z3) > 1.0f)
				return false;
		} else {
			cz = -cz;

			f32 z1 = -r31->getPoint1().z;
			f32 z2 = -r31->getPoint2().z;
			f32 z3 = -r31->getPoint3().z;

			if (skewProduct(y1 - cy, z1 - cz, y2 - y1, z2 - z1) < -1.0f)
				return false;

			if (skewProduct(y2 - cy, z2 - cz, y3 - y2, z3 - z2) < -1.0f)
				return false;

			if (skewProduct(y3 - cy, z3 - cz, y1 - y3, z1 - z3) < -1.0f)
				return false;
		}
	} else {
		if (nz > 0.0f) {
			f32 x1 = r31->getPoint1().x;
			f32 x2 = r31->getPoint2().x;
			f32 x3 = r31->getPoint3().x;

			if (skewProduct(y1 - cy, x1 - cx, y2 - y1, x2 - x1) > 1.0f)
				return false;

			if (skewProduct(y2 - cy, x2 - cx, y3 - y2, x3 - x2) > 1.0f)
				return false;

			if (skewProduct(y3 - cy, x3 - cx, y1 - y3, x1 - x3) > 1.0f)
				return false;
		} else {
			f32 x1 = r31->getPoint1().x;
			f32 x2 = r31->getPoint2().x;
			f32 x3 = r31->getPoint3().x;

			if (skewProduct(y1 - cy, x1 - cx, y2 - y1, x2 - x1) < -1.0f)
				return false;

			if (skewProduct(y2 - cy, x2 - cx, y3 - y2, x3 - x2) < -1.0f)
				return false;

			if (skewProduct(y3 - cy, x3 - cx, y1 - y3, x1 - x3) < -1.0f)
				return false;
		}
	}

	if (!(flags & TBGWallCheckRecord::DONT_MOVE_XZ)) {
		f32 d = radius - signedDist;
		r29->mCenter.x += d * r31->getNormal().x;
		r29->mCenter.z += d * r31->getNormal().z;
	}

	return true;
}

int TMapCollisionData::checkWallList(const TBGCheckList* param_1,
                                     TBGWallCheckRecord* param_2)
{
	if (!param_1)
		return 0;

	f32 f27 = param_2->mCenter.y;
	// param_1: r28
	// param_2: r29

	int r30 = 0;
	while (param_1) {
		TBGCheckData* r31 = param_1->unk8;
		param_1           = param_1->mNext;

		if (r31->mMinY > f27)
			continue;

		if (r31->mMaxY < f27)
			return r30;

		if (!someUnknownInline(r31, param_2))
			continue;

		++r30;
		param_2->mResultWalls[param_2->mResultWallsNum] = r31;
		++param_2->mResultWallsNum;
		if (param_2->mResultWallsNum >= param_2->mMaxResults)
			return r30;
	}

	return r30;
}

int TMapCollisionData::checkWalls(TBGWallCheckRecord* param_1) const
{
	param_1->mResultWallsNum = 0;
	if (param_1->mCenter.x < -mGridExtentX || mGridExtentX <= param_1->mCenter.x
	    || param_1->mCenter.z < -mGridExtentY
	    || mGridExtentY <= param_1->mCenter.z)
		return 0;

	int gridX = (param_1->mCenter.x + mGridExtentX) * (1.0f / 1024);
	int gridZ = (param_1->mCenter.z + mGridExtentY) * (1.0f / 1024);

	int iVar6
	    = checkWallList(getGridRoot18(gridX, gridZ).getWallList(), param_1);

	if (iVar6 >= param_1->mMaxResults)
		return iVar6;

	iVar6 += checkWallList(getGridRoot14(gridX, gridZ).getWallList(), param_1);
	return iVar6;
}

f32 TMapCollisionData::checkRoofList(f32 x, f32 y, f32 z, u8 param_4,
                                     const TBGCheckList* head,
                                     const TBGCheckData** result)
{
	while (head) {
		const TBGCheckData* data = head->unk8;
		head                     = head->mNext;

		if (param_4 & 0x4 && data->isWaterThrough())
			continue;

		if ((data->mPoint1.z - z) * (data->mPoint2.x - data->mPoint1.x)
		        - (data->mPoint1.x - x) * (data->mPoint2.z - data->mPoint1.z)
		    > 1.0f)
			continue;

		if ((data->mPoint2.z - z) * (data->mPoint3.x - data->mPoint2.x)
		        - (data->mPoint2.x - x) * (data->mPoint3.z - data->mPoint2.z)
		    > 1.0f)
			continue;

		if ((data->mPoint3.z - z) * (data->mPoint1.x - data->mPoint3.x)
		        - (data->mPoint3.x - x) * (data->mPoint1.z - data->mPoint3.z)
		    > 1.0f)
			continue;

		f32 tmp = (x * data->mNormal.x) + (z * data->mNormal.z)
		          + data->mPlaneDistance;
		f32 dVar10 = -tmp / data->mNormal.y;

		if (!(y - (dVar10 - -78.0f) > 0.0f)) {
			*result = data;
			return dVar10;
		}
	}

	*result = &mIllegalCheckData;
	return 9999999.0f;
}

f32 TMapCollisionData::checkRoof(f32 x, f32 y, f32 z, u8 flags,
                                 const TBGCheckData** result) const
{
	if (x < -mGridExtentX || mGridExtentX <= x || z < -mGridExtentY
	    || mGridExtentY <= z) {
		*result = &mIllegalCheckData;
		return 9999999.0f;
	}

	int gridX = (x + mGridExtentX) * (1.0f / 1024);
	int gridZ = (z + mGridExtentY) * (1.0f / 1024);

	const TBGCheckData* local_4c;
	f32 dVar5 = checkRoofList(
	    x, y, z, flags, getGridRoot18(gridX, gridZ).getRoofList(), &local_4c);

	const TBGCheckData* local_50;
	f32 dVar6 = checkRoofList(
	    x, y, z, flags, getGridRoot14(gridX, gridZ).getRoofList(), &local_50);

	if (dVar5 < dVar6) {
		*result = local_4c;
		return dVar5;
	} else {
		*result = local_50;
		return dVar6;
	}
}

f32 TMapCollisionData::checkGroundList(f32 x, f32 y, f32 z, u8 flags,
                                       const TBGCheckList* head,
                                       const TBGCheckData** result)
{
	while (head) {
		const TBGCheckData* data = head->unk8;
		head                     = head->getNext();

		if (data->mMinY > y)
			continue;

		if ((flags & IGNORE_WATER_THROUGH) && data->isWaterThrough())
			continue;

		if ((flags & IGNORE_WATER_SURFACE) && data->isWaterSurface())
			continue;

		if ((data->mPoint1.z - z) * (data->mPoint2.x - data->mPoint1.x)
		        - (data->mPoint1.x - x) * (data->mPoint2.z - data->mPoint1.z)
		    < -1.0f)
			continue;

		if ((data->mPoint2.z - z) * (data->mPoint3.x - data->mPoint2.x)
		        - (data->mPoint2.x - x) * (data->mPoint3.z - data->mPoint2.z)
		    < -1.0f)
			continue;

		if ((data->mPoint3.z - z) * (data->mPoint1.x - data->mPoint3.x)
		        - (data->mPoint3.x - x) * (data->mPoint1.z - data->mPoint3.z)
		    < -1.0f)
			continue;

		f32 tmp
		    = x * data->mNormal.x + z * data->mNormal.z + data->mPlaneDistance;
		f32 dVar10 = -tmp / data->mNormal.y;

		if (!(y - (dVar10 + -78.0f) < 0.0f)) {
			*result = data;
			return dVar10;
		}
	}

	*result = &mIllegalCheckData;
	return 9999999.0f;
}

f32 TMapCollisionData::checkGround(f32 x, f32 y, f32 z, u8 flags,
                                   const TBGCheckData** result) const
{
	if (x < -mGridExtentX || mGridExtentX <= x || z < -mGridExtentY
	    || mGridExtentY <= z) {
		*result = &mIllegalCheckData;
		return -32767.0f;
	}

	int gridX = (x + mGridExtentX) * (1.0f / 1024);
	int gridZ = (z + mGridExtentY) * (1.0f / 1024);

	const TBGCheckData* local_60;
	f32 dVar5 = checkGroundList(
	    x, y, z, flags, getGridRoot18(gridX, gridZ).getGroundList(), &local_60);

	const TBGCheckData* local_64;
	f32 dVar6 = checkGroundList(
	    x, y, z, flags, getGridRoot14(gridX, gridZ).getGroundList(), &local_64);

	if (mGroundPlane != nullptr) {
		const TBGCheckData* local_68;
		f32 dVar7 = mGroundPlane->checkPlaneGround(x, y, z, &local_68);
		if (dVar7 > dVar6) {
			local_64 = local_68;
			dVar6    = dVar7;
		}
	}

	if (dVar5 > dVar6) {
		*result = local_60;
		return dVar5;
	} else {
		*result = local_64;
		return dVar6;
	}
}

inline static f32 crossXZ(const JGeometry::TVec2<f32>& a,
                          const JGeometry::TVec2<f32>& b)
{
	return a.x * b.y - a.y * b.x;
}

inline static bool LineInLineXZ(const JGeometry::TVec2<f32>& a1,
                                const JGeometry::TVec2<f32>& a2,
                                const JGeometry::TVec2<f32>& b1,
                                const JGeometry::TVec2<f32>& b2)
{
	JGeometry::TVec2<f32> a2a1(a2);
	JGeometry::TVec2<f32> b1a1(b1);
	a2a1.sub(a1);
	b1a1.sub(a1);

	JGeometry::TVec2<f32> b2a1(b2);
	b2a1.sub(a1);

	f32 c1 = crossXZ(a2a1, b1a1);
	f32 c2 = crossXZ(a2a1, b2a1);
	bool result = false;
	if (c1 * c2 <= 0.0f) {
		JGeometry::TVec2<f32> b2b1(b2);
		JGeometry::TVec2<f32> a1b1(a1);
		b2b1.sub(b1);
		a1b1.sub(b1);

		JGeometry::TVec2<f32> a2b1(a2);
		a2b1.sub(b1);

		f32 c3 = crossXZ(b2b1, a1b1);
		f32 c4 = crossXZ(b2b1, a2b1);
		if (c3 * c4 <= 0.0f)
			result = true;
	}
	return result;
}

inline static f32 calcEdgeAngle(f32 ax, f32 ay, f32 az, f32 bx, f32 by, f32 bz)
{
	f32 cross_x  = ay * bz - az * by;
	f32 cross_y  = az * bx - ax * bz;
	f32 cross_z  = ax * by - ay * bx;
	f32 cross_sq = cross_x * cross_x + cross_y * cross_y + cross_z * cross_z;
	f32 cross_len = JGeometry::TUtil<f32>::sqrt(cross_sq);
	f32 dot       = ax * bx + ay * by + az * bz;
	return fabsf(atan2f(cross_len, dot));
}

static bool bgIntersectLine(const TBGCheckData* data,
                            const JGeometry::TVec3<f32>& start,
                            const JGeometry::TVec3<f32>& end,
                            bool ignore_back_faces,
                            JGeometry::TVec3<f32>* hit_point)
{
	if (data == nullptr)
		return false;

	if (data->isMarioThrough())
		return false;

	JGeometry::TVec3<f32> normal(data->mNormal);
	JGeometry::TVec3<f32> dir(end);
	dir.sub(start);

	f32 denom = normal.dot(dir);
	if (ignore_back_faces && denom >= 0.0f)
		return false;

	if (fabsf(denom) < 0.00001f)
		return false;

	f32 t = -(data->mPlaneDistance + normal.dot(start)) / denom;
	if (t < 0.0f)
		return false;
	if (1.0f < t)
		return false;

	JGeometry::TVec3<f32> scaled(dir);
	scaled.scale(t);
	JGeometry::TVec3<f32> hit = start + scaled;

	f32 point1x = data->mPoint1.x - hit.x;
	f32 point1y = data->mPoint1.y - hit.y;
	f32 point1z = data->mPoint1.z - hit.z;
	f32 point2x = data->mPoint2.x - hit.x;
	f32 point2y = data->mPoint2.y - hit.y;
	f32 point2z = data->mPoint2.z - hit.z;
	f32 point3x = data->mPoint3.x - hit.x;
	f32 point3y = data->mPoint3.y - hit.y;
	f32 point3z = data->mPoint3.z - hit.z;

	f32 angle_sum = 0.0f;
	angle_sum += calcEdgeAngle(point1x, point1y, point1z, point2x, point2y,
	                           point2z);
	angle_sum += calcEdgeAngle(point2x, point2y, point2z, point3x, point3y,
	                           point3z);
	angle_sum += calcEdgeAngle(point3x, point3y, point3z, point1x, point1y,
	                           point1z);

	if (fabsf(6.2831855f - angle_sum) > 0.001f)
		return false;

	if (hit_point != nullptr)
		*hit_point = hit;

	return true;
}

const TBGCheckData*
TMapCollisionData::intersectLine(const JGeometry::TVec3<f32>& start,
                                 const JGeometry::TVec3<f32>& end,
                                 bool ignore_back_faces,
                                 JGeometry::TVec3<f32>* hit_point) const
{
	f32 startX = start.x;
	f32 endX   = end.x;
	f32 startZ = start.z;
	f32 endZ   = end.z;

	s32 min_x = (s32)startX;
	s32 max_x = (s32)endX;
	if (min_x > max_x) {
		s32 tmp = min_x;
		min_x   = max_x;
		max_x   = tmp;
	}

	s32 min_z = (s32)startZ;
	s32 max_z = (s32)endZ;
	if (min_z > max_z) {
		s32 tmp = min_z;
		min_z   = max_z;
		max_z   = tmp;
	}

	s32 min_grid_x = (s32)((min_x + mGridExtentX) * (1.0f / 1024.0f));
	s32 max_grid_x = (s32)((max_x + mGridExtentX) * (1.0f / 1024.0f));
	s32 min_grid_z = (s32)((min_z + mGridExtentY) * (1.0f / 1024.0f));
	s32 max_grid_z = (s32)((max_z + mGridExtentY) * (1.0f / 1024.0f));

	JGeometry::TVec2<f32> line_a(startX, startZ);
	JGeometry::TVec2<f32> line_b(endX, endZ);

	for (s32 z = min_grid_z; z <= max_grid_z; ++z) {
		for (s32 x = min_grid_x; x <= max_grid_x; ++x) {
			if (x != min_grid_x || z != min_grid_z) {
				f32 left   = x * 1024.0f - mGridExtentX;
				f32 right  = (x + 1) * 1024.0f - mGridExtentX;
				f32 bottom = z * 1024.0f - mGridExtentY;
				f32 top    = (z + 1) * 1024.0f - mGridExtentY;

				JGeometry::TVec2<f32> p1(left, bottom);
				JGeometry::TVec2<f32> p2(right, bottom);
				JGeometry::TVec2<f32> p3(left, top);
				JGeometry::TVec2<f32> p4(right, top);

				if (!LineInLineXZ(line_a, line_b, p1, p2)
				    && !LineInLineXZ(line_a, line_b, p1, p3)
				    && !LineInLineXZ(line_a, line_b, p3, p4)
				    && !LineInLineXZ(line_a, line_b, p2, p4))
					continue;
			}

			const TBGCheckListRoot& root = getGridRoot14(x, z);
			const TBGCheckList* node     = root.unk0[0].getNext();
			while (node != nullptr) {
				const TBGCheckData* data = node->unk8;
				node                     = node->getNext();
				if (bgIntersectLine(data, start, end, ignore_back_faces,
				                    hit_point))
					return data;
			}

			node = root.unk0[2].getNext();
			while (node != nullptr) {
				const TBGCheckData* data = node->unk8;
				node                     = node->getNext();
				if (bgIntersectLine(data, start, end, ignore_back_faces,
				                    hit_point))
					return data;
			}

			node = root.unk0[1].getNext();
			while (node != nullptr) {
				const TBGCheckData* data = node->unk8;
				node                     = node->getNext();
				if (bgIntersectLine(data, start, end, ignore_back_faces,
				                    hit_point))
					return data;
			}
		}
	}

	return nullptr;
}
