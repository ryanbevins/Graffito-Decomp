#include <Camera/Camera.hpp>
#include <Camera/CameraInbetween.hpp>
#include <Camera/CameraKindParam.hpp>
#include <Camera/CameraMarioData.hpp>
#include <Camera/cameralib.hpp>
#include <Player/MarioAccess.hpp>

static inline void addVec3At(void* base, u32 off, const Vec& d)
{
	f32* p = (f32*)((u8*)base + off);
	p[0] += d.x;
	p[1] += d.y;
	p[2] += d.z;
}

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

void CPolarSubCamera::addMoveCameraAndMario(const Vec& delta)
{
	addVec3At(this, 0x10, delta);
	addVec3At(this, 0x3C, delta);
	addVec3At(this, 0x124, delta);
	addVec3At(this, 0x148, delta);

	JGeometry::TVec3<f32> tmp;
	tmp.set(delta);
	gpCameraMario->mPosX += tmp.x;
	gpCameraMario->mPosY += tmp.y;
	gpCameraMario->mPosZ += tmp.z;

	TCameraInbetween* inb = *(TCameraInbetween**)((u8*)this + 0x6C);
	inb->addMoveCameraAndMario(delta);

	addVec3At(this, 0x80, delta);
	addVec3At(this, 0x8C, delta);
	addVec3At(this, 0x98, delta);
	addVec3At(this, 0xB4, delta);
	addVec3At(this, 0xC0, delta);
	addVec3At(this, 0xCC, delta);
}

void CPolarSubCamera::warpPosAndAt(f32 dist, s16 angY)
{
	if (mMode >= 0x49)
		return;

	u8* saveParam = (u8*)this + (mMode << 2);
	unk68->copySaveParam(
	    **(const TCamSaveKindParam**)(saveParam + 0x2D8));

	JGeometry::TVec3<f32> lookat = getUsualLookat();

	// Clamp dist
	if (isLButtonCameraSpecifyMode(mMode)) {
		f32 v = dist;
		if (dist > 1.0f)
			v = 1.0f;
		else if (dist < 0.0f)
			v = 0.0f;
		*(f32*)((u8*)this + 0xA8) = v;
	} else {
		f32 hi = *(f32*)((u8*)this + 0x26C);
		f32 lo = *(f32*)((u8*)this + 0x268);
		f32 v  = dist;
		if (dist > hi)
			v = hi;
		else if (dist < lo)
			v = lo;
		*(f32*)((u8*)this + 0xA8) = v;
	}

	*(s16*)((u8*)this + 0xA4) = (s16)calcAngleXFromXRotRatio_();
	*(s16*)((u8*)this + 0xA6) = angY;
	calcDistFromXRotRatio_();

	JGeometry::TVec3<f32> pos;
	CLBPolarToCross(lookat, &pos, *(f32*)((u8*)this + 0xA8),
	                *(s16*)((u8*)this + 0xA4), *(s16*)((u8*)this + 0xA6));

	if (mMode >= 0x49)
		return;

	saveParam = (u8*)this + (mMode << 2);
	unk68->copySaveParam(
	    **(const TCamSaveKindParam**)(saveParam + 0x2D8));
	killHeightPan_();

	// Sync pos and lookat into several fields
	*(f32*)((u8*)this + 0x10)  = pos.x;
	*(f32*)((u8*)this + 0x14)  = pos.y;
	*(f32*)((u8*)this + 0x18)  = pos.z;
	*(f32*)((u8*)this + 0x3C)  = lookat.x;
	*(f32*)((u8*)this + 0x40)  = lookat.y;
	*(f32*)((u8*)this + 0x44)  = lookat.z;
	*(f32*)((u8*)this + 0x124) = pos.x;
	*(f32*)((u8*)this + 0x128) = pos.y;
	*(f32*)((u8*)this + 0x12C) = pos.z;
	*(f32*)((u8*)this + 0x148) = lookat.x;
	*(f32*)((u8*)this + 0x14C) = lookat.y;
	*(f32*)((u8*)this + 0x150) = lookat.z;

	(*(TCameraInbetween**)((u8*)this + 0x6C))->warpPosAndAt(pos, lookat);
	(*(TCameraInbetween**)((u8*)this + 0x6C))->mFrameCount = 0;

	calcNowTargetFromPosAndAt_(pos, lookat);

	copyCameraState(this);
}

void CPolarSubCamera::warpPosAndAt(const Vec& pos, const Vec& lookat)
{
	if (mMode >= 0x49)
		return;

	u8* saveParam = (u8*)this + (mMode << 2);
	unk68->copySaveParam(
	    **(const TCamSaveKindParam**)(saveParam + 0x2D8));

	killHeightPan_();

	*(f32*)((u8*)this + 0x10)  = pos.x;
	*(f32*)((u8*)this + 0x14)  = pos.y;
	*(f32*)((u8*)this + 0x18)  = pos.z;
	*(f32*)((u8*)this + 0x3C)  = lookat.x;
	*(f32*)((u8*)this + 0x40)  = lookat.y;
	*(f32*)((u8*)this + 0x44)  = lookat.z;
	*(f32*)((u8*)this + 0x124) = pos.x;
	*(f32*)((u8*)this + 0x128) = pos.y;
	*(f32*)((u8*)this + 0x12C) = pos.z;
	*(f32*)((u8*)this + 0x148) = lookat.x;
	*(f32*)((u8*)this + 0x14C) = lookat.y;
	*(f32*)((u8*)this + 0x150) = lookat.z;

	(*(TCameraInbetween**)((u8*)this + 0x6C))->warpPosAndAt(pos, lookat);
	(*(TCameraInbetween**)((u8*)this + 0x6C))->mFrameCount = 0;

	calcNowTargetFromPosAndAt_(pos, lookat);

	copyCameraState(this);
}
