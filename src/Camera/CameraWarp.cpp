#include <Camera/Camera.hpp>
#include <Camera/CameraInbetween.hpp>
#include <Camera/CameraMarioData.hpp>
#include <Camera/cameralib.hpp>
#include <Player/MarioAccess.hpp>

// Forward declarations for types in still-empty TUs
struct TCamSaveKindParam;
class TCameraKindParam {
public:
	void copySaveParam(const TCamSaveKindParam&);
};

void CPolarSubCamera::addMoveCameraAndMario(const Vec& delta)
{
	// Add delta to multiple Vec3 fields scattered through the camera
	static const u32 kOffsets[]
	    = { 0x10, 0x3C, 0x124, 0x148, 0x80, 0x8C, 0x98, 0xB4, 0xC0, 0xCC };
	for (u32 i = 0; i < sizeof(kOffsets) / sizeof(kOffsets[0]); i++) {
		f32* p = (f32*)((u8*)this + kOffsets[i]);
		p[0] += delta.x;
		p[1] += delta.y;
		p[2] += delta.z;
	}

	// Apply to gpCameraMario position
	gpCameraMario->mPosX += delta.x;
	gpCameraMario->mPosY += delta.y;
	gpCameraMario->mPosZ += delta.z;

	// Apply to camera inbetween
	TCameraInbetween* inb = *(TCameraInbetween**)((u8*)this + 0x6C);
	if (inb)
		inb->addMoveCameraAndMario(delta);
}

void CPolarSubCamera::warpPosAndAt(f32 dist, s16 angY)
{
	if (mMode >= 0x49)
		return;

	void* paramObj          = *(void**)((u8*)this + 0x68);
	TCameraKindParam** save = (TCameraKindParam**)((u8*)this + 0x2D8);
	((TCameraKindParam*)paramObj)
	    ->copySaveParam(*(const TCamSaveKindParam*)save[mMode]);

	JGeometry::TVec3<f32> lookat = getUsualLookat();

	// Clamp dist
	if (isLButtonCameraSpecifyMode(mMode)) {
		f32 v = dist;
		if (dist > 0.0f)
			v = 0.0f;
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

	((TCameraKindParam*)paramObj)
	    ->copySaveParam(*(const TCamSaveKindParam*)save[mMode]);
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

	TCameraInbetween* inb = *(TCameraInbetween**)((u8*)this + 0x6C);
	inb->warpPosAndAt(pos, lookat);
	inb->mFrameCount = 0;

	calcNowTargetFromPosAndAt_(pos, lookat);

	// Copy 0x80..0xB0 block to 0xB4..0xE4
	for (u32 i = 0; i < 0x30; i += 4) {
		*(u32*)((u8*)this + 0xB4 + i) = *(u32*)((u8*)this + 0x80 + i);
	}
}

void CPolarSubCamera::warpPosAndAt(const Vec& pos, const Vec& lookat)
{
	if (mMode >= 0x49)
		return;

	void* paramObj          = *(void**)((u8*)this + 0x68);
	TCameraKindParam** save = (TCameraKindParam**)((u8*)this + 0x2D8);
	((TCameraKindParam*)paramObj)
	    ->copySaveParam(*(const TCamSaveKindParam*)save[mMode]);

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

	TCameraInbetween* inb = *(TCameraInbetween**)((u8*)this + 0x6C);
	inb->warpPosAndAt(pos, lookat);
	inb->mFrameCount = 0;

	calcNowTargetFromPosAndAt_(pos, lookat);

	for (u32 i = 0; i < 0x30; i += 4) {
		*(u32*)((u8*)this + 0xB4 + i) = *(u32*)((u8*)this + 0x80 + i);
	}
}
