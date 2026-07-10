#define JGEOMETRY_CAMERASHAKE_ROTATION3_SETROTATE_OUT_OF_LINE
#define JGEOMETRY_CAMERASHAKE_TVEC3_DOT_SCALE_OUT_OF_LINE
#include <Camera/CameraShake.hpp>
#undef JGEOMETRY_CAMERASHAKE_TVEC3_DOT_SCALE_OUT_OF_LINE
#undef JGEOMETRY_CAMERASHAKE_ROTATION3_SETROTATE_OUT_OF_LINE
#include <Camera/camerasave.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JGeometry.hpp>
#include <dolphin/mtx.h>
#include <macros.h>

TCameraShake* gpCameraShake;

static inline void unitVecTo(const Vec& from, const Vec& to,
                             JGeometry::TVec3<f32>* out)
{
	out->set(to.x - from.x, to.y - from.y, to.z - from.z);
	out->normalize();
}

#pragma strength off

TCameraShake::TCameraShake()
{
	mRollAccum = 0;
	for (int i = 0; i < ARRAY_COUNT(mShakeData); ++i)
		mShakeData[i] = new TCamSaveShake(mCamShakeNameSave[i]);

	for (int i = 0; i < ARRAY_COUNT(mShakeInfo); ++i)
		mShakeInfo[i].reset();
}

TCameraShake::TCamShakeInfo* TCameraShake::getUseShakeData_()
{
	int i;
	for (i = 0; i < ARRAY_COUNT(mShakeInfo); ++i)
		if (!mShakeInfo[i].isActive())
			return &mShakeInfo[i];

	TCamShakeInfo* best = mShakeInfo;
	u16 minRemaining    = -1;
	for (i = 0; i < ARRAY_COUNT(mShakeInfo); ++i) {
		if (mShakeInfo[i].mIsDecreasing != 0) {
			u16 remaining = mShakeInfo[i].mDuration - mShakeInfo[i].mFrame;
			if (remaining < minRemaining) {
				minRemaining = remaining;
				best         = &mShakeInfo[i];
			}
		}
	}
	return best;
}

void TCameraShake::startShake(EnumCamShakeMode mode, f32 strength)
{
	TCamSaveShake* save = mShakeData[(s32)mode];
	s16 duration = save->mShakeTime.get();
	if ((u16)duration == 0) {
		return;
	}

	TCamShakeInfo* info = getUseShakeData_();
	info->mMode         = (s32)mode;
	info->mIsKeep       = 0;
	info->mIsDecreasing = 0;
	info->mDuration     = duration;
	info->mFrame        = 0;

	const u8* sd = (const u8*)save;

	// X axis
	{
		f32 phase = save->mShakeAmpX.get();
		phase *= strength;
		s16 angle = save->mShakeVelX.get();
		if (strength < 0.0f) {
			phase = -phase;
			angle = -angle;
		}
		info->mAngleX.mAmp = phase;
		info->mAngleX.mDec = phase * (1.0f / (f32)(u16)duration);
		info->mAngleX.mVel = angle;
	}

	// Y axis
	{
		f32 phase = save->mShakeAmpY.get();
		phase *= strength;
		s16 angle = save->mShakeVelY.get();
		if (strength < 0.0f) {
			phase = -phase;
			angle = -angle;
		}
		info->mAngleY.mAmp = phase;
		info->mAngleY.mDec = phase * (1.0f / (f32)(u16)duration);
		info->mAngleY.mVel = angle;
	}

	// Z axis
	{
		f32 phase = *(const f32*)(sd + 0x7C) * strength;
		s16 angle = save->mShakeVelZ.get();
		if (strength < 0.0f) {
			phase = -phase;
			angle = -angle;
		}
		info->mAngleZ.mAmp = phase;
		info->mAngleZ.mDec = phase * (1.0f / (f32)(u16)duration);
		info->mAngleZ.mVel = angle;
	}
}

void TCameraShake::keepShake(EnumCamShakeMode mode, f32 strength)
{
	TCamSaveShake* save = mShakeData[(s32)mode];
	s16 duration = save->mShakeTime.get();
	if ((u16)duration == 0) {
		return;
	}

	// Search for an existing entry with this mode and mIsDecreasing=0
	TCamShakeInfo* found = mShakeInfo;
	for (s32 i = 0; i < 32; i++, found++) {
		if (found->mMode == (s32)mode && found->mIsDecreasing == 0) {
			found->mIsKeep = 1;
			return;
		}
	}

	// Not found: allocate a new entry
	TCamShakeInfo* info = getUseShakeData_();
	info->mMode         = (s32)mode;
	info->mIsKeep       = 1;
	info->mIsDecreasing = 0;
	info->mDuration     = duration;
	info->mFrame        = 0;

	const u8* sd = (const u8*)save;

	// X axis
	{
		f32 phase = save->mShakeAmpX.get();
		phase *= strength;
		s16 angle = save->mShakeVelX.get();
		if (strength < 0.0f) {
			phase = -phase;
			angle = -angle;
		}
		info->mAngleX.mAmp = phase;
		info->mAngleX.mDec = phase * (1.0f / (f32)(u16)duration);
		info->mAngleX.mVel = angle;
	}

	// Y axis
	{
		f32 phase = save->mShakeAmpY.get();
		phase *= strength;
		s16 angle = save->mShakeVelY.get();
		if (strength < 0.0f) {
			phase = -phase;
			angle = -angle;
		}
		info->mAngleY.mAmp = phase;
		info->mAngleY.mDec = phase * (1.0f / (f32)(u16)duration);
		info->mAngleY.mVel = angle;
	}

	// Z axis
	{
		f32 phase = *(const f32*)(sd + 0x7C) * strength;
		s16 angle = save->mShakeVelZ.get();
		if (strength < 0.0f) {
			phase = -phase;
			angle = -angle;
		}
		info->mAngleZ.mAmp = phase;
		info->mAngleZ.mDec = phase * (1.0f / (f32)(u16)duration);
		info->mAngleZ.mVel = angle;
	}
}

void TCameraShake::execShake(const JGeometry::TVec3<f32>& origin,
                             JGeometry::TVec3<f32>* pos,
                             JGeometry::TVec3<f32>* up)
{
	bool anyActive                = false;
	JGeometry::TVec3<f32> origPos = *pos;

	mRollAccum = 0;

	TCamShakeInfo* it = mShakeInfo;
	for (int i = 0; i < 32; ++i, ++it) {
		if (it->isActive()) {
			anyActive = true;
			break;
		}
	}

	if (anyActive) {
		f32 r;
		s16 vAngle, hAngle;
		CLBCrossToPolar(origin, *pos, &r, &vAngle, &hAngle);

		TCamShakeInfo* it = mShakeInfo;
		for (int i = 0; i < 32; ++i, ++it) {
			if (it->isActive()) {
				vAngle
				    += (s16)(it->mAngleX.mAmp
				             * JMASSin((s16)(it->mAngleX.mVel
				                              * it->mFrame)));
				hAngle
				    += (s16)(it->mAngleY.mAmp
				             * JMASSin((s16)(it->mAngleY.mVel
				                              * it->mFrame)));
				mRollAccum
				    += (s16)(it->mAngleZ.mAmp
				             * JMASSin((s16)(it->mAngleZ.mVel
				                              * it->mFrame)));
				it->mFrame += 1;

				bool finished = false;
				if (it->mIsKeep != 0) {
					it->mDuration += 1;
					it->mIsKeep = 0;
				} else {
					it->mIsDecreasing = 1;
					it->mAngleX.mAmp -= it->mAngleX.mDec;
					it->mAngleY.mAmp -= it->mAngleY.mDec;
					it->mAngleZ.mAmp -= it->mAngleZ.mDec;
					if (it->mFrame >= it->mDuration)
						finished = true;
				}

				if (finished)
					it->reset();
			}
		}

		CLBPolarToCross(origin, pos, r, vAngle, hAngle);
	}

	JGeometry::TVec3<f32> dir;
	unitVecTo(*pos, origPos, &dir);

	JGeometry::TRotation3<TMtx33f> rot(
	    dir, -(0.017453294f * (0.005493164f * (f32)mRollAccum)));

	const JGeometry::TVec3<f32> oldUp = *up;
	up->x = oldUp.x * rot.at(0, 0) + oldUp.y * rot.at(1, 0)
	        + oldUp.z * rot.at(2, 0);
	up->y = oldUp.x * rot.at(0, 1) + oldUp.y * rot.at(1, 1)
	        + oldUp.z * rot.at(2, 1);
	up->z = oldUp.x * rot.at(0, 2) + oldUp.y * rot.at(1, 2)
	        + oldUp.z * rot.at(2, 2);
}
