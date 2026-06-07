#include <Camera/Camera.hpp>
#include <Camera/CameraMarioData.hpp>
#include <Camera/cameralib.hpp>
#include <Enemy/FireWanwan.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/MarioMain.hpp>
#include <Strategic/NameRefAry.hpp>
#include <System/MarioGamePad.hpp>
#include <System/PositionHolder.hpp>

template <> f32 CLBLinearInbetween<f32>(f32, f32, f32);
template <> s16 CLBRoundf<s16>(f32);
template <> s16 CLBTwoDegreeGeneralInbetween<s16>(s16, s16, f32, f32);
template <> BOOL CLBChaseGeneralConstantSpecifySpeed<s16>(s16*, s16, s16);

template <> s16 CLBEaseInInbetween<s16>(s16 a, s16 b, f32 ratio)
{
	return CLBTwoDegreeGeneralInbetween<s16>(a, b, ratio, (f32)(b - a));
}

void CPolarSubCamera::calcTowerCenterPos_(Vec* out)
{
	static const char* sPositionNameTable[] = {
		"塔カメラＡ中心", "塔カメラＢ中心", "塔カメラＣ中心",
		"塔カメラＤ中心", "塔カメラＥ中心",
	};
	const char* name = nullptr;
	switch (mMode) {
	case 0x27: name = sPositionNameTable[0]; break;
	case 0x28: name = sPositionNameTable[1]; break;
	case 0x29: name = sPositionNameTable[2]; break;
	case 0x37: name = sPositionNameTable[3]; break;
	case 0x41: name = sPositionNameTable[4]; break;
	default:
		out->x = 0.0f;
		out->y = 0.0f;
		out->z = 0.0f;
		return;
	}

	TStagePositionInfo* info = (TStagePositionInfo*)gpPositionHolder->searchF(
	    JDrama::TNameRef::calcKeyCode(name), name);
	if (info != nullptr) {
		*out = info->unkC;
	} else {
		out->x = 0.0f;
		out->y = 0.0f;
		out->z = 0.0f;
	}
}

void CPolarSubCamera::ctrlNormalOrTowerCamera_()
{
	TMarioGamePad* pad         = unk120;
	f32 stickX                 = pad->mCompSPos[6];
	f32 stickY                 = pad->mCompSPos[7];
	*(f32*)((u8*)this + 0x250) = 0.0f;

	if (unk7C == 0) {
		TCameraMarioData* mario = gpCameraMario;
		unk8C.x                 = mario->mPosX;
		unk8C.y                 = mario->mPosY;
		unk8C.z                 = mario->mPosZ;
	}

	if (*(u32*)((u8*)this + 0x78) == 0) {
		if (*(u16*)((u8*)this + 0x64) & 0x4) {
			if (!CLBChaseAngleDecrease(
			        &unkA6, *(s16*)((u8*)this + 0x274),
			        *(s16*)((u8*)this + 0x276))) {
				*(u16*)((u8*)this + 0x64) &= ~0x4;
				if (*(u16*)((u8*)this + 0x64) & 0x8) {
					*(u16*)((u8*)this + 0x64) &= ~0x8;
					*(u16*)((u8*)this + 0x64) |= 0x10;
				}
			}
		} else if (isTowerCameraSpecifyMode(mMode)) {
			if (stickX != 0.0f) {
				rotateY_ByStickX_(stickX);
				execInvalidAutoChase_();
				*(u16*)((u8*)this + 0x64) |= 0x80;
			} else if (!(*(u16*)((u8*)this + 0x64) & 0x80) && !isMarioCrabWalk_()) {
				Vec center;
				calcTowerCenterPos_(&center);
				calcNoticeTargetYrot_(center);
			}
		} else if (stickX != 0.0f) {
			rotateY_ByStickX_(stickX);
			execInvalidAutoChase_();
		} else if (mMode == 0x39) {
			if (isChangeToBossGesoCamera_()) {
				calcNoticeTargetYrot_(*(Vec*)((u8*)*(void**)((u8*)this
				                                              + 0x2A8)
				                              + 0x10));
			}
		} else if (mMode == 0x43) {
			if (isChangeToCancanCamera_()) {
				TFireWanwanTailHit* hit
				    = *(TFireWanwanTailHit**)((u8*)gpMarioOriginal + 0x6C);
				calcNoticeTargetYrot_(hit->getHostPos());
			}
		} else {
			u8* p68 = *(u8**)((u8*)this + 0x68);
			if (!SMS_IsMarioTouchGround4cm()) {
				*(f32*)((u8*)this + 0x250) = CLBLinearInbetween<f32>(
				    *(f32*)(p68 + 0x74), *(f32*)(p68 + 0x78), unkA8);
			} else {
				*(f32*)((u8*)this + 0x250) = CLBLinearInbetween<f32>(
				    *(f32*)(p68 + 0x6C), *(f32*)(p68 + 0x70), unkA8);
			}

			if (isMomentDefinite_()) {
				unkA6 = matan(*(f32*)((u8*)this + 0xBC) - unk8C.z,
				              *(f32*)((u8*)this + 0xB4) - unk8C.x);
			} else if (!(*(u16*)((u8*)this + 0x64) & 0x80) && !isMarioCrabWalk_()) {
				bool doSpeedCalc;
				if (isMarioAimWithGun_()) {
					if (isChangeToParallelCameraByMoveBG_()
					    || isChangeToParallelCameraCByMoveBG_()) {
						doSpeedCalc = true;
					} else {
						doSpeedCalc = false;
						if (*(f32*)((u8*)this + 0x288) != 0.0f) {
							f32 ratio = CLBEaseInInbetween<f32>(0.0f, 1.0f,
							                                    unkA8);
							void* p2D4 = *(void**)((u8*)this + 0x2D4);
							s16 sf     = CLBEaseInInbetween<s16>(
                                *(s16*)((u8*)p2D4 + 0x194),
                                *(s16*)((u8*)p2D4 + 0x1A8), ratio);
							CLBChaseAngleDecrease(
							    &unkA6, *gpMarioAngleY - 0x8000, sf);
						}
					}
				} else {
					doSpeedCalc = true;
				}

				if (doSpeedCalc) {
					s16 angleY = *gpMarioAngleY;
					s16 r31    = angleY - 0x8000;
					s16 cur258 = *(s16*)((u8*)this + 0x258);
					f32 f29;
					if (mMode == 0x12 || mMode == 0x2B) {
						s16 diff    = r31 - cur258;
						s16 absDiff = (diff < 0) ? -diff : diff;
						f29         = (f32)absDiff * (1.0f / 32768.0f);
					} else {
						f29 = 0.5f * (1.0f - JMASCos((r31 - cur258) * 2));
					}

					f32 f30 = 1.0f;
					if (*(s16*)((u8*)this + 0x2CA) != -1) {
						f30 = CLBLinearInbetween<f32>(*(f32*)(p68 + 0x84),
						                              *(f32*)(p68 + 0x88),
						                              unkA8);
					} else if (*gpMarioFlag & 0x1) {
						f30 = CLBLinearInbetween<f32>(*(f32*)(p68 + 0x7C),
						                              *(f32*)(p68 + 0x80),
						                              unkA8);
					}

					if ((s32)pad->mCompSPos[2] != 0) {
						f30 *= CLBLinearInbetween<f32>(*(f32*)(p68 + 0x8C),
						                               *(f32*)(p68 + 0x90),
						                               unkA8);
					}

					f32 fMult;
					if (mMode == 0x12 || mMode == 0x2B) {
						fMult = 100.0f;
					} else {
						fMult = gpCameraMario->mDistXZ;
					}

					f32 speed
					    = *(f32*)((u8*)this + 0x288)
					      * (fMult
					         * (f30
					            * (*(f32*)((u8*)this + 0x250) * f29)));
					if (speed > 32766.998f)
						speed = 32766.998f;
					s16 delta = CLBRoundf<s16>(speed);
					CLBChaseGeneralConstantSpecifySpeed<s16>(&unkA6, r31,
					                                         delta);
				}
			}
		}

		rotateX_ByStickY_(stickY);
	}

	calcPosAndAt_();
}
