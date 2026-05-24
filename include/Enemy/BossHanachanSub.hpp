#ifndef ENEMY_BOSS_HANACHAN_SUB_HPP
#define ENEMY_BOSS_HANACHAN_SUB_HPP

#include <JSystem/JGeometry.hpp>

class TSpherePoint {
public:
	TSpherePoint() { }

	/* 0x00 */ JGeometry::TVec3<f32> mPrev;
	/* 0x0C */ JGeometry::TVec3<f32> mPos;
	/* 0x18 */ JGeometry::TVec3<f32> mVel;
	/* 0x24 */ f32 mSegLen;
	/* 0x28 */ f32 mDegree;
};

class TSphereLink {
public:
	TSphereLink(u16 count, const JGeometry::TVec3<f32>& pos, f32 radius,
	            f32 a, f32 b, f32 c, f32 d, f32 angleDeg);

	void moveHead(const JGeometry::TVec3<f32>& head);
	BOOL setDegreeZAndRevisionPosXZ(int index, f32 newDeg);

	/* 0x00 */ u16 mCount;
	/* 0x04 */ TSpherePoint* mPoints;
	/* 0x08 */ f32 m08;
	/* 0x0C */ f32 mGravityY;
	/* 0x10 */ f32 m10;
	/* 0x14 */ f32 m14;
	/* 0x18 */ f32 mAngleOffset;
};

f32 BHSCalcCentrifugalForce(const JGeometry::TVec3<f32>& a,
                            const JGeometry::TVec3<f32>& b,
                            const JGeometry::TVec3<f32>& c, f32 d);
void BHSCalcRevisionDistXZByRotateZ(f32 angle, f32 a, f32 b, f32* outX,
                                    f32* outZ);

#endif
