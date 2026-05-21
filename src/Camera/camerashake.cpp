#include <Camera/CameraShake.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JGeometry.hpp>
#include <dolphin/mtx.h>

extern const char* mCamShakeNameSave__12TCameraShake[];

TCameraShake::TCameraShake()
{
	mYaw = 0;
	for (s32 i = 0; i < 41; i++) {
		TCamSaveShake* save = new TCamSaveShake(mCamShakeNameSave__12TCameraShake[i]);
		mShakeSaveData[i] = save;
	}
	for (s32 i = 0; i < 32; i++) {
		TCamShakeInfo& info = mShakeInfos[i];
		info.mMode             = 1;
		info.mPause            = 0;
		info.mActiveSet        = 0;
		info.mCurFrame         = 0;
		info.mDuration         = 0;
		info.mAngleX.mDecrement = 0.0f;
		info.mAngleX.mPhase    = 0.0f;
		info.mAngleX.mAngle    = 0;
		info.mAngleY.mDecrement = 0.0f;
		info.mAngleY.mPhase    = 0.0f;
		info.mAngleY.mAngle    = 0;
		info.mAngleZ.mDecrement = 0.0f;
		info.mAngleZ.mPhase    = 0.0f;
		info.mAngleZ.mAngle    = 0;
	}
}

TCameraShake::TCamShakeInfo* TCameraShake::getUseShakeData_()
{
	TCamShakeInfo* fallback = &mShakeInfos[0];
	u32 best_delta = 0xFFFF;

	// First pass: find first inactive entry
	for (s32 i = 0; i < 32; i++) {
		TCamShakeInfo& info = mShakeInfos[i];
		bool active = info.mDuration != 0 ? true : false;
		if (!active) {
			return &mShakeInfos[i];
		}
	}

	// Second pass: pick entry with smallest (mDuration - mCurFrame) among active ones
	for (s32 i = 0; i < 32; i++) {
		TCamShakeInfo& info = mShakeInfos[i];
		if (info.mActiveSet) {
			u16 remain = (u16)(info.mDuration - (u16)info.mCurFrame);
			if (remain < (u16)best_delta) {
				best_delta = remain;
				fallback   = &mShakeInfos[i];
			}
		}
	}

	return fallback;
}

void TCameraShake::setShakeAngleOne_(TCameraShake::TCamShakeAngle* angle,
                                     f32 phase, s16 initAngle, u16 duration,
                                     f32 strength)
{
	f32 phaseSigned;
	s16 angleSigned;

	if (strength < 0.0f) {
		phaseSigned = -phase;
		angleSigned = -initAngle;
	} else {
		phaseSigned = phase;
		angleSigned = initAngle;
	}
	angle->mPhase     = phaseSigned;
	angle->mDecrement = phaseSigned / (f32)duration;
	angle->mAngle     = angleSigned;
}

void TCameraShake::setShakeAngleAll_(TCameraShake::TCamShakeInfo* info,
                                     const TCamSaveShake* save, u16 duration,
                                     f32 strength)
{
	const u8* sd = (const u8*)save;
	setShakeAngleOne_(&info->mAngleX, *(const f32*)(sd + 0x2C) * strength,
	                  *(const s16*)(sd + 0x40), duration, strength);
	setShakeAngleOne_(&info->mAngleY, *(const f32*)(sd + 0x54) * strength,
	                  *(const s16*)(sd + 0x68), duration, strength);
	setShakeAngleOne_(&info->mAngleZ, *(const f32*)(sd + 0x7C) * strength,
	                  *(const s16*)(sd + 0x90), duration, strength);
}

void TCameraShake::startShake(EnumCamShakeMode mode, f32 strength)
{
	TCamSaveShake* save = mShakeSaveData[(s32)mode];
	u16 duration = *(const u16*)((const u8*)save + 0x18);
	if (duration == 0) {
		return;
	}

	TCamShakeInfo* info = getUseShakeData_();
	info->mMode      = (s32)mode;
	info->mPause     = 0;
	info->mActiveSet = 0;
	info->mDuration  = duration;
	info->mCurFrame  = 0;

	setShakeAngleAll_(info, save, duration, strength);
}

void TCameraShake::keepShake(EnumCamShakeMode mode, f32 strength)
{
	TCamSaveShake* save = mShakeSaveData[(s32)mode];
	u16 duration = *(const u16*)((const u8*)save + 0x18);
	if (duration == 0) {
		return;
	}

	// Search for an existing entry with this mode and ActiveSet=0
	for (s32 i = 0; i < 32; i++) {
		TCamShakeInfo& info = mShakeInfos[i];
		if (info.mMode == (s32)mode && info.mActiveSet == 0) {
			info.mPause = 1;
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

	setShakeAngleAll_(info, save, duration, strength);
}

void TCameraShake::execShake(const JGeometry::TVec3<f32>& center,
                             JGeometry::TVec3<f32>* outPos,
                             JGeometry::TVec3<f32>* outAt)
{
	JGeometry::TVec3<f32> saved = *outPos;
	bool anyActive = false;

	mYaw = 0;

	for (s32 i = 0; i < 32; i++) {
		if (mShakeInfos[i].mDuration != 0) {
			anyActive = true;
			break;
		}
	}

	if (anyActive) {
		f32 dist;
		s16 polarY;
		s16 polarX;
		CLBCrossToPolar(center, *outPos, &dist, &polarY, &polarX);

		for (s32 i = 0; i < 32; i++) {
			TCamShakeInfo& info = mShakeInfos[i];
			if (info.mDuration == 0) {
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
				info.mMode             = 1;
				info.mPause            = 0;
				info.mActiveSet        = 0;
				info.mCurFrame         = 0;
				info.mDuration         = 0;
				info.mAngleX.mDecrement = 0.0f;
				info.mAngleX.mPhase    = 0.0f;
				info.mAngleX.mAngle    = 0;
				info.mAngleY.mDecrement = 0.0f;
				info.mAngleY.mPhase    = 0.0f;
				info.mAngleY.mAngle    = 0;
				info.mAngleZ.mDecrement = 0.0f;
				info.mAngleZ.mPhase    = 0.0f;
				info.mAngleZ.mAngle    = 0;
			}
		}

		CLBPolarToCross(center, outPos, dist, polarY, polarX);
	}

	// Compute axis = (saved - center) normalized
	JGeometry::TVec3<f32> axis;
	axis.x = saved.x - center.x;
	axis.y = saved.y - center.y;
	axis.z = saved.z - center.z;

	f32 lenSq = axis.dot(axis);
	if (lenSq <= 0.0000038146973f) {
		axis.x = 0.0f;
		axis.y = 0.0f;
		axis.z = 0.0f;
	} else {
		axis.scale(1.0f * JGeometry::TUtil<f32>::inv_sqrt(lenSq), axis);
	}

	// Build rotation matrix for mYaw degrees about 'axis'.
	JGeometry::TRotation3<JGeometry::TMatrix33<JGeometry::SMatrix33C<f32> > > rot;
	rot.identity();
	f32 angleRad = -((f32)mYaw * 0.005493164f * 0.017453294f);
	rot.setRotate(axis, angleRad);

	// outAt' = rot^T * outAt (column-vector convention via column dot)
	JGeometry::TVec3<f32> saved_at = *outAt;
	outAt->x = saved_at.x * rot.mMtx[0][0] + saved_at.y * rot.mMtx[1][0]
	         + saved_at.z * rot.mMtx[2][0];
	outAt->y = saved_at.x * rot.mMtx[0][1] + saved_at.y * rot.mMtx[1][1]
	         + saved_at.z * rot.mMtx[2][1];
	outAt->z = saved_at.x * rot.mMtx[0][2] + saved_at.y * rot.mMtx[1][2]
	         + saved_at.z * rot.mMtx[2][2];
}
