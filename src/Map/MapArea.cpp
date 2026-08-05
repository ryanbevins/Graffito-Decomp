#include <Map/MapCollisionData.hpp>
#include <Map/MapData.hpp>

static bool checkLinesCollision(f32 ax, f32 ay, f32 bx, f32 by, f32 cx, f32 cy,
                                f32 dx, f32 dy)
{
	f32 dabx    = bx - ax;
	f32 daby    = by - ay;
	f32 cross_c = daby * (cx - bx) - dabx * (cy - by);
	f32 cross_d = daby * (dx - bx) - dabx * (dy - by);
	if ((cross_c >= 0.0f && cross_d >= 0.0f)
	    || (cross_c < 0.0f && cross_d < 0.0f))
		return false;

	f32 dcdx    = dx - cx;
	f32 dcdy    = dy - cy;
	f32 cross_a = dcdy * (ax - dx) - dcdx * (ay - dy);
	f32 cross_b = dcdy * (bx - dx) - dcdx * (by - dy);
	if ((cross_a >= 0.0f && cross_b >= 0.0f)
	    || (cross_a < 0.0f && cross_b < 0.0f))
		return false;

	return true;
}

static inline bool checkLinePolygonCollision(f32 x1, f32 z1, f32 x2, f32 z2,
                                             TBGCheckData* poly)
{
	const JGeometry::TVec3<f32>& p2 = poly->mPoint2;
	const JGeometry::TVec3<f32>& p1 = poly->mPoint1;

	if (!checkLinesCollision(x1, z1, x2, z2, p1.x, p1.z, p2.x, p2.z)) {
		const JGeometry::TVec3<f32>& p3 = poly->mPoint3;
		if (!checkLinesCollision(x1, z1, x2, z2, p2.x, p2.z, p3.x,
		                         p3.z)
		    && !checkLinesCollision(x1, z1, x2, z2, p3.x, p3.z, p1.x,
		                            p1.z))
			return false;
	}

	return true;
}

static inline bool pointIsInPolygon(f32 px, f32 pz, TBGCheckData* poly)
{
	f32 p1x = poly->mPoint1.x;
	f32 p1z = poly->mPoint1.z;
	f32 p2x = poly->mPoint2.x;
	f32 p2z = poly->mPoint2.z;
	f32 p3x = poly->mPoint3.x;
	f32 p3z = poly->mPoint3.z;
	if ((p1z - pz) * (p2x - p1x) - (p1x - px) * (p2z - p1z) < 0.0f)
		return false;
	if ((p2z - pz) * (p3x - p2x) - (p2x - px) * (p3z - p2z) < 0.0f)
		return false;
	if ((p3z - pz) * (p1x - p3x) - (p3x - px) * (p1z - p3z) < 0.0f)
		return false;
	return true;
}

static inline bool pointIsInGrid(f32 px, f32 pz, f32 a, f32 b, f32 c, f32 d)
{
	if (a <= px && px <= c && b <= pz && pz <= d)
		return true;
	return false;
}

bool TMapCollisionData::polygonIsInGrid(f32 a, f32 b, f32 c, f32 d,
                                       TBGCheckData* poly)
{
	if (poly->mNormal.y < 0.0f)
		return true;

	if (pointIsInGrid(poly->mPoint1.x, poly->mPoint1.z, a, b, c, d)
	    || pointIsInGrid(poly->mPoint2.x, poly->mPoint2.z, a, b, c, d)
	    || pointIsInGrid(poly->mPoint3.x, poly->mPoint3.z, a, b, c, d))
		return true;

	if (pointIsInPolygon(a, b, poly) || pointIsInPolygon(c, b, poly)
	    || pointIsInPolygon(a, d, poly) || pointIsInPolygon(c, d, poly))
		return true;

	if (checkLinePolygonCollision(a, b, c, b, poly))
		return true;

	if (checkLinePolygonCollision(a, d, c, d, poly))
		return true;

	if (checkLinePolygonCollision(a, b, a, d, poly))
		return true;

	if (checkLinePolygonCollision(c, b, c, d, poly))
		return true;

	return false;
}
