#include <Camera/Camera.hpp>
#include <Camera/CameraKindParam.hpp>
#include <Camera/CameraMapTool.hpp>
#include <Camera/cameralib.hpp>
#include <Enemy/BossGesso.hpp>
#include <MarioUtil/MapUtil.hpp>
#include <MSound/MSound.hpp>
#include <Map/MapData.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/MarioMain.hpp>
#include <Strategic/HitActor.hpp>
#include <Strategic/TakeActor.hpp>
#include <System/MarioGamePad.hpp>

#pragma dont_inline on

void CPolarSubCamera::execCameraModeChangeProc_(int mode) { (void)mode; }

bool CPolarSubCamera::isChangeToParallelCameraCByMoveBG_() const
{
	bool result = false;
	if (SMS_GetGroundActor(SMS_GetMarioGrPlane(), 0x4000012F) != nullptr) {
		result = true;
	}
	return result;
}

bool CPolarSubCamera::isChangeToParallelCameraByMoveBG_() const
{
	bool result = false;
	TTakeActor* holder = gpMarioOriginal->mHolder;
	if (holder != nullptr) {
		u32 type = holder->getActorType();
		if (type == 0x400000BB || type == 0x40000049) {
			result = true;
		}
	}
	if (!result) {
		if (SMS_GetMarioGrPlane() != nullptr) {
			const TLiveActor* actor = SMS_GetMarioGrPlane()->getActor();
			if (actor != nullptr) {
				u32 atype
				    = static_cast<const THitActor*>(actor)->getActorType();
				switch (atype) {
				case 0x4000012E:
					result = true;
					break;
				case 0x4000009C:
				case 0x400000A5:
				case 0x4000022E:
				case 0x40000249:
					if (SMS_IsMarioTouchGround4cm()) {
						result = true;
					}
					break;
				}
			}
		}
	}
	return result;
}

bool CPolarSubCamera::isChangeToCancanCamera_() const
{
	bool result = false;
	TTakeActor* held = gpMarioOriginal->mHeldObject;
	if (held != nullptr && held->getActorType() == 0x10000028) {
		result = true;
	}
	return result;
}

bool CPolarSubCamera::isChangeToBossGesoCamera_() const
{
	bool result = false;
	TTakeActor* held = gpMarioOriginal->mHeldObject;
	if (held != nullptr) {
		u32 type = held->getActorType();
		if (type == 0x08000006 || type == 0x08000008) {
			TBossGesso* gesso = *(TBossGesso**)((u8*)this + 0x2A8);
			if (gesso != nullptr) {
				if (gesso->beakHeld() || gesso->tentacleHeld()) {
					result = true;
				}
			}
		}
	}
	return result;
}

void CPolarSubCamera::doLButtonCameraOff_(bool flag)
{
	bool ready = false;
	if (isLButtonCameraSpecifyMode(mMode)) {
		if (!isNowInbetween()) {
			ready = true;
		}
	}
	bool go = ready ? true : false;
	if (go) {
		*(s16*)((u8*)this + 0x282) = 0x3C;
		if (flag) {
			*(TCameraMapTool**)((u8*)this + 0x74) = unk70;
			unk70 = nullptr;
			changeCamModeSub_(-1, 1, false);
		} else {
			gpMSound->startSoundSystemSE(0x4825, 0, nullptr, 0);
			s16 frame = (s16)getCameraInbetweenFrame_(-1);
			*(TCameraMapTool**)((u8*)this + 0x74) = unk70;
			unk70 = nullptr;
			changeCamModeSub_(-1, frame, false);
		}
		if (*(u16*)((u8*)this + 0x64) & 0x20) {
			execNoticeOnOffProc_((EnumNoticeOnOffMode)0);
		}
	}
}

void CPolarSubCamera::execFrontRotate_()
{
	if (isLButtonCameraSpecifyMode(mMode))
		return;
	if (SMS_GetMarioStatus() == 0x8008A9)
		return;

	*(u16*)((u8*)this + 0x64) &= ~0x10;
	*(u16*)((u8*)this + 0x64) |= 0x4;

	*(s16*)((u8*)this + 0x274) = *gpMarioAngleY + (s16)0x8000;

	u32 m = unk120->mEnabledFrameMeaning;
	if (m & 0x4000) {
		s16 v = *(s16*)((u8*)*(void**)((u8*)this + 0x2D4) + 0x11C);
		*(s16*)((u8*)this + 0x276) = v;
		*(u16*)((u8*)this + 0x64) |= 0x8;
	} else if (m & 0x8000) {
		s16 v = *(s16*)((u8*)*(void**)((u8*)this + 0x2D4) + 0x130);
		*(s16*)((u8*)this + 0x276) = v;
		*(u16*)((u8*)this + 0x64) &= ~0x8;
		gpMSound->startSoundSystemSE(0x4826, 0, nullptr, 0);
	}
}

void CPolarSubCamera::changeCamModeSpecifyCamMapToolAndFrame_(
    const TCameraMapTool* tool, int frame)
{
	int newMode = tool->unk24;
	if (mMode == newMode && unk70 == tool) {
		return;
	}
	*(TCameraMapTool**)((u8*)this + 0x74) = unk70;
	unk70 = const_cast<TCameraMapTool*>(tool);
	changeCamModeSub_(newMode, frame, true);
}

void CPolarSubCamera::changeCamModeSpecifyCamMapTool_(const TCameraMapTool* tool)
{
	int newMode = tool->unk24;
	if (mMode == newMode && unk70 == tool) {
		return;
	}
	*(TCameraMapTool**)((u8*)this + 0x74) = unk70;
	unk70 = const_cast<TCameraMapTool*>(tool);
	s16 frame = (s16)getCameraInbetweenFrame_(newMode);
	changeCamModeSub_(newMode, frame, true);
}

void CPolarSubCamera::changeCamModeSpecifyFrame_(int mode, int frame)
{
	*(TCameraMapTool**)((u8*)this + 0x74) = unk70;
	unk70 = nullptr;
	changeCamModeSub_(mode, frame, false);
}

void CPolarSubCamera::changeCamModeSub_(int newMode, int frame, bool flag)
{
	(void)newMode;
	(void)frame;
	(void)flag;
}

void CPolarSubCamera::setUpFromLButtonCamera_()
{
	f32 v = *(f32*)((u8*)this + 0xB0);
	*(f32*)((u8*)this + 0xA8) = v;
	*(f32*)((u8*)this + 0xDC) = v;
}

void CPolarSubCamera::setUpToLButtonCamera_(int mode)
{
	*(f32*)((u8*)this + 0xB0) = *(f32*)((u8*)this + 0xA8);

	TCameraKindParam buf;
	TCameraKindParam* kind
	    = *(TCameraKindParam**)((u8*)this + 0x2D8 + mode * 4);
	buf.copySaveParam(*(const TCamSaveKindParam*)kind);

	f32 ratio = CLBCalcRatio<s16>(buf.unk18, buf.unk1A, -buf.unk58);
	if (ratio > 1.0f) {
		ratio = 1.0f;
	} else if (ratio < 0.0f) {
		ratio = 0.0f;
	}
	*(f32*)((u8*)this + 0xA8) = ratio;
	*(f32*)((u8*)this + 0xDC) = ratio;
}

int CPolarSubCamera::getCameraInbetweenFrame_(int mode)
{
	(void)mode;
	return 1;
}

#pragma dont_inline off
