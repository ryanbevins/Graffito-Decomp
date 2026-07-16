#include <Camera/Camera.hpp>
#include <Camera/CameraInbetween.hpp>
#include <Camera/CameraKindParam.hpp>
#include <Camera/CameraMarioData.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/JGeometry.hpp>
#include <MarioUtil/MathUtil.hpp>

static inline void copyCameraState(CPolarSubCamera* camera)
{
	*(Vec*)((u8*)camera + 0xB4) = *(Vec*)((u8*)camera + 0x80);
	*(Vec*)((u8*)camera + 0xC0) = *(Vec*)((u8*)camera + 0x8C);
	*(Vec*)((u8*)camera + 0xCC) = *(Vec*)((u8*)camera + 0x98);
	*(s16*)((u8*)camera + 0xD8) = *(s16*)((u8*)camera + 0xA4);
	*(s16*)((u8*)camera + 0xDA) = *(s16*)((u8*)camera + 0xA6);
	*(f32*)((u8*)camera + 0xDC) = *(f32*)((u8*)camera + 0xA8);
	*(s16*)((u8*)camera + 0xE0) = *(s16*)((u8*)camera + 0xAC);
	*(f32*)((u8*)camera + 0xE4) = *(f32*)((u8*)camera + 0xB0);
}

void CPolarSubCamera::warpPosAndAt(const Vec& pos, const Vec& lookat)
{
	if (mMode >= CAMERA_MODE_REPRODUCE_DEMO)
		return;

	mCurrentParams->copySaveParam(*unk2D8[mMode]);

	killHeightPan_();

	mPosition.set(pos);
	mTarget.set(lookat);
	unk124.set(pos);
	unk148.set(lookat);

	unk6C->warpPosAndAt(pos, lookat);
	unk6C->mFrameCount = 0;

	calcNowTargetFromPosAndAt_(pos, lookat);

	copyCameraState(this);
}

void CPolarSubCamera::warpPosAndAt(f32 dist, s16 angY)
{
	if (mMode >= CAMERA_MODE_REPRODUCE_DEMO)
		return;

	mCurrentParams->copySaveParam(*unk2D8[mMode]);

	JGeometry::TVec3<f32> lookat;
	lookat.set(getUsualLookat());

	if (isLButtonCameraSpecifyMode(mMode)) {
		f32 v = dist;
		if (dist > 1.0f)
			v = 1.0f;
		else if (dist < 0.0f)
			v = 0.0f;
		mCurrentTarget.unk28 = v;
	} else {
		f32 hi = unk26C;
		f32 lo = unk268;
		f32 v  = dist;
		if (dist > hi)
			v = hi;
		else if (dist < lo)
			v = lo;
		mCurrentTarget.unk28 = v;
	}

	mCurrentTarget.mPitch = (s16)calcAngleXFromXRotRatio_();
	mCurrentTarget.mYaw = angY;
	f32 polarDist = calcDistFromXRotRatio_();

	JGeometry::TVec3<f32> pos;
	CLBPolarToCross(lookat, &pos, polarDist, mCurrentTarget.mPitch,
	                mCurrentTarget.mYaw);

	warpPosAndAt(pos, lookat);
}

void CPolarSubCamera::addMoveCameraAndMario(const Vec& v)
{
	mPosition += v;
	mTarget += v;
	unk124 += v;
	unk148 += v;

	gpCameraMario->addMoveCameraAndMario(v);

	unk6C->addMoveCameraAndMario(v);

	mCurrentTarget.mPosition += v;
	mCurrentTarget.mTarget += v;
	mCurrentTarget.unk18 += v;

	mPreviousTarget.mPosition += v;
	mPreviousTarget.mTarget += v;
	mPreviousTarget.unk18 += v;
}
