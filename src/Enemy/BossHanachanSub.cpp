#define TBGCHECKDATA_ISILLEGAL_OUT_OF_LINE
#include <Enemy/BossHanachanSub.hpp>

#include <Camera/cameralib.hpp>
#include <JSystem/JMath.hpp>
#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Player/ModelWaterManager.hpp>
#include <Strategic/HitActor.hpp>
#include <System/MarDirector.hpp>
#include <dolphin/mtx.h>

f32 BHSCalcCentrifugalForce(const JGeometry::TVec3<f32>& a,
                            const JGeometry::TVec3<f32>& b,
                            const JGeometry::TVec3<f32>& c, f32 d)
{
	f32 abX  = a.x - b.x;
	f32 abZ  = a.z - b.z;
	f32 abSq = abX * abX + abZ * abZ;
	if (abSq < 0.001f)
		return 0.0f;

	f32 bcX  = b.x - c.x;
	f32 bcZ  = b.z - c.z;
	f32 bcSq = bcX * bcX + bcZ * bcZ;
	if (bcSq < 0.001f)
		return 0.0f;

	f32 angAB;
	if (abZ == 0.0f) {
		if (abX >= 0.0f)
			angAB = 90.0f;
		else
			angAB = -90.0f;
	} else if (abZ >= 0.0f) {
		angAB = matan(abZ, abX) * (360.0f / 65536.0f);
	} else {
		f32 angle = matan(-abZ, abX) * (360.0f / 65536.0f);
		angAB    = 180.0f - angle;
	}
	s16 a1 = CLBRoundf<s16>(angAB * (65536.0f / 360.0f));

	f32 angBC;
	if (bcZ == 0.0f) {
		if (bcX >= 0.0f)
			angBC = 90.0f;
		else
			angBC = -90.0f;
	} else if (bcZ >= 0.0f) {
		angBC = matan(bcZ, bcX) * (360.0f / 65536.0f);
	} else {
		f32 angle = matan(-bcZ, bcX) * (360.0f / 65536.0f);
		angBC    = 180.0f - angle;
	}
	s16 a2 = CLBRoundf<s16>(angBC * (65536.0f / 360.0f));

	if (a1 == a2)
		return 0.0f;

	s16 deltaS = a1 - a2;
	s16 absD   = deltaS < 0 ? -deltaS : deltaS;
	f32 ratio  = absD * (1.0f / 32768.0f);
	if (ratio >= 0.5f)
		return 0.0f;

	f32 scaled = abSq * ratio;
	if (deltaS < 0)
		scaled = -scaled;

	s16 dPart      = CLBRoundf<s16>(d * (65536.0f / 360.0f));
	s16 finalAngle = a1 - dPart;
	return scaled * jmaCosTable[(u16)finalAngle >> jmaSinShift];
}

void BHSCalcRevisionDistXZByRotateZ(f32 angleDeg, f32 a, f32 b, f32* outX,
                                    f32* outZ)
{
	f32 c    = b * a;
	s16 ang  = CLBRoundf<s16>(angleDeg * (65536.0f / 360.0f));
	f32 cosV = jmaCosTable[(u16)ang >> jmaSinShift];
	f32 sinV = jmaSinTable[(u16)ang >> jmaSinShift];
	f32 zero = 0.0f;
	*outX    = c * cosV + zero * sinV;
	*outZ    = -c * sinV + zero * cosV;
}

void TWaterHitActor::onWaterHitCounter() { *(u16*)&unk68 = 0x3C; }

BOOL TWaterHitActor::receiveMessage(THitActor* sender, u32 message)
{
	BOOL ret = FALSE;
	if (message == 0xF) {
		if (gpMarDirector->isThing())
			*(u16*)&unk68 = 0;
		else
			onWaterHitCounter();
		ret = TRUE;
	}
	return ret;
}

TSphereLink::TSphereLink(u16 count, const JGeometry::TVec3<f32>& pos,
                         f32 radius, f32 a, f32 b, f32 c, f32 d, f32 angleDeg)
{
	mCount       = count;
	mPoints      = new TSpherePoint[count];
	m08          = b;
	mGravityY    = c;
	m10          = a;
	m14          = d;
	mAngleOffset = angleDeg;

	s32 ang16 = (s32)(mAngleOffset * (65536.0f / 360.0f));
	f32 sinV  = jmaSinTable[(u16)ang16 >> jmaSinShift];
	f32 cosV  = jmaCosTable[(u16)ang16 >> jmaSinShift];
	f32 dx    = sinV * radius;
	f32 dz    = cosV * radius;

	for (int i = 0; i < mCount; i++) {
		TSpherePoint& sp = mPoints[i];
		JGeometry::TVec3<f32> p;
		if (i == 0) {
			p = pos;
		} else {
			p = mPoints[i - 1].mPos;
			p.x -= dx;
			p.z -= dz;
		}
		sp.mPos          = p;
		sp.mPrev         = sp.mPos;
		sp.mVel.x        = 0.0f;
		sp.mVel.y        = 0.0f;
		sp.mVel.z        = 0.0f;
		sp.mSegLen       = radius;
		sp.mDegree       = 0.0f;
	}
}

#pragma dont_inline on
bool TBGCheckData::isIllegalData() const
{
	return mFlags & BG_CHECK_FLAG_ILLEGAL ? true : false;
}
#pragma dont_inline off

void TSphereLink::moveHead(const JGeometry::TVec3<f32>& head)
{
	for (int i = 0; i < mCount; i++) {
		mPoints[i].mPos.y += mGravityY;
		mPoints[i].mPos.add(mPoints[i].mVel);
	}

	mPoints[0].mPos = head;

	JGeometry::TVec3<f32>& headPos = mPoints[0].mPos;
	gpMap->isTouchedOneWallAndMoveXZ(&headPos.x, headPos.y, &headPos.z, m10);

	const TBGCheckData* ground;
	f32 groundY                = gpMap->checkGroundIgnoreWaterSurface(
        headPos.x, headPos.y + m10, headPos.z, &ground);
	if (ground != nullptr && !ground->isIllegalData() && headPos.y < groundY) {
		headPos.y = groundY;
	}

	for (int i = 1; i < mCount; i++) {
		TSpherePoint& sp           = mPoints[i];
		JGeometry::TVec3<f32> diff = sp.mPos;
		diff.sub(mPoints[i - 1].mPos);
		JGeometry::TVec3<f32> dir = diff;
		if (dir.squared() <= 3.81469727e-06f) {
			dir.x = 0.0f;
			dir.y = 1.0f;
			dir.z = 0.0f;
		} else {
			PSVECNormalize(&dir, &dir);
		}
		dir.scale(sp.mSegLen);

		JGeometry::TVec3<f32> newPos = mPoints[i - 1].mPos;
		newPos.add(dir);
		sp.mPos = newPos;

		JGeometry::TVec3<f32>& spPos = sp.mPos;
		gpMap->isTouchedOneWallAndMoveXZ(&spPos.x, spPos.y, &spPos.z, m10);

		const TBGCheckData* ground2;
		f32 groundY2                = gpMap->checkGroundIgnoreWaterSurface(
            spPos.x, spPos.y + m10, spPos.z, &ground2);
		if (ground2 != nullptr && !ground2->isIllegalData()
		    && spPos.y < groundY2) {
			spPos.y = groundY2;
		}
	}

	for (int i = 0; i < mCount; i++) {
		TSpherePoint& sp = mPoints[i];
		sp.mVel          = (sp.mPos - sp.mPrev) * m08;
		sp.mPrev         = sp.mPos;
	}
}

BOOL TSphereLink::setDegreeZAndRevisionPosXZ(int index, f32 newDeg)
{
	TSpherePoint& p = mPoints[index];
	BOOL ok         = FALSE;
	f32 oldDeg      = p.mDegree;
	if (oldDeg != newDeg) {
		p.mDegree = newDeg;
		ok        = TRUE;

		f32 baseAngle;
		if (index == 0) {
			baseAngle = mAngleOffset;
		} else {
			JGeometry::TVec3<f32> delta = mPoints[index - 1].mPos;
			delta.sub(p.mPos);
			baseAngle = MsGetRotFromZaxisY(delta);
			while (baseAngle >= 360.0f)
				baseAngle -= 360.0f;
			while (baseAngle < 0.0f)
				baseAngle += 360.0f;
		}

		f32 magnitude = m14 * (newDeg - oldDeg);
		s16 angle = CLBRoundf<s16>(baseAngle * (65536.0f / 360.0f));
		f32 sinV  = jmaSinTable[(u16)angle >> jmaSinShift];
		f32 cosV  = jmaCosTable[(u16)angle >> jmaSinShift];
		f32 zero  = 0.0f;

		p.mPos.x += magnitude * cosV + zero * sinV;
		p.mPos.z += -magnitude * sinV + zero * cosV;
	}
	return ok;
}
