#include <Camera/Camera.hpp>
#include <Camera/CameraMarioData.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/JMath.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <System/MarioGamePad.hpp>

class TFireWanwanTailHit {
public:
	const JGeometry::TVec3<f32>& getHostPos() const;
};

class TStagePositionInfo : public JDrama::TNameRef {
public:
	TStagePositionInfo(const char*);

	Vec unkC;
};

class TStagePositionHolder : public JDrama::TNameRef {
public:
	TStagePositionHolder(const char*);

	virtual JDrama::TNameRef* searchF(u16, const char*);
};

extern void* gpMarioOriginal;
extern TStagePositionHolder* gpPositionHolder;

template <> f32 CLBLinearInbetween<f32>(f32, f32, f32);
template <> s16 CLBRoundf<s16>(f32);
template <> s16 CLBTwoDegreeGeneralInbetween<s16>(s16, s16, f32, f32);
template <> BOOL CLBChaseGeneralConstantSpecifySpeed<s16>(s16*, s16, s16);

static const char dummyMactorStringValue1[]
    = "\0\0\0\0\0\0\0\0\0\0\0";
static const char SMS_NO_MEMORY_MESSAGE[] = "メモリが足りません\n";
static const char MtxCalcTypeName0[]
    = "MActorMtxCalcType_Basic クラシックスケールＯＮ";
static const char MtxCalcTypeName1[]
    = "MActorMtxCalcType_Softimage クラシックスケールＯＦＦ";
static const char MtxCalcTypeName2[]
    = "MActorMtxCalcType_MotionBlend モーションブレンド";
static const char MtxCalcTypeName3[]
    = "MActorMtxCalcType_User ユーザー定義";

static const char* sPositionNameTable[] = {
	"塔カメラＡ中心", "塔カメラＢ中心", "塔カメラＣ中心",
	"塔カメラＤ中心", "塔カメラＥ中心",
};

#pragma dont_inline on
template <> s16 CLBEaseInInbetween<s16>(s16 a, s16 b, f32 ratio)
{
	return CLBTwoDegreeGeneralInbetween<s16>(a, b, ratio, (f32)(b - a));
}
#pragma dont_inline off

void CPolarSubCamera::calcTowerCenterPos_(Vec* out)
{
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
	unk250 = 0.0f;

	if (unk7C == 0) {
		TCameraMarioData* mario = gpCameraMario;
		mCurrentTarget.mTarget.x                 = mario->mPosX;
		mCurrentTarget.mTarget.y                 = mario->mPosY;
		mCurrentTarget.mTarget.z                 = mario->mPosZ;
	}

	if (*(u32*)((u8*)this + 0x78) == 0) {
		if (unk64 & 0x4) {
			if (!CLBChaseAngleDecrease(
			        &mCurrentTarget.mYaw, *(s16*)((u8*)this + 0x274),
			        *(s16*)((u8*)this + 0x276))) {
				unk64 &= ~0x4;
				if (unk64 & 0x8) {
					unk64 &= ~0x8;
					unk64 |= 0x10;
				}
			}
		} else if (isTowerCameraSpecifyMode(mMode)) {
			if (stickX != 0.0f) {
				rotateY_ByStickX_(stickX);
				execInvalidAutoChase_();
				unk64 |= 0x80;
			} else if (!(unk64 & 0x80) && !isMarioCrabWalk_()) {
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
			if (!SMS_IsMarioTouchGround4cm()) {
				unk250 = CLBLinearInbetween<f32>(
				    *(f32*)(*(u8**)((u8*)this + 0x68) + 0x74),
				    *(f32*)(*(u8**)((u8*)this + 0x68) + 0x78), mCurrentTarget.unk28);
			} else {
				unk250 = CLBLinearInbetween<f32>(
				    *(f32*)(*(u8**)((u8*)this + 0x68) + 0x6C),
				    *(f32*)(*(u8**)((u8*)this + 0x68) + 0x70), mCurrentTarget.unk28);
			}

			if (isMomentDefinite_()) {
				mCurrentTarget.mYaw
				    = matan(*(f32*)((u8*)this + 0xBC)
				                - mCurrentTarget.mTarget.z,
				            *(f32*)((u8*)this + 0xB4)
				                - mCurrentTarget.mTarget.x);
			} else if (!(unk64 & 0x80) && !isMarioCrabWalk_()) {
				if (isMarioAimWithGun_()
				    && !isChangeToParallelCameraByMoveBG_()
				    && !isChangeToParallelCameraCByMoveBG_()) {
					if (unk288 != 0.0f) {
						f32 ratio = CLBEaseInInbetween<f32>(0.0f, 1.0f,
						                                    mCurrentTarget.unk28);
						void* p2D4 = *(void**)((u8*)this + 0x2D4);
						s16 sf     = CLBEaseInInbetween<s16>(
                            *(s16*)((u8*)p2D4 + 0x194),
                            *(s16*)((u8*)p2D4 + 0x1A8), ratio);
						CLBChaseAngleDecrease(&mCurrentTarget.mYaw, *gpMarioAngleY - 0x8000,
						                      sf);
					}
				} else {
					s16 sVar9 = *gpMarioAngleY - 0x8000;
					f32 f29;
					f32 f30;

					switch (mMode) {
					case CAMERA_MODE_DIVING:
					case CAMERA_MODE_HOVERING: {
						s16 diff = sVar9 - unk258;
						f30 = (f32)(diff >= 0 ? diff : -diff)
						      * (2.0f / 65536.0f);
						break;
					}
					default:
						f30 = (1.0f
						       - JMASCos((*gpMarioAngleY - 0x8000 - unk258) * 2))
						      * 0.5f;
					}

					f29 = 1.0f;
					if (*(s16*)((u8*)this + 0x2CA) != -1) {
						f29 = CLBLinearInbetween<f32>(
						    *(f32*)(*(u8**)((u8*)this + 0x68) + 0x84),
						    *(f32*)(*(u8**)((u8*)this + 0x68) + 0x88),
						    mCurrentTarget.unk28);
					} else if ((*gpMarioFlag & 0x1) ? true : false) {
						f29 = CLBLinearInbetween<f32>(
						    *(f32*)(*(u8**)((u8*)this + 0x68) + 0x7C),
						    *(f32*)(*(u8**)((u8*)this + 0x68) + 0x80),
						    mCurrentTarget.unk28);
					}

					int uVar1 = pad->mCompSPos[2];
					if (uVar1 & 0xff) {
						f29 *= CLBLinearInbetween<f32>(
						    *(f32*)(*(u8**)((u8*)this + 0x68) + 0x8C),
						    *(f32*)(*(u8**)((u8*)this + 0x68) + 0x90),
						    mCurrentTarget.unk28);
					}

					f32 fVar4;
					switch (mMode) {
					case CAMERA_MODE_DIVING:
					case CAMERA_MODE_HOVERING:
						fVar4 = 100.0f;
						break;
					default:
						fVar4 = gpCameraMario->mDistXZ;
						break;
					}

					f32 speed = unk250 * f30 * f29 * fVar4 * unk288;
					if (speed > 32766.998f)
						speed = 32766.998f;
					CLBChaseGeneralConstantSpecifySpeed<s16>(
					    &mCurrentTarget.mYaw, sVar9, CLBRoundf<s16>(speed));
				}
			}
		}

		rotateX_ByStickY_(stickY);
	}

	calcPosAndAt_();
}
