#include <Camera/CameraShake.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JGeometry.hpp>
#include <dolphin/mtx.h>

extern const char* mCamShakeNameSave__12TCameraShake[];

TCameraShake* gpCameraShake;

static inline void unitVecTo(const Vec& from, const Vec& to,
                             JGeometry::TVec3<f32>* out)
{
	out->set(to.x - from.x, to.y - from.y, to.z - from.z);
	out->normalize();
}

TCameraShake::TCameraShake()
{
	mYaw = 0;
	s32 offset = 0;
	for (s32 i = 0; i < 41; i++, offset += 4) {
		TCamSaveShake* save = new TCamSaveShake(
		    *(const char**)((const u8*)mCamShakeNameSave__12TCameraShake
		                    + offset));
		mShakeSaveData[i] = save;
	}
	for (s32 i = 0; i < 32; i++) {
		TCamShakeInfo& info = mShakeInfos[i];
		info.reset();
	}
}

TCameraShake::TCamShakeInfo* TCameraShake::getUseShakeData_()
{
	TCamShakeInfo* info = mShakeInfos;
	for (s32 i = 0; i < 32; i++, info++) {
		if (!info->isActive()) {
			return info;
		}
	}

	TCamShakeInfo* fallback = mShakeInfos;
	u16 best_delta = 0xFFFF;
	info = mShakeInfos;
	for (s32 i = 0; i < 32; i++, info++) {
		if (info->mActiveSet) {
			u16 remain = (u16)(info->mDuration - info->mCurFrame);
			if (remain < best_delta) {
				best_delta = remain;
				fallback   = info;
			}
		}
	}

	return fallback;
}

void TCameraShake::startShake(EnumCamShakeMode mode, f32 strength)
{
	TCamSaveShake* save = mShakeSaveData[(s32)mode];
	s16 duration = save->mShakeTime.get();
	if ((u16)duration == 0) {
		return;
	}

	TCamShakeInfo* info = getUseShakeData_();
	info->mMode      = (s32)mode;
	info->mPause     = 0;
	info->mActiveSet = 0;
	info->mDuration  = duration;
	info->mCurFrame  = 0;

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
		info->mAngleX.mPhase     = phase;
		info->mAngleX.mDecrement = phase * (1.0f / (f32)(u16)duration);
		info->mAngleX.mAngle     = angle;
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
		info->mAngleY.mPhase     = phase;
		info->mAngleY.mDecrement = phase * (1.0f / (f32)(u16)duration);
		info->mAngleY.mAngle     = angle;
	}

	// Z axis
	{
		f32 phase = *(const f32*)(sd + 0x7C) * strength;
		s16 angle = save->mShakeVelZ.get();
		if (strength < 0.0f) {
			phase = -phase;
			angle = -angle;
		}
		info->mAngleZ.mPhase     = phase;
		info->mAngleZ.mDecrement = phase * (1.0f / (f32)(u16)duration);
		info->mAngleZ.mAngle     = angle;
	}
}

void TCameraShake::keepShake(EnumCamShakeMode mode, f32 strength)
{
	TCamSaveShake* save = mShakeSaveData[(s32)mode];
	s16 duration = save->mShakeTime.get();
	if ((u16)duration == 0) {
		return;
	}

	// Search for an existing entry with this mode and ActiveSet=0
	TCamShakeInfo* found = mShakeInfos;
	for (s32 i = 0; i < 32; i++, found++) {
		if (found->mMode == (s32)mode && found->mActiveSet == 0) {
			found->mPause = 1;
			return;
		}
	}

	// Not found: allocate a new entry
	TCamShakeInfo* info = getUseShakeData_();
	info->mMode      = (s32)mode;
	info->mPause     = 1;
	info->mActiveSet = 0;
	info->mDuration  = duration;
	info->mCurFrame  = 0;

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
		info->mAngleX.mPhase     = phase;
		info->mAngleX.mDecrement = phase * (1.0f / (f32)(u16)duration);
		info->mAngleX.mAngle     = angle;
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
		info->mAngleY.mPhase     = phase;
		info->mAngleY.mDecrement = phase * (1.0f / (f32)(u16)duration);
		info->mAngleY.mAngle     = angle;
	}

	// Z axis
	{
		f32 phase = *(const f32*)(sd + 0x7C) * strength;
		s16 angle = save->mShakeVelZ.get();
		if (strength < 0.0f) {
			phase = -phase;
			angle = -angle;
		}
		info->mAngleZ.mPhase     = phase;
		info->mAngleZ.mDecrement = phase * (1.0f / (f32)(u16)duration);
		info->mAngleZ.mAngle     = angle;
	}
}

void TCameraShake::execShake(const JGeometry::TVec3<f32>& origin,
                             JGeometry::TVec3<f32>* pos,
                             JGeometry::TVec3<f32>* up)
{
	bool anyActive                = false;
	JGeometry::TVec3<f32> origPos = *pos;

	mYaw = 0;

	TCamShakeInfo* it = mShakeInfos;
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

		TCamShakeInfo* it = mShakeInfos;
		for (int i = 0; i < 32; ++i, ++it) {
			if (it->isActive()) {
				vAngle
				    += (s16)(it->mAngleX.mPhase
				             * JMASSin((s16)(it->mAngleX.mAngle
				                              * it->mCurFrame)));
				hAngle
				    += (s16)(it->mAngleY.mPhase
				             * JMASSin((s16)(it->mAngleY.mAngle
				                              * it->mCurFrame)));
				mYaw
				    += (s16)(it->mAngleZ.mPhase
				             * JMASSin((s16)(it->mAngleZ.mAngle
				                              * it->mCurFrame)));
				it->mCurFrame += 1;

				bool finished = false;
				if (it->mPause != 0) {
					it->mDuration += 1;
					it->mPause = 0;
				} else {
					it->mActiveSet = 1;
					it->mAngleX.mPhase -= it->mAngleX.mDecrement;
					it->mAngleY.mPhase -= it->mAngleY.mDecrement;
					it->mAngleZ.mPhase -= it->mAngleZ.mDecrement;
					if (it->mCurFrame >= it->mDuration)
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
	    dir, -(0.017453294f * (0.005493164f * (f32)mYaw)));

	JGeometry::TVec3<f32> oldUp = *up;
	up->x = oldUp.x * rot.at(0, 0) + oldUp.y * rot.at(1, 0)
	        + oldUp.z * rot.at(2, 0);
	up->y = oldUp.x * rot.at(0, 1) + oldUp.y * rot.at(1, 1)
	        + oldUp.z * rot.at(2, 1);
	up->z = oldUp.x * rot.at(0, 2) + oldUp.y * rot.at(1, 2)
	        + oldUp.z * rot.at(2, 2);
}
