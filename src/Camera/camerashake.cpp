#include <Camera/CameraShake.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JGeometry.hpp>
#include <dolphin/mtx.h>

extern const char* mCamShakeNameSave__12TCameraShake[];

static inline void fakeSetRotate(
    JGeometry::TRotation3<JGeometry::TMatrix33<JGeometry::SMatrix33C<f32> > >* rot,
    const JGeometry::TVec3<f32>& axis, f32 angle)
{
	rot->setRotate(axis, angle);
}

static inline void normalizeAxis(JGeometry::TVec3<f32>* vec)
{
	vec->normalize();
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

void TCameraShake::execShake(const JGeometry::TVec3<f32>& center,
                             JGeometry::TVec3<f32>* outPos,
                             JGeometry::TVec3<f32>* outAt)
{
	JGeometry::TVec3<f32> saved = *outPos;
	bool anyActive = false;

	mYaw = 0;

	{
		TCamShakeInfo* p = mShakeInfos;
		for (s32 i = 0; i < 32; i++, p++) {
			if (p->isActive()) {
				anyActive = true;
				break;
			}
		}
	}

	if (anyActive) {
		f32 dist;
		s16 polarY;
		s16 polarX;
		CLBCrossToPolar(center, *outPos, &dist, &polarY, &polarX);

		for (s32 i = 0; i < 32; i++) {
			TCamShakeInfo& info = mShakeInfos[i];
			if (!info.isActive()) {
				continue;
			}

			// Accumulate sinusoidal shake into polar Y, polar X, and yaw.
			s16 freqX = (s16)((s16)info.mAngleX.mAngle * (u16)info.mCurFrame);
			polarY = (s16)(polarY + (s32)(info.mAngleX.mPhase * JMASSin(freqX)));

			s16 freqY = (s16)((s16)info.mAngleY.mAngle * (u16)info.mCurFrame);
			polarX = (s16)(polarX + (s32)(info.mAngleY.mPhase * JMASSin(freqY)));

			s16 freqZ = (s16)((s16)info.mAngleZ.mAngle * (u16)info.mCurFrame);
			mYaw = (s16)(mYaw + (s32)(info.mAngleZ.mPhase * JMASSin(freqZ)));

			info.mCurFrame++;

			bool done;
			if (info.mPause != 0) {
				info.mDuration++;
				info.mPause = 0;
				done        = false;
			} else {
				info.mActiveSet = 1;
				info.mAngleX.mPhase -= info.mAngleX.mDecrement;
				info.mAngleY.mPhase -= info.mAngleY.mDecrement;
				info.mAngleZ.mPhase -= info.mAngleZ.mDecrement;
				done = info.mCurFrame >= info.mDuration ? true : false;
			}

			if (done) {
				info.reset();
			}
		}

		CLBPolarToCross(center, outPos, dist, polarY, polarX);
	}

	// Compute axis = normalize(saved - center)
	JGeometry::TVec3<f32> axis;
	axis.x = saved.x - center.x;
	axis.y = saved.y - center.y;
	axis.z = saved.z - center.z;
	normalizeAxis(&axis);

	// Build rotation matrix for mYaw degrees about 'axis'.
	JGeometry::TRotation3<JGeometry::TMatrix33<JGeometry::SMatrix33C<f32> > > rot;
	rot.identity();
	f32 angleRad = -((f32)mYaw * 0.005493164f * 0.017453294f);
	fakeSetRotate(&rot, axis, angleRad);

	// outAt' = rot^T * outAt (column-vector convention via column dot)
	JGeometry::TVec3<f32> saved_at = *outAt;
	outAt->x = saved_at.x * rot.mMtx[0][0] + saved_at.y * rot.mMtx[1][0]
	         + saved_at.z * rot.mMtx[2][0];
	outAt->y = saved_at.x * rot.mMtx[0][1] + saved_at.y * rot.mMtx[1][1]
	         + saved_at.z * rot.mMtx[2][1];
	outAt->z = saved_at.x * rot.mMtx[0][2] + saved_at.y * rot.mMtx[1][2]
	         + saved_at.z * rot.mMtx[2][2];
}
