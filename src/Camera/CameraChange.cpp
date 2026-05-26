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
		switch (holder->getActorType()) {
		case 0x400000BB:
		case 0x40000049:
			result = true;
			break;
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
	*(f32*)((u8*)this + 0xB0) = unkA8;

	TCameraKindParam buf;
	u8* p = (u8*)this + mode * 4;
	buf.copySaveParam(*(const TCamSaveKindParam*)
	    *(TCameraKindParam**)(p + 0x2D8));

	f32 ratio = CLBCalcRatio<s16>(buf.unk18, buf.unk1A, -buf.unk58);
	if (ratio > 1.0f) {
		ratio = 1.0f;
	} else if (ratio < 0.0f) {
		ratio = 0.0f;
	}
	unkA8 = ratio;
	*(f32*)((u8*)this + 0xDC) = ratio;
}

int CPolarSubCamera::getCameraInbetweenFrame_(int newMode)
{
	if (newMode == -1) {
		u8* hist = *(u8**)((u8*)this + 0x60);
		int top  = *(int*)(hist + 0x4);
		int* p;
		if (top <= 0) {
			p = *(int**)(hist + 0x8);
		} else {
			p = *(int**)(hist + 0x8) + (top - 1);
		}
		newMode = *p;
	}

	int mode  = mMode;
	int frame = 1;
	if (mode < 0x49 && newMode < 0x49) {
	u8* p_base = (u8*)this + mode * 4;
	u8* save   = *(u8**)(p_base + 0x2D8);
	switch (newMode) {
	case 0:  frame = *(s16*)(save + 0x3C4); break;
	case 1:  frame = *(s16*)(save + 0x3D8); break;
	case 2:  frame = *(s16*)(save + 0x3EC); break;
	case 3:  frame = *(s16*)(save + 0x400); break;
	case 4:  frame = *(s16*)(save + 0x414); break;
	case 5:  frame = *(s16*)(save + 0x428); break;
	case 6:  frame = *(s16*)(save + 0x43C); break;
	case 7:  frame = *(s16*)(save + 0x450); break;
	case 8:  frame = *(s16*)(save + 0x464); break;
	case 9:  frame = *(s16*)(save + 0x478); break;
	case 10: frame = *(s16*)(save + 0x48C); break;
	case 11: frame = *(s16*)(save + 0x4A0); break;
	case 12: frame = *(s16*)(save + 0x4B4); break;
	case 13: frame = *(s16*)(save + 0x4C8); break;
	case 14: frame = *(s16*)(save + 0x4DC); break;
	case 15: frame = *(s16*)(save + 0x4F0); break;
	case 16: frame = *(s16*)(save + 0x504); break;
	case 17: frame = *(s16*)(save + 0x518); break;
	case 18: frame = *(s16*)(save + 0x52C); break;
	case 19: frame = *(s16*)(save + 0x540); break;
	case 20: frame = *(s16*)(save + 0x554); break;
	case 21: frame = *(s16*)(save + 0x568); break;
	case 22: frame = *(s16*)(save + 0x57C); break;
	case 23: frame = *(s16*)(save + 0x590); break;
	case 24: frame = *(s16*)(save + 0x5A4); break;
	case 25: frame = *(s16*)(save + 0x5B8); break;
	case 26: frame = *(s16*)(save + 0x5CC); break;
	case 27: frame = *(s16*)(save + 0x5E0); break;
	case 28: frame = *(s16*)(save + 0x5F4); break;
	case 29: frame = *(s16*)(save + 0x608); break;
	case 30: frame = *(s16*)(save + 0x61C); break;
	case 31: frame = *(s16*)(save + 0x630); break;
	case 32: frame = *(s16*)(save + 0x644); break;
	case 33: frame = *(s16*)(save + 0x658); break;
	case 34: frame = *(s16*)(save + 0x66C); break;
	case 35: frame = *(s16*)(save + 0x680); break;
	case 36: frame = *(s16*)(save + 0x694); break;
	case 37: frame = *(s16*)(save + 0x6A8); break;
	case 38: frame = *(s16*)(save + 0x6BC); break;
	case 39: frame = *(s16*)(save + 0x6D0); break;
	case 40: frame = *(s16*)(save + 0x6E4); break;
	case 41: frame = *(s16*)(save + 0x6F8); break;
	case 42: frame = *(s16*)(save + 0x70C); break;
	case 43: frame = *(s16*)(save + 0x720); break;
	case 44: frame = *(s16*)(save + 0x734); break;
	case 45: frame = *(s16*)(save + 0x748); break;
	case 46: frame = *(s16*)(save + 0x75C); break;
	case 47: frame = *(s16*)(save + 0x770); break;
	case 48: frame = *(s16*)(save + 0x784); break;
	case 49: frame = *(s16*)(save + 0x798); break;
	case 50: frame = *(s16*)(save + 0x7AC); break;
	case 51: frame = *(s16*)(save + 0x7C0); break;
	case 52: frame = *(s16*)(save + 0x7D4); break;
	case 53: frame = *(s16*)(save + 0x7E8); break;
	case 54: frame = *(s16*)(save + 0x7FC); break;
	case 55: frame = *(s16*)(save + 0x810); break;
	case 56: frame = *(s16*)(save + 0x824); break;
	case 57: frame = *(s16*)(save + 0x838); break;
	case 58: frame = *(s16*)(save + 0x84C); break;
	case 59: frame = *(s16*)(save + 0x860); break;
	case 60: frame = *(s16*)(save + 0x874); break;
	case 61: frame = *(s16*)(save + 0x888); break;
	case 62: frame = *(s16*)(save + 0x89C); break;
	case 63: frame = *(s16*)(save + 0x8B0); break;
	case 64: frame = *(s16*)(save + 0x8C4); break;
	case 65: frame = *(s16*)(save + 0x8D8); break;
	case 66: frame = *(s16*)(save + 0x8EC); break;
	case 67: frame = *(s16*)(save + 0x900); break;
	case 68: frame = *(s16*)(save + 0x914); break;
	case 69: frame = *(s16*)(save + 0x928); break;
	case 70: frame = *(s16*)(save + 0x93C); break;
	case 71: frame = *(s16*)(save + 0x950); break;
	case 72: frame = *(s16*)(save + 0x964); break;
	}
	}
	return frame;
}

#pragma dont_inline off
