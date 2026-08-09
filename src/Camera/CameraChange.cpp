#define MSCLAMP_OUT_OF_LINE
#include <Camera/Camera.hpp>
#include <Camera/CameraInbetween.hpp>
#include <Camera/CameraKindParam.hpp>
#include <Camera/CameraMapTool.hpp>
#include <Camera/CameraMarioData.hpp>
#include <Camera/cameralib.hpp>
#include <Enemy/BossGesso.hpp>
#include <MarioUtil/MapUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MSound/MSSetSound.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <Map/MapData.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/MarioMain.hpp>
#include <Strategic/HitActor.hpp>
#include <Strategic/TakeActor.hpp>
#include <System/MarDirector.hpp>
#include <System/MarioGamePad.hpp>
#include <System/StageUtil.hpp>

template <> f32 CLBCalcRatio<s16>(s16, s16, s16);

#pragma dont_inline on

// Fabricated minimal TTargetCamera class to trigger MWCC emission of
// __as__13TTargetCameraFRC13TTargetCamera (weak operator=). Layout
// matches the disassembly at 0x80352670 (size = 0x34).
class TTargetCamera {
public:
	Vec mPos;   // 0x00
	Vec mTgt;   // 0x0C
	Vec mUp;    // 0x18
	s16 unk24;  // 0x24
	s16 unk26;  // 0x26
	f32 unk28;  // 0x28
	s16 unk2C;  // 0x2C
	f32 unk30;  // 0x30

	TTargetCamera& operator=(const TTargetCamera& other);
};

TTargetCamera& TTargetCamera::operator=(const TTargetCamera& other)
{
	mPos  = other.mPos;
	mTgt  = other.mTgt;
	mUp   = other.mUp;
	unk24 = other.unk24;
	unk26 = other.unk26;
	unk28 = other.unk28;
	unk2C = other.unk2C;
	unk30 = other.unk30;
	return *this;
}

template <> f32 MsClamp<f32>(f32 t, f32 l, f32 r)
{
	if (t > r)
		return r;
	if (t < l)
		return l;
	return t;
}

void CPolarSubCamera::execCameraModeChangeProc_(int mode)
{
	if (SMS_isMultiPlayerMap()) {
		s16 frame = (s16)getCameraInbetweenFrame_(2);
		*(TCameraMapTool**)((u8*)this + 0x74) = unk70;
		unk70 = nullptr;
		changeCamModeSub_(2, frame, false);
		return;
	}

	if (SMS_GetMarioStatus() == 0x800447) {
		s16 frame = (s16)getCameraInbetweenFrame_(0x2E);
		*(TCameraMapTool**)((u8*)this + 0x74) = unk70;
		unk70 = nullptr;
		changeCamModeSub_(0x2E, frame, false);
		return;
	}

	if (isFixCameraSpecifyMode(mode) || isDefiniteCameraSpecifyMode(mode))
		return;

	if (unk64 & 0x20)
		execNoticeOnOffProc_((EnumNoticeOnOffMode)1);

	int prevMode = mMode;

	if (gpMarioOriginal->mAction == 0xC400202
	    || gpMarioOriginal->mAction == 0xC000203 || SMS_CheckMarioFlag(2)
	    || (SMS_GetMarioStatus() & 0x10000)
	    || gpCameraMario->isMarioRocketing()
	    || gpMarioOriginal->checkFlag(MARIO_FLAG_FLUDD_EMITTING)
	    || gpCameraMario->isMarioClimb(SMS_GetMarioStatus())) {
		if (isLButtonCameraSpecifyMode(mMode))
			doLButtonCameraOff_(true);
		if (unk120->checkFrameMeaning(0x8000))
			execFrontRotate_();
	} else {
		if (isLButtonCameraSpecifyMode(mMode)) {
			if (SMS_GetMarioStatus() & 0x20000) {
				doLButtonCameraOff_(true);
			} else if (!isLButtonCameraInbetween()
			           && unk120->checkFrameMeaning(0x14000)
			           && unk282 == 0) {
				doLButtonCameraOff_(false);
			}
		} else if (isNormalCameraSpecifyMode(mMode)
		           || isTowerCameraSpecifyMode(mMode)) {
			if (unk64 & 0x10) {
				unk64 &= ~0x10;
				unk282 = 0x3C;
				if (gpMSound->gateCheck(0x4824))
					gpMSound->startSoundSystemSE(0x4824, 0, nullptr, 0);
				changeCamModeSpecifyFrame_(7, getCameraInbetweenFrame_(7));
			} else if (unk120->checkFrameMeaning(0xC000)) {
				bool doCheck = true;
				if (unk120->checkFrameMeaning(0x4000)) {
					if (unk282 != 0)
						doCheck = false;
					else
						execNoticeOnOffProc_((EnumNoticeOnOffMode)2);
				}
				if (doCheck) {
					if (unk64 & 0x20) {
						unk282 = 0x3C;
						if (gpMSound->gateCheck(0x4824))
							gpMSound->startSoundSystemSE(0x4824, 0, nullptr,
							                             0);
						changeCamModeSpecifyFrame_(7,
						                           getCameraInbetweenFrame_(7));
					} else if (!isLButtonCameraInbetween()) {
						execFrontRotate_();
					}
				}
			}
		}
	}

	if (prevMode != mMode)
		return;
	if (isLButtonCameraSpecifyMode(mMode))
		return;

	u32 status     = SMS_GetMarioStatus();
	u32 prevStatus = gpMarioOriginal->mPrevAction;
	u8  stage      = gpMarDirector->getCurrentMap();

	int newMode;
	if (gpMarioOriginal->checkFlag(MARIO_FLAG_HELMET_FLW_CAMERA)) {
		newMode = 0x2B;
	} else if (SMS_CheckMarioFlag(2)) {
		if (stage == 9)
			newMode = 0x8;
		else
			newMode = 0xD;
	} else {
		if (gpMarioOriginal->checkFlag(MARIO_FLAG_FLUDD_EMITTING)) {
			if (stage != 7
			    && ((status & 0x2000)
			        || ((prevStatus & 0x2000) && (status & 0x800))))
				newMode = 0x44;
			else
				newMode = 0x2C;
		} else if (stage != 7 && (status & 0x2000)) {
			newMode = 0x31;
		} else if (SMS_IsMarioOnWire()
		           && (status == 0x10000554 || status == 0x10000357
		               || status == 0x10000358)) {
			newMode = 0x10;
		} else if (SMS_IsMarioOnWire() || status == 0x892
		           || (prevStatus == 0x892 && status == 0x8008A9)) {
			newMode = 0x6;
		} else if (status & 0x10000) {
			newMode = 0x30;
		} else if ((status & 0x20000000)
		           && gpMarioOriginal->mHeldObject != nullptr
		           && gpMarioOriginal->mHeldObject->getActorType()
		                  == 0x4000006C) {
			newMode = 0x34;
		} else {
			bool isFence = false;
			switch (SMS_GetMarioStatus()) {
			case 0x30000569:
			case 0x38000368:
			case 0x3000036A:
			case 0x3000036B:
			case 0x3000036C:
				isFence = true;
				break;
			}
			if (isFence) {
				if (stage == 8)
					newMode = 0x3D;
				else
					newMode = 0x3C;
			} else if (gpCameraMario->isMarioSlider()) {
				newMode = 0x2A;
			} else if (gpCameraMario->isMarioLeanMirror()) {
				newMode = 0xB;
			} else if (gpCameraMario->isMarioIndoor()) {
				if (mode >= 0 && mode < 0x49)
					newMode = mode;
				else if (stage == 7)
					newMode = 0x14;
				else
					newMode = 0xE;
			} else if (gpCameraMario->isMarioRocketing() && mode != 0x41) {
				if (*(u8*)((u8*)SMS_GetMarioWaterGun() + 0x1C84) == 4)
					newMode = 0x12;
				else
					newMode = 0x5;
			} else if ((status & 0x200000) && status != 0x200345) {
				if (stage == 8)
					newMode = 0x3E;
				else
					newMode = 0xF;
			} else if (isChangeToBossGesoCamera_()) {
				newMode = 0x39;
			} else {
				bool isCancan = false;
				if (gpMarioOriginal->mHolder != nullptr
				    && gpMarioOriginal->mHolder->getActorType() == 0x10000028)
					isCancan = true;
				if (isCancan) {
					newMode = 0x43;
				} else {
					bool onPlatform = false;
					if (SMS_GetGroundActor(SMS_GetMarioGrPlane(), 0x400002C9))
						onPlatform = true;
					if (onPlatform) {
						newMode = 0x11;
					} else if (stage != 7 && status == 0x884) {
						newMode = 0x13;
					} else if (isChangeToParallelCameraByMoveBG_()) {
						newMode = 1;
					} else {
						bool onPlatformC = false;
						if (SMS_GetGroundActor(SMS_GetMarioGrPlane(),
						                       0x4000012F))
							onPlatformC = true;
						if (onPlatformC) {
							newMode = 0x47;
						} else if (status == 0x8008A9) {
							if (isOverHipAttackSpecifyMode(mode)) {
								newMode = mode;
							} else {
								bool exMap = false;
								if (SMS_isExMap()) {
									switch (stage) {
									case 0x1D:
									case 0x1E:
										break;
									default:
										exMap = true;
										break;
									}
								}
								if (exMap)
									newMode = 0x26;
								else
									newMode = 4;
							}
						} else if (mode >= 0 && mode < 0x49
						           && !isFollowCameraSpecifyMode(mode)) {
							newMode = mode;
						} else if (gpCameraMario->isMarioClimb(status)) {
							newMode = 0x15;
						} else {
							switch (status) {
							case 0x200886:
							case 0x8A7:
								if (gpCameraMario->isMarioClimb(prevStatus)
								    && (mMode == 0x15 || mPrevMode == 0x15)) {
									newMode = 0x32;
								} else {
									bool exMap = false;
									if (SMS_isExMap()) {
										switch (stage) {
										case 0x1D:
										case 0x1E:
											break;
										default:
											exMap = true;
											break;
										}
									}
									if (exMap)
										newMode = 0x26;
									else
										newMode = 3;
								}
								break;

							default:
								bool exMap = false;
								if (SMS_isExMap()) {
									switch (stage) {
									case 0x1D:
									case 0x1E:
										break;
									default:
										exMap = true;
										break;
									}
								}
								if (exMap)
									newMode = 0x26;
								else if (isFollowCameraSpecifyMode(mode))
									newMode = mode;
								else
									newMode = 0;
								break;
							}
						}
					}
				}
			}
		}
	}

	changeCamModeSpecifyFrame_(newMode, getCameraInbetweenFrame_(newMode));
}

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

	unk64 &= ~0x10;
	unk64 |= 0x4;

	*(s16*)((u8*)this + 0x274) = *gpMarioAngleY + (s16)0x8000;

	u32 m = unk120->mEnabledFrameMeaning;
	if (m & 0x4000) {
		s16 v = *(s16*)((u8*)*(void**)((u8*)this + 0x2D4) + 0x11C);
		*(s16*)((u8*)this + 0x276) = v;
		unk64 |= 0x8;
	} else if (m & 0x8000) {
		s16 v = *(s16*)((u8*)*(void**)((u8*)this + 0x2D4) + 0x130);
		*(s16*)((u8*)this + 0x276) = v;
		unk64 &= ~0x8;
		gpMSound->startSoundSystemSE(0x4826, 0, nullptr, 0);
	}
}

void CPolarSubCamera::changeCamModeSpecifyCamMapToolAndFrame_(
    const TCameraMapTool* tool, int frame)
{
	int newMode = tool->mCameraMode;
	if (mMode == newMode && unk70 == tool) {
		return;
	}
	*(TCameraMapTool**)((u8*)this + 0x74) = unk70;
	unk70 = const_cast<TCameraMapTool*>(tool);
	changeCamModeSub_(newMode, frame, true);
}

void CPolarSubCamera::changeCamModeSpecifyCamMapTool_(const TCameraMapTool* tool)
{
	int newMode = tool->mCameraMode;
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
	bool wasMinusOne = false;
	if (newMode == -1) {
		u8* hist = *(u8**)((u8*)this + 0x60);
		int top  = *(int*)(hist + 0x4);
		int* p;
		if (top <= 0) {
			p = *(int**)(hist + 0x8);
		} else {
			p = *(int**)(hist + 0x8) + (top - 1);
		}
		newMode     = *p;
		wasMinusOne = true;
	}

	if (!flag) {
		if (mMode == newMode)
			return;
	}
	if (frame < 0)
		return;
	if (frame == 0)
		frame = 1;

	*(int*)((u8*)this + 0x54) = mMode;
	if (wasMinusOne) {
		u8* hist = *(u8**)((u8*)this + 0x60);
		int top  = *(int*)(hist + 0x4);
		if (top > 0) {
			*(int*)(hist + 0x4) = top - 1;
		}
	} else {
		u8* hist = *(u8**)((u8*)this + 0x60);
		int top  = *(int*)(hist + 0x4);
		int cap  = *(int*)(hist + 0x0);
		if (top >= cap) {
			int i = 0;
			int off = 0;
			while (i < *(int*)(hist + 0x0) - 1) {
				int* base = *(int**)(hist + 0x8);
				u8* slot  = (u8*)base + off;
				*(int*)slot = *(int*)(slot + 4);
				++i;
				off += 4;
			}
			int* base = *(int**)(hist + 0x8);
			int cap2  = *(int*)(hist + 0x0);
			*(int*)((u8*)base + cap2 * 4 - 4) = mMode;
		} else {
			int* entries          = *(int**)(hist + 0x8);
			entries[top]          = mMode;
			*(int*)(hist + 0x4)   = *(int*)(hist + 0x4) + 1;
		}
	}

	if (newMode < 0x49) {
		int curMode = mMode;
		if (curMode == 0x14 && newMode == 0x42) {
			*(f32*)((u8*)this + 0xA8) = 0.0f;
			*(f32*)((u8*)this + 0xDC) = 0.0f;
		} else if (curMode == 0x33 && newMode == 0x3E) {
			*(f32*)((u8*)this + 0xA8) = 0.0f;
			*(f32*)((u8*)this + 0xDC) = 0.0f;
		} else if (!isLButtonCameraSpecifyMode(curMode)) {
			if (isLButtonCameraSpecifyMode(newMode)) {
				*(f32*)((u8*)this + 0xB0)
				    = *(f32*)((u8*)this + 0xA8);

				TCameraKindParam buf;
				u8* p = (u8*)this + newMode * 4;
				buf.copySaveParam(*(const TCamSaveKindParam*)
				    *(TCameraKindParam**)(p + 0x2D8));

				f32 ratio
				    = CLBCalcRatio<s16>(buf.unk18, buf.unk1A, -buf.unk58);
				f32 v = MsClamp<f32>(ratio, 0.0f, 1.0f);
				*(f32*)((u8*)this + 0xA8) = v;
				*(f32*)((u8*)this + 0xDC) = v;
				unk120->onNeutralMarioKey();
			}
		} else {
			if (!isLButtonCameraSpecifyMode(newMode)) {
				f32 v = *(f32*)((u8*)this + 0xB0);
				*(f32*)((u8*)this + 0xA8) = v;
				*(f32*)((u8*)this + 0xDC) = v;
				unk120->onNeutralMarioKey();
			}
		}

		(*(TCameraInbetween**)((u8*)this + 0x6C))->startCameraInbetween(frame);
	}

	mMode       = newMode;
	int curMode = mMode;
	if (curMode < 0x49 && *(int*)((u8*)this + 0x54) < 0x49) {
		bool isFixOrDefNew = true;
		bool isFixOrDefOld = true;
		bool fixNewWithTool = false;

		if (!isFixCameraSpecifyMode(curMode)
		    && !isDefiniteCameraSpecifyMode(curMode)) {
			isFixOrDefNew = false;
		}
		if (isFixOrDefNew && unk70 != nullptr) {
			fixNewWithTool = true;
		}

		int prevMode = *(int*)((u8*)this + 0x54);
		if (!isFixCameraSpecifyMode(prevMode)
		    && !isDefiniteCameraSpecifyMode(prevMode)) {
			isFixOrDefOld = false;
		}

		if (!isFixOrDefOld && fixNewWithTool) {
			*(int*)((u8*)this + 0xE8) = *(int*)((u8*)this + 0x80);
			*(int*)((u8*)this + 0xEC) = *(int*)((u8*)this + 0x84);
			*(int*)((u8*)this + 0xF0) = *(int*)((u8*)this + 0x88);
			*(int*)((u8*)this + 0xF4) = *(int*)((u8*)this + 0x8C);
			*(int*)((u8*)this + 0xF8) = *(int*)((u8*)this + 0x90);
			*(int*)((u8*)this + 0xFC) = *(int*)((u8*)this + 0x94);
			*(int*)((u8*)this + 0x100) = *(int*)((u8*)this + 0x98);
			*(int*)((u8*)this + 0x104) = *(int*)((u8*)this + 0x9C);
			*(int*)((u8*)this + 0x108) = *(int*)((u8*)this + 0xA0);
			*(s16*)((u8*)this + 0x10C) = *(s16*)((u8*)this + 0xA4);
			*(s16*)((u8*)this + 0x10E) = *(s16*)((u8*)this + 0xA6);
			*(f32*)((u8*)this + 0x110) = *(f32*)((u8*)this + 0xA8);
			*(s16*)((u8*)this + 0x114) = *(s16*)((u8*)this + 0xAC);
			*(f32*)((u8*)this + 0x118) = *(f32*)((u8*)this + 0xB0);
			*(int*)((u8*)this + 0x11C) = *(int*)((u8*)unk70 + 0x28);
		}

		if (isFixOrDefOld) {
			if (*(int*)((u8*)this + 0x11C) & 1) {
				TTargetCamera& dst = *(TTargetCamera*)((u8*)this + 0x80);
				dst = *(TTargetCamera*)((u8*)this + 0xE8);

				*(Vec*)((u8*)this + 0xB4) = *(Vec*)&dst.mPos;
				*(Vec*)((u8*)this + 0xC0) = *(Vec*)&dst.mTgt;
				*(Vec*)((u8*)this + 0xCC) = *(Vec*)&dst.mUp;
				*(s16*)((u8*)this + 0xD8) = dst.unk24;
				*(s16*)((u8*)this + 0xDA) = dst.unk26;
				*(f32*)((u8*)this + 0xDC) = dst.unk28;
				*(s16*)((u8*)this + 0xE0) = dst.unk2C;
				*(f32*)((u8*)this + 0xE4) = dst.unk30;
				killHeightPan_();
			} else {
				calcNowTargetFromPosAndAt_(*(const Vec*)((u8*)this + 0x10),
				    *(const Vec*)((u8*)this + 0x3C));
			}
		}

		int sw = *(int*)((u8*)this + 0x54) - 9;
		if ((unsigned int)sw <= 0x32) {
			switch (sw) {
			case 0:  /* prevMode 9 */
			case 14: /* 0x17 */
			case 16: /* 0x19 */
			case 22: /* 0x1F */
			case 24: /* 0x21 */
				warpPosAndAt(*(f32*)((u8*)this + 0xA8),
				    *(s16*)((u8*)this + 0xA6));
				break;
			case 20: /* 0x1D */
			case 28: /* 0x25 */
				*(s16*)((u8*)this + 0xA6)
				    = *gpMarioAngleY + (s16)0x8000;
				break;
			case 49: /* 0x3A */
			case 50: /* 0x3B */
				*(s16*)((u8*)this + 0xA6)
				    = *gpMarioAngleY + (s16)0x8000;
				warpPosAndAt(*(f32*)((u8*)this + 0xA8),
				    *(s16*)((u8*)this + 0xA6));
				break;
			}
		}

		if (fixNewWithTool) {
			TCameraMapTool* tool = unk70;
			u32 flagBit          = *(u32*)((u8*)tool + 0x28) & 0x2;
			bool useVecB         = (flagBit != 0);
			int m                = mMode;
			bool useBlock1;
			if (m >= 0x1E) {
				useBlock1 = (m < 0x20);
			} else if (m >= 0x18) {
				useBlock1 = false;
			} else if (m >= 0x16) {
				useBlock1 = true;
			} else {
				useBlock1 = false;
			}

			Vec tmp;
			if (useBlock1) {
				if (useVecB) {
					tmp = *(Vec*)((u8*)this + 0x80);
				}
				tool->calcPosAndAt(
				    (JGeometry::TVec3<f32>*)((u8*)this + 0x80),
				    (JGeometry::TVec3<f32>*)((u8*)this + 0x8C));
				if (useVecB) {
					*(f32*)((u8*)this + 0x80) = tmp.x;
					*(f32*)((u8*)this + 0x84) = tmp.y;
					*(f32*)((u8*)this + 0x88) = tmp.z;
				}
			} else {
				if (useVecB) {
					tmp = *(Vec*)((u8*)this + 0x10);
				}
				tool->calcPosAndAt(
				    (JGeometry::TVec3<f32>*)((u8*)this + 0x10),
				    (JGeometry::TVec3<f32>*)((u8*)this + 0x3C));
				if (useVecB) {
					*(f32*)((u8*)this + 0x10) = tmp.x;
					*(f32*)((u8*)this + 0x14) = tmp.y;
					*(f32*)((u8*)this + 0x18) = tmp.z;
				}
				warpPosAndAt(*(const Vec*)((u8*)this + 0x10),
				    *(const Vec*)((u8*)this + 0x3C));
			}
		}

		*(int*)((u8*)this + 0x78) = 0;
		*(int*)((u8*)this + 0x7C) = 0;
		int pm                    = *(int*)((u8*)this + 0x54);
		if (pm == 0x1C || pm == 0x24) {
			onMoveApproach_();
		} else {
			offMoveApproach_();
		}
	}

	if (!isNormalCameraSpecifyMode(mMode)
	    && !isTowerCameraSpecifyMode(mMode)) {
		*(u16*)((u8*)this + 0x64) &= ~0x1C;
	}

	int pm = *(int*)((u8*)this + 0x54);
	if (pm == 0x33 && mMode == 0x3E) {
		u16* p = (u16*)((u8*)this + 0x278);
		if (*p < 0x78)
			*p = 0x78;
	} else if (pm == 0x3E && mMode == 0x33) {
		u16* p = (u16*)((u8*)this + 0x27A);
		if (*p < 0x78)
			*p = 0x78;
	}

	killHeightPanWhenChangeCamMode_();
	{
		u8* p           = *(u8**)((u8*)this + 0x2AC);
		*(s16*)(p + 0x0) = 0;
		*(f32*)(p + 0x4) = 1.0f;
		*(f32*)(p + 0x8) = 1.0f;
		*(f32*)(p + 0xC) = 1.0f;
	}
}

void CPolarSubCamera::setUpFromLButtonCamera_()
{
	f32 v = *(f32*)((u8*)this + 0xB0);
	*(f32*)((u8*)this + 0xA8) = v;
	*(f32*)((u8*)this + 0xDC) = v;
}

void CPolarSubCamera::setUpToLButtonCamera_(int mode)
{
	*(f32*)((u8*)this + 0xB0) = mCurrentTarget.unk28;

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
	mCurrentTarget.unk28 = ratio;
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
	TCamSaveKindParam** saveTable = (TCamSaveKindParam**)((u8*)this + 0x2D8);
	u8* save = (u8*)saveTable[mode];
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
