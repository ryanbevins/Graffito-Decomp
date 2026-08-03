#define JGEOMETRY_CAMERAGC_OWNER_HELPERS
#define MSCLAMP_OUT_OF_LINE
#define MS_SQRTF_OUT_OF_LINE

// cameragc.cpp -- main CPolarSubCamera implementation. Large TU (~12KB).
// Stub for portability; key functions need full decomp:
// - perform, ctrlGameCamera_, calcPosAndAt_, calcFinalPosAndAt_,
//   calcNowTargetFromPosAndAt_, calcSlopeAngleX_, rotateX/Y_ByStickXY_,
//   __ct__, ~CPolarSubCamera, loadAfter

#include <Camera/Camera.hpp>
#include <Camera/CameraBck.hpp>
#include <Camera/CameraInbetween.hpp>
#include <Camera/CameraKindParam.hpp>
#include <Camera/CameraMapTool.hpp>
#include <Camera/CameraMarioData.hpp>
#include <Camera/CameraOption.hpp>
#include <Camera/CameraShake.hpp>
#include <Camera/cameralib.hpp>
#include <System/MarDirector.hpp>
#include <System/StageUtil.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JMath.hpp>
#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <Player/MarioMain.hpp>
#include <Player/MarioAccess.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <System/MarioGamePad.hpp>
#include <dolphin/gx.h>
#include <stdio.h>

#undef JGEOMETRY_CAMERAGC_OWNER_HELPERS
#undef MSCLAMP_OUT_OF_LINE

template <> f32 CLBLinearInbetween<f32>(f32, f32, f32);
template <> f32 CLBCalcRatio<s16>(s16, s16, s16);
template <> f32 CLBCalcRatio<f32>(f32, f32, f32);
template <> f32 CLBCalcRatio<long>(long, long, long);
template <> s16 CLBRoundf<s16>(f32);
template <> BOOL CLBChaseGeneralConstantSpecifySpeed<s16>(s16*, s16, s16);
template <> f32 CLBEaseInInbetween<f32>(f32, f32, f32);
template <> f32 CLBTwoDegreeGeneralInbetween<f32>(f32, f32, f32, f32);

static const char cDirtyFileName[] = "/scene/map/pollution/H_ma_rak.bti";
static const char cDirtyTexName[]  = "H_ma_rak_dummy";

static const char* cCameraBckNameShineGetInside
    = "/common/camera/camera_demo_shine_get_inside";
static const char* cCameraBckNameShineGetOutside
    = "/common/camera/camera_demo_shine_get_outside";
static const char* cCameraBckNameGate = "/common/camera/camera_demo_gate_in";

CPolarSubCamera* gpCamera;

const char* cStartCamName
    = "\x8A\x4A\x8E\x6E\x83\x4A\x83\x81\x83\x89";
const char* cStartAfterCamName
    = "\x8A\x4A\x8E\x6E\x8C\xE3\x83\x4A\x83\x81\x83\x89";
const char* cJetCoasterCam0BckName  = "pinna2_camera";
const char* cJetCoasterCam1BckName  = "tinkoopa_camera";
const char* cJetCoasterDemoBckName  = "tinkoopa_killer_camera";
const char* cStartCamBckFileName    = "/scene/map/camera/StartCamera.bck";

class TCamSaveKindParam {
public:
	TCamSaveKindParam(const char*);

private:
	u8 _pad[0x968];
};

class TCamSaveEx : public TParams {
public:
	TCamSaveEx();

public:
	u8 _pad08[0x1AC - 0x08];
	TParamRT<s16> mSLLimitMinAngleX;
	TParamRT<s16> mSLLimitMaxAngleX;
	TParamRT<s16> mSLSlopeMaxAngleX;
	TParamRT<s16> mSLSlopeSpeedAngleX;
	TParamRT<f32> mSLSlopeForwardDistXZ;
};

class TCamSaveNotice {
public:
	TCamSaveNotice();

private:
	u8 _pad[0xBC];
};

class TCameraJetCoaster {
public:
	TCameraJetCoaster();

private:
	u8 _pad[0x3C];
};

class TCameraPtrArray {
public:
	TCameraPtrArray()
	    : mCapacity(4)
	    , unk4(nullptr)
	    , mStorage(nullptr)
	{
		mStorage = new void*[mCapacity];
	}

private:
	s32 mCapacity;
	void* unk4;
	void** mStorage;
};

class TCameraOffsetState {
public:
	TCameraOffsetState()
	    : unk0(0)
	    , unk4(0.0f)
	    , unk8(0.0f)
	    , unkC(0.0f)
	{
	}

public:
	s16 unk0;
	u8 _pad[2];
	f32 unk4;
	f32 unk8;
	f32 unkC;
};

class TCameraLookatSave {
public:
	TCameraLookatSave()
	    : unk0(nullptr)
	    , unk4(0.0f)
	    , unk8(nullptr)
	    , unkC(0)
	    , unk10(nullptr)
	    , unk14(nullptr)
	{
	}

private:
	void* unk0;
	f32 unk4;
	void* unk8;
	u8 unkC;
	u8 _pad[3];
	void* unk10;
	void* unk14;
};

CPolarSubCamera::CPolarSubCamera(const char* name)
    : JDrama::TLookAtCamera(CLBConstUpVec, name)
{
	mMode = -1;
	unk54 = -1;
	unk58 = -1;
	unk5C = -1;
	unk60 = nullptr;
	unk64 = 0;
	unk70 = nullptr;
	unk74 = nullptr;
	unk78 = 0;
	unk7C = 0;
	unk11C = 0;
	unk120 = nullptr;
	unk24C = 0.0f;
	unk250 = 0.0f;
	unk254 = 0;
	unk268 = 0.0f;
	unk26C = 1.0f;
	*(u16*)((u8*)this + 0x274) = 0;
	*(u16*)((u8*)this + 0x276) = 0;
	unk278 = 0;
	unk27A = 0;
	*(u16*)((u8*)this + 0x27C) = 0;
	*(u16*)((u8*)this + 0x27E) = 0;
	*(u16*)((u8*)this + 0x280) = 0;
	unk282 = 0;
	unk284 = 0;
	unk288 = 1.0f;
	unk28C = 0;
	*(u16*)((u8*)this + 0x28E) = 0;
	unk290 = 0.0f;
	unk294 = 0.0f;
	unk298 = 0.0f;
	unk2AC = nullptr;
	unk2B0 = nullptr;
	unk2B4 = nullptr;
	unk2B8 = nullptr;
	unk2BC = nullptr;
	unk2C0 = 3.0f;
	unk2C4 = 2.0f;

	gpCamera = this;
	gpCameraShake = new TCameraShake();
	gpCameraMario = new TCameraMarioData();
	gpCameraOption = nullptr;
	unk60 = new TCameraPtrArray();
	unk68 = new TCameraKindParam();
	unk6C = new TCameraInbetween();
	unk2AC = new TCameraOffsetState();
	unk2B0 = new TCameraBck();
	unk2B4 = new TCameraLookatSave();
	unk2D0 = new TCamSaveNotice();
	unk2D4 = new TCamSaveEx();

	for (int i = 0; i < 0x49; ++i)
		unk2D8[i] = new TCamSaveKindParam(mCamKindNameSaveFile[i]);

	if (SMS_isMultiPlayerMap())
		createMultiPlayer(4);

	if (gpMarDirector->mMap == 0x3A
	    && (gpMarDirector->unk7D == 0 || gpMarDirector->unk7D == 1)) {
		unk64 |= 0x1000;
		unk2B8 = new TCameraJetCoaster();
		if (gpMarDirector->unk7D == 0)
			((TCameraBck*)unk2B0)->startDemo(cJetCoasterCam0BckName,
			                                 nullptr);
	}

	unk2CA = -1;
	unk2C8 = -1;
	unk2CC = 0;
}

#pragma dont_inline on
template <> f32 CLBEaseInInbetween<f32>(f32 a, f32 b, f32 t)
{
	return CLBTwoDegreeGeneralInbetween<f32>(a, b, t, b - a);
}

template <>
f32 CLBTwoDegreeGeneralInbetween<f32>(f32 a, f32 b, f32 t, f32 ba)
{
	f32 diff = b - a;
	f32 tba  = ba * t;
	f32 ease = t * (diff - ba);
	return t * tba + ease + a;
}
#pragma dont_inline off

CPolarSubCamera::~CPolarSubCamera() { }

void CPolarSubCamera::perform(u32 flags, JDrama::TGraphics* gfx)
{
	if (flags & 1) {
		if (gfx->unk0 & 1) {
			unk13C.x = unk124.x;
			unk13C.y = unk124.y;
			unk13C.z = unk124.z;
			unk160.x = unk148.x;
			unk160.y = unk148.y;
			unk160.z = unk148.z;
			unk1AC[0][0] = unk16C[0][0];
			unk1AC[0][1] = unk16C[0][1];
			unk1AC[0][2] = unk16C[0][2];
			unk1AC[0][3] = unk16C[0][3];
			unk1AC[1][0] = unk16C[1][0];
			unk1AC[1][1] = unk16C[1][1];
			unk1AC[1][2] = unk16C[1][2];
			unk1AC[1][3] = unk16C[1][3];
			unk1AC[2][0] = unk16C[2][0];
			unk1AC[2][1] = unk16C[2][1];
			unk1AC[2][2] = unk16C[2][2];
			unk1AC[2][3] = unk16C[2][3];
			unk1AC[3][0] = unk16C[3][0];
			unk1AC[3][1] = unk16C[3][1];
			unk1AC[3][2] = unk16C[3][2];
			unk1AC[3][3] = unk16C[3][3];
			PSMTXCopy(unk1EC, unk21C);
		}

		mUp.x  = CLBConstUpVec.x;
		mUp.y  = CLBConstUpVec.y;
		mUp.z  = CLBConstUpVec.z;
		unk254 = 0;

		if (mMode != 0x49) {
			if (SMS_isOptionMap())
				ctrlOptionCamera_();
			else
				ctrlGameCamera_();

			calcFinalPosAndAt_();

			fabricatedInline2();
		}

		if (mMode != 0x49) {
			C_MTXPerspective(unk16C, mFovy, mAspect, mNear, mFar);
			C_MTXLookAt(unk1EC, &unk124, &mUp, &unk148);
		}

		bool updateDemo = false;
		if (gfx->unk0 & 2)
			updateDemo = true;

		if (updateDemo) {
			if (mMode != 0x49 && !(unk64 & 0x400)) {
				if (unk64 & 0x200) {
					updateGateDemoCamera_();
				} else if (unk64 & 0x1000) {
					u8* coaster = (u8*)unk2B8;
					f32* frame  = (f32*)(coaster + 0x34);
					JGeometry::TVec3<f32>* up
					    = (JGeometry::TVec3<f32>*)(coaster + 0x28);
					JGeometry::TVec3<f32>* at
					    = (JGeometry::TVec3<f32>*)(coaster + 0x1C);
					JGeometry::TVec3<f32>* pos
					    = (JGeometry::TVec3<f32>*)(coaster + 0x10);
					((TCameraBck*)unk2B0)->updateDemo(pos, at, up, frame);
				}
			}

			if (!SMS_isOptionMap() && mMode != 0x2E && mMode != 0x49)
				calcInHouseNo_(false);
		}

		updateDemoCamera_(updateDemo);
	}

	if (flags & 0x14) {
		gfx->mProjMtx.mMtx[0][0] = unk16C[0][0];
		gfx->mProjMtx.mMtx[0][1] = unk16C[0][1];
		gfx->mProjMtx.mMtx[0][2] = unk16C[0][2];
		gfx->mProjMtx.mMtx[0][3] = unk16C[0][3];
		gfx->mProjMtx.mMtx[1][0] = unk16C[1][0];
		gfx->mProjMtx.mMtx[1][1] = unk16C[1][1];
		gfx->mProjMtx.mMtx[1][2] = unk16C[1][2];
		gfx->mProjMtx.mMtx[1][3] = unk16C[1][3];
		gfx->mProjMtx.mMtx[2][0] = unk16C[2][0];
		gfx->mProjMtx.mMtx[2][1] = unk16C[2][1];
		gfx->mProjMtx.mMtx[2][2] = unk16C[2][2];
		gfx->mProjMtx.mMtx[2][3] = unk16C[2][3];
		gfx->mProjMtx.mMtx[3][0] = unk16C[3][0];
		gfx->mProjMtx.mMtx[3][1] = unk16C[3][1];
		gfx->mProjMtx.mMtx[3][2] = unk16C[3][2];
		gfx->mProjMtx.mMtx[3][3] = unk16C[3][3];
		MtxPtr viewMtx = gfx->mViewMtx.mMtx;
		MtxPtr cameraMtx = unk1EC;
		PSMTXCopy(cameraMtx, viewMtx);
		gfx->mNearPlane = mNear;
		gfx->mFarPlane  = mFar;
		if (flags & 0x10)
			GXSetProjection(gfx->mProjMtx.mMtx, GX_PERSPECTIVE);
	}
}

s16 CPolarSubCamera::getFinalAngleZ() const
{
	return unk254 + gpCameraShake->mRollAccum;
}
s16 CPolarSubCamera::getOffsetAngleY() const { return unk68->unk5A; }
s16 CPolarSubCamera::getOffsetAngleX() const { return unk68->unk58; }

#pragma dont_inline on
template <class T> T MsClamp(T t, T l, T r)
{
	if (t > r)
		return r;
	if (t < l)
		return l;
	return t;
}

namespace JGeometry {
f32 TUtil<f32>::one() { return 1.0f; }
} // namespace JGeometry

f32 MsSqrtf(f32 mag)
{
	if (mag > 0.0f) {
		f64 root           = __frsqrte(mag);
		volatile f32 result
		    = 0.5 * root * (3.0 - mag * (root * root)) * mag;
		return result;
	}

	return mag;
}
#pragma dont_inline off

void CPolarSubCamera::ctrlGameCamera_()
{
	if (!(unk64 & 0x400))
		execDeadDemoProc_();

	if (!(unk64 & 0x80) && unk284 > 0)
		--unk284;

	if (unk278 != 0)
		--unk278;
	if (unk27A != 0)
		--unk27A;
	if (unk282 != 0)
		--unk282;

	JGeometry::TVec3<f32> marioPos = *gpMarioPos;
	f32 marioHeight;
	if (isNormalDeadDemo()) {
		marioHeight = 35.0f;
	} else {
		marioHeight = unk68->unk24 + mCurrentTarget.unk28 * unk68->unk28;
		if (SMS_GetMarioStatus() == 0x200345)
			marioHeight += 260.0f;
		if (mMode == 9)
			marioHeight += unk290;
	}
	marioPos.y += marioHeight;
	TCameraMarioData* marioData = gpCameraMario;
	marioData->mPosX            = marioPos.x;
	marioData->mPosY            = marioPos.y;
	marioData->mPosZ            = marioPos.z;
	gpCameraMario->calcAndSetMarioData();

	mPreviousTarget.mPosition = mCurrentTarget.mPosition;
	mPreviousTarget.mTarget = mCurrentTarget.mTarget;
	mPreviousTarget.unk18 = mCurrentTarget.unk18;
	mPreviousTarget.mPitch   = mCurrentTarget.mPitch;
	mPreviousTarget.mYaw   = mCurrentTarget.mYaw;
	mPreviousTarget.unk28   = mCurrentTarget.unk28;
	mPreviousTarget.unk2C   = mCurrentTarget.unk2C;
	mPreviousTarget.unk30   = mCurrentTarget.unk30;

	if (gpMarDirector->mState == 4 && !(unk64 & 0x400)) {
		if (isTalkCameraSpecifyMode(mMode)) {
			bool inTalkEvent = true;
			if (gpMarDirector->unk124 != 1 && gpMarDirector->unk124 != 2)
				inTalkEvent = false;
			if (!inTalkEvent) {
				int oldMode = unk5C;
				s16 frames  = (s16)getCameraInbetweenFrame_(oldMode);
				changeCamModeSpecifyFrame_(oldMode, frames);
			}
		} else if (!isSimpleDemoCamera()) {
			int mode;
			if (controlByCameraCode_(&mode))
				execCameraModeChangeProc_(mode);
		}
	}

	TCameraKindParam kindParam;
	const u8* savePtr = (const u8*)this + mMode * 4;
	kindParam.copySaveParam(
	    **(const TCamSaveKindParam* const*)(savePtr + 0x2D8));

	s32 frameCount = unk6C->mFrameCount;
	bool shouldInterpolate;
	if (frameCount > 0)
		shouldInterpolate = true;
	else
		shouldInterpolate = false;
	if (shouldInterpolate) {
		unk68->inbetweenData(kindParam, (f32)frameCount);
	} else {
		*unk68 = kindParam;
	}

	if (unk284 > (s32)unk68->unk68)
		unk284 = (s32)unk68->unk68;

	if (!isNormalDeadDemo() && !(unk64 & 0x1200))
		mFovy = unk68->unk00;
	mNear = unk68->unk04;
	mFar  = 300000.0f;

	if (isNormalDeadDemo()) {
		ctrlNormalDeadDemo_();
	} else {
		int mode = mMode;
		switch (mode) {
		case 2:
			ctrlMultiPlayerCamera_();
			break;
		case 0x2E:
			ctrlJetCoasterCamera_();
			break;
		default: {
			bool calcPos = true;
			if (!isFixCameraSpecifyMode(mode)
			    && !isDefiniteCameraSpecifyMode(mode))
				calcPos = false;

			if (calcPos) {
				calcPosAndAt_();
			} else if (isLButtonCameraSpecifyMode(mMode)) {
				ctrlLButtonCamera_();
			} else if (isTalkCameraSpecifyMode(mMode)) {
				ctrlTalkCamera_();
			} else {
				ctrlNormalOrTowerCamera_();
			}
			break;
		}
		}
	}

	unk124.x = mPosition.x;
	unk124.y = mPosition.y;
	unk124.z = mPosition.z;
	unk148.x = mTarget.x;
	unk148.y = mTarget.y;
	unk148.z = mTarget.z;
}
void CPolarSubCamera::calcFinalPosAndAt_()
{
	if (mMode != 0x49) {
		gpCameraShake->execShake(unk124, &unk148, &mUp);
		if (mMode != 0x2E) {
			if (!isFixCameraSpecifyMode(mMode) || isNowInbetween()) {
				if (isHellDeadDemo()) {
					if (unk256 > mSaveEx->mSLLimitMaxAngleX.get()) {
						unk7C = 1;
						unk64 |= 0x40;
					}
				} else {
					CLBRevisionLookatByAngleX(
					    mSaveEx->mSLLimitMinAngleX.get(),
					    mSaveEx->mSLLimitMaxAngleX.get(), unk124, &unk148);
				}
			}
		}
	}

	if (unk124 == unk148) {
		unk124.set(unk130);
		unk148.set(unk154);
	} else {
		unk130.set(unk124);
		unk154.set(unk148);
	}
}
void CPolarSubCamera::calcPosAndAt_()
{
	const f32 moveThreshold = 0.05f;
	const u8* save         = (const u8*)unk2D4;
	TCameraOffsetState* offsetState = (TCameraOffsetState*)unk2AC;

	if (gpCameraMario->mDistXZ >= moveThreshold)
		unk64 &= ~0x80;

	if (!(unk64 & 0x80)) {
		if (unk284 > 0) {
			long total   = unk68->unk68;
			long start   = unk68->unk64;
			long elapsed = total - unk284 + 1;
			if (elapsed <= start)
				unk288 = 0.0f;
			else
				unk288 = CLBCalcRatio<long>(start, total, elapsed);
		} else {
			unk288 = 1.0f;
		}
	}

	s16 nozzleMaxAngle = CLBLinearInbetween<s16>(
	    unk68->unk54, unk68->unk56, mCurrentTarget.unk28);
	f32 nozzleMaxDist
	    = CLBLinearInbetween(unk68->unk4C, unk68->unk50, mCurrentTarget.unk28);
	s16 nozzleAngle = 0;
	f32 nozzleDist  = 0.0f;

	bool canUseNozzle = false;
	if (isNormalCameraSpecifyMode(mMode)) {
		bool hasFludd = gpMarioOriginal->mState & MARIO_FLAG_HAS_FLUDD ? true
		                                                               : false;
		if (hasFludd) {
			if (gpMarioOriginal->checkStatusType(0x8000))
				canUseNozzle = true;
		}
	}

	if (canUseNozzle) {
		f32 ratio  = gpCameraMario->mNozzleAngleRatio;
		nozzleDist = ratio * nozzleMaxDist;
		nozzleAngle = (s16)(ratio * (f32)nozzleMaxAngle);
	}

	CLBChaseAngleDecrease(&offsetState->unk0, nozzleAngle,
	                      *(const s16*)(save + 0x16C));
	CLBChaseDecrease(&offsetState->unk4, nozzleDist,
	                 *(const f32*)(save + 0x180), 0.0f);

	if (nozzleMaxDist < 0.001f) {
		if (nozzleMaxAngle != 0) {
			s16 base = nozzleMaxAngle;
			s16 now  = nozzleAngle;
			if (base < 0)
				base = -base;
			if (now < 0)
				now = -now;
			offsetState->unkC = (f32)now * (1.0f / (f32)base);
		}
	} else {
		offsetState->unkC = nozzleDist * (1.0f / nozzleMaxDist);
	}
	offsetState->unkC = MsClamp<f32>(offsetState->unkC, 0.0f, 1.0f);
	CLBChaseDecrease(&offsetState->unk8, offsetState->unkC,
	                 *(const f32*)(save + 0x180), 0.0f);

	unk64 &= ~0x100;
	if (unk78 != 0)
		--unk78;
	if (unk7C != 0)
		--unk7C;

	bool marioMoving = true;
	if (gpCameraMario->mDistXZ < moveThreshold
	    && gpCameraMario->mDistY < moveThreshold)
		marioMoving = false;

	bool nozzleAction = false;
	if (gpMarioOriginal->mState & MARIO_FLAG_HAS_FLUDD) {
		if (gpMarioOriginal->checkStatusType(0x8000))
			nozzleAction = true;
	}

	bool acceptsNozzleControl = true;
	if (!nozzleAction && !(unk64 & 4))
		acceptsNozzleControl = false;

	bool hasControlInput = true;
	if (!acceptsNozzleControl) {
		hasControlInput = false;
		if (unk120 != nullptr
		    && (unk120->mCompSPos[6] != 0.0f
		        || unk120->mCompSPos[7] != 0.0f))
			hasControlInput = true;
	}

	if (unk64 & 0x40) {
		if (unk78 == 0)
			unk78 = 1;
		if (unk7C == 0)
			unk7C = 1;
	} else {
		if (marioMoving || hasControlInput) {
			unk7C = 0;
			unk78 = 0;
		}
		if ((unk64 & 0x400) && unk78 == 0)
			unk78 = 1;
	}

	bool skipSolve = false;
	if (unk78 != 0 && unk7C != 0)
		skipSolve = true;

	if (!skipSolve) {
		if (unk78 == 0 && unk7C == 0)
			unk64 &= ~0x40;

		if (unk6C->mChaseFrame != 0.0f && !marioMoving
		    && !hasControlInput)
			skipSolve = true;
	}

	if (!skipSolve) {
		if (unk7C == 0 && isDefiniteCameraSpecifyMode(mMode))
			mCurrentTarget.mTarget = getUsualLookat();

		Vec targetAt;
		if (isFixCameraSpecifyMode(mMode)
		    || isDefiniteCameraSpecifyMode(mMode)) {
			targetAt.x = mCurrentTarget.mTarget.x;
			targetAt.y = mCurrentTarget.mTarget.y;
			targetAt.z = mCurrentTarget.mTarget.z;
			unk28C     = 0;
		} else {
			mCurrentTarget.mPitch = calcAngleXFromXRotRatio_();
			s16 angleX = mCurrentTarget.mPitch + unk68->unk58 + offsetState->unk0;
			s16 angleY = mCurrentTarget.mYaw + unk68->unk5A;

			if (gpCameraMario->mDistXZ >= moveThreshold) {
				s16 marioBackAngle = *gpMarioAngleY - 0x8000;
				s16 diffFromPrev   = marioBackAngle - unk258;
				f32 chaseAngle
				    = (1.0f - JMASCos(diffFromPrev)) * 0.5f
				      * (f32)unk68->unk60;
				if (chaseAngle > 32766.998f)
					chaseAngle = 32766.998f;
				else if (chaseAngle < -32766.998f)
					chaseAngle = -32766.998f;
				if ((s16)(marioBackAngle - angleY) < 0)
					chaseAngle = -chaseAngle;
				CLBChaseGeneralConstantSpecifySpeed<s16>(
				    &unk28E, CLBRoundf<s16>(chaseAngle),
				    *(const s16*)(save + 0x144));
			}
			angleY += unk28E;

			calcSlopeAngleX_(&angleX);
			if (mMode == 0x41) {
				if (gpCameraMario->isMarioRocketing()) {
					angleX = *(const s16*)(save + 0x1D0) - 0x1770;
				} else if (SMS_GetMarioStatus() == 0x8008A9) {
					angleX = 1000;
				}
			}

			s16 sideAngle = angleY - 0x4000;
			targetAt.x = mCurrentTarget.mTarget.x - unk68->unk5C * JMASSin(sideAngle);
			targetAt.y = mCurrentTarget.mTarget.y;
			targetAt.z = mCurrentTarget.mTarget.z - unk68->unk5C * JMASCos(sideAngle);

			if (mMode != 2) {
				execSecureView_(angleY, &targetAt);
				if (!isRailCameraSpecifyMode(mMode)) {
					Vec anchor = targetAt;
					f32 baseDist = CLBLinearInbetween(
					    unk68->unk08, unk68->unk0C, mCurrentTarget.unk28);
					s16 baseAngleX = CLBLinearInbetween<s16>(
					    unk68->unk18, unk68->unk1A, mCurrentTarget.unk28);
					f32 baseCos = JMASCos(baseAngleX);
					Vec plannedPos;
					CLBPolarToCross(anchor, &plannedPos,
					                baseDist + unk6C->mChaseFrame,
					                angleX, angleY);

					if (isNormalCameraCompletely()) {
						Vec prevPos = mPreviousTarget.mPosition;
						prevPos.y   = mPreviousTarget.unk18.y;
						f32 prevDist;
						s16 prevAngleX;
						s16 prevAngleY;
						CLBCrossToPolar(anchor, prevPos, &prevDist,
						                &prevAngleX, &prevAngleY);

						f32 plannedHoriz = baseCos * baseDist;
						f32 prevHoriz    = baseCos * prevDist;
						if (prevHoriz > plannedHoriz) {
							unk64 |= 0x100;
							mCurrentTarget.unk18 = plannedPos;
						} else {
							f32 lead = CLBLinearInbetween(
							    unk68->unk10, unk68->unk14, mCurrentTarget.unk28);
							f32 minHoriz = baseCos * (baseDist - lead);
							f32 saveMin  = *(const f32*)(save + 0x68);
							if (minHoriz < saveMin)
								minHoriz = saveMin;

							bool canKeepPrev = true;
							if (mMode == 0xD)
								canKeepPrev = false;
							else if (unk54 == 0xD && unk6C->mFrameCount > 0)
								canKeepPrev = false;
							else if (mMode == 0x13)
								canKeepPrev = false;

							if (canKeepPrev && prevHoriz < minHoriz) {
								f32 move = minHoriz - prevHoriz;
								prevPos.x += move * JMASSin(prevAngleY);
								prevPos.z += move * JMASCos(prevAngleY);
								CLBCrossToPolar(anchor, prevPos, &prevDist,
								                &prevAngleX, &prevAngleY);
							}

							bool usePrev = false;
							if (!(unk64 & 0x100)
							    && isNormalCameraCompletely()
							    && unk250 > 0.001f) {
								f32 lead = CLBLinearInbetween(
								    unk68->unk10, unk68->unk14, mCurrentTarget.unk28);
								if (lead > 0.001f)
									usePrev = true;
							}

							if (usePrev) {
								mCurrentTarget.unk18 = prevPos;
							} else {
								CLBPolarToCross(anchor, &mCurrentTarget.unk18, prevDist,
								                prevAngleX, angleY);
							}
							mCurrentTarget.unk18.y = plannedPos.y;
						}
					} else {
						mCurrentTarget.unk18 = plannedPos;
					}

					f32 sinY = JMASSin(angleY);
					f32 cosY = JMASCos(angleY);
					bool keepClear = true;
					if (mMode == 0xD)
						keepClear = false;
					else if (unk54 == 0xD && unk6C->mFrameCount > 0)
						keepClear = false;
					else if (mMode == 0x13)
						keepClear = false;

					if (keepClear) {
						f32 dz = gpMarioPos->z - mCurrentTarget.unk18.z;
						f32 dx = gpMarioPos->x - mCurrentTarget.unk18.x;
						f32 xz = MsSqrtf(dx * dx + dz * dz);
						f32 limit = baseDist * baseCos;
						f32 saveLimit = *(const f32*)(save + 0x68);
						if (saveLimit < limit)
							limit = saveLimit;
						if (xz < limit) {
							f32 push = limit - xz;
							mCurrentTarget.unk18.x += push * sinY;
							mCurrentTarget.unk18.z += push * cosY;
						}
					}

					if (mMode != 8 && mMode != 0xD) {
						mCurrentTarget.unk18.x -= offsetState->unk4 * sinY;
						mCurrentTarget.unk18.z -= offsetState->unk4 * cosY;
					}

					if (unk6C->mChaseFrame != 0.0f && hasControlInput) {
						Vec warpPos;
						CLBPolarToCross(
						    unk6C->mTargetAt, &warpPos,
						    CLBLinearInbetween(unk68->unk08, unk68->unk0C,
						                       mCurrentTarget.unk28)
						        + unk6C->mChaseFrame,
						    angleX, angleY);
						unk6C->warpPosAndAt(warpPos, unk6C->mTargetAt);
					}
				}
			}

			mCurrentTarget.mPosition.x = mCurrentTarget.unk18.x;
			mCurrentTarget.mPosition.z = mCurrentTarget.unk18.z;
			execHeightPan_();
			targetAt.y = mCurrentTarget.mTarget.y;

			Vec checkedPos = mCurrentTarget.mPosition;
			if (isNeedWallCheck_() && execWallCheck_(&checkedPos)) {
				f32 dz = mCurrentTarget.mPosition.z - mCurrentTarget.mTarget.z;
				f32 dx = mCurrentTarget.mPosition.x - mCurrentTarget.mTarget.x;
				mCurrentTarget.mPitch = matan(MsSqrtf(dx * dx + dz * dz),
				              mCurrentTarget.mPosition.y - mCurrentTarget.mTarget.y);
				mCurrentTarget.mYaw = matan(dz, dx);
				mCurrentTarget.unk28 = mPreviousTarget.unk28;
			}

			if (isNeedRoofCheck_())
				execRoofCheck_(checkedPos);
			if (isNeedGroundCheck_())
				execGroundCheck_(checkedPos);
		}

		unk6C->execCameraInbetween(mCurrentTarget.mPosition, targetAt, *gpMarioPos);
	}

	if (unk78 == 0) {
		f32 posChaseXZ;
		f32 posChaseY;
		if (isRailCameraSpecifyMode(unk54)
		    && isRailCameraSpecifyMode(mMode) && unk6C->mFrameCount > 0) {
			posChaseXZ = 1.0f;
			posChaseY  = 1.0f;
		} else {
			posChaseXZ = unk68->unk94;
			posChaseY  = unk68->unk9C;
			if (mMode == 0x41 && SMS_GetMarioStatus() == 0x8008A9
			    && gpCameraMario->mStatusTimer < 0x78) {
				f32 ratio = (f32)(0x78 - gpCameraMario->mStatusTimer)
				            * (1.0f / 120.0f);
				posChaseY = CLBLinearInbetween(unk68->unk9C, 0.0f,
				                              ratio);
			}

			if (unk64 & 4) {
				posChaseXZ = 1.0f;
			} else if (unk120->mSubStick.mPosY != 0.0f
			           || unk120->mSubStick.mPosX != 0.0f) {
				f32 dist = gpCameraMario->mDistXZ;
				if (dist > 20.0f)
					dist = 20.0f;
				f32 ratio = CLBCalcRatio<f32>(20.0f, 0.0f, dist);
				posChaseXZ = CLBEaseInInbetween(unk68->unk94, unk68->unk98,
				                                ratio);
			}

			if (unk120->mSubStick.mPosY != 0.0f) {
				f32 dist = gpCameraMario->mDistY;
				if (dist > 20.0f)
					dist = 20.0f;
				f32 ratio = CLBCalcRatio<f32>(20.0f, 0.0f, dist);
				posChaseY = CLBEaseInInbetween(unk68->unk9C, unk68->unkA0,
				                               ratio);
			}
		}

		CLBChaseDecrease(&mPosition.x, unk6C->mTargetPos.x, posChaseXZ,
		                 0.0f);
		CLBChaseDecrease(&mPosition.y, unk6C->mTargetPos.y, posChaseY,
		                 0.0f);
		CLBChaseDecrease(&mPosition.z, unk6C->mTargetPos.z, posChaseXZ,
		                 0.0f);
	}

	if (unk7C == 0) {
		f32 atChaseXZ = unk68->unkA4;
		f32 atChaseY  = unk68->unkA8;
		const Vec& at = unk6C->mTargetAt;
		CLBChaseDecrease(&mTarget.x, at.x, atChaseXZ, 0.0f);
		CLBChaseDecrease(&mTarget.y, at.y, atChaseY, 0.0f);
		CLBChaseDecrease(&mTarget.z, at.z, atChaseXZ, 0.0f);
	}
}
void CPolarSubCamera::calcSlopeAngleX_(s16* out)
{
	s16 slopeAngle       = 0;
	bool isNozzleAction  = false;
	bool canSampleSlope  = false;
	TMario* mario        = gpMarioOriginal;

	if (mario->mState & MARIO_FLAG_HAS_FLUDD) {
		if (mario->checkStatusType(0x8000))
			isNozzleAction = true;
	}

	if (!isNozzleAction) {
		const TBGCheckData* ground = *gpMarioGroundPlane;
		if (ground != nullptr) {
			if (ground->mBGType & 0xA000)
				canSampleSlope = true;
		}

		if (canSampleSlope && isSlopeCameraMode()) {
			JGeometry::TVec3<f32> toMario;
			toMario.set(gpMarioPos->x - mPosition.x, 0.0f,
			            gpMarioPos->z - mPosition.z);
			if (!toMario.isZero()) {
				f32 marioGroundY   = SMS_GetMarioGrLevel();
				s16 maxSlopeAngle  = mSaveEx->mSLSlopeMaxAngleX.get();
				f32 forwardDist    = mSaveEx->mSLSlopeForwardDistXZ.get();
				JGeometry::TVec3<f32> forwardOffset;
				MsVECNormalize(&toMario, &forwardOffset);
				forwardOffset *= forwardDist;

				JGeometry::TVec3<f32> sample = SMS_GetMarioPos();
				sample += forwardOffset;
				JGeometry::TVec3<f32> checkPos = sample;
				JGeometry::TVec3<f32> checkPos2 = checkPos;

				const TBGCheckData* checkData;
				f32 checkY = forwardDist
				                 * (JMASSin(maxSlopeAngle)
				                    * (1.0f / JMASCos(maxSlopeAngle)))
				             + 10.0f + gpMarioPos->y;
				f32 groundY = gpMap->checkGroundIgnoreWaterSurface(
				    checkPos2.x, checkY, checkPos2.z, &checkData);
				f32 rise = groundY - marioGroundY;
				s16 sampleAngle = 0;
				if (rise > 0.0f)
					sampleAngle = matan(forwardDist, rise);

				if (sampleAngle > maxSlopeAngle)
					slopeAngle = 0;
				else
					slopeAngle = sampleAngle;
			}
		}
	}

	s16 speed = mSaveEx->mSLSlopeSpeedAngleX.get();
	CLBChaseGeneralConstantSpecifySpeed<s16>(
	    &unk28C, slopeAngle, speed);
	*out -= unk28C;
	s16 angle = *out;
	s16 min   = mSaveEx->mSLLimitMinAngleX.get();
	s16 max   = mSaveEx->mSLLimitMaxAngleX.get();
	if (angle > max)
		angle = max;
	else if (angle < min)
		angle = min;
	*out = angle;
}

bool CPolarSubCamera::isMomentDefinite_() const
{
	bool result = false;
	if (!(unk64 & 0x100) && isNormalCameraCompletely() && unk250 > 0.001f
	    && CLBLinearInbetween(unk68->unk10, unk68->unk14,
	                          mCurrentTarget.unk28)
	           > 0.001f)
		result = true;
	return result;
}

void CPolarSubCamera::execInvalidAutoChase_() { unk284 = unk68->unk68; }

static inline bool isMarioReadyGun()
{
	return gpMarioOriginal->checkFlag(MARIO_FLAG_HAS_FLUDD)
	       && gpMarioOriginal->checkStatusType(0x8000);
}

bool CPolarSubCamera::isMarioCrabWalk_() const
{
	return isMarioReadyGun() && (unk120->mMeaning & TMarioGamePad::MEANING_0x8000);
}

bool CPolarSubCamera::isMarioAimWithGun_() const
{
	return isMarioReadyGun() && (unk120->mMeaning & TMarioGamePad::MEANING_0x400);
}

void CPolarSubCamera::onMoveApproach_()
{
	f32 dist = (mPosition.x - mTarget.x) * (mPosition.x - mTarget.x)
	           + (mPosition.y - mTarget.y) * (mPosition.y - mTarget.y)
	           + (mPosition.z - mTarget.z) * (mPosition.z - mTarget.z);
	if (dist > 0.0f) {
		f64 root = __frsqrte(dist);
		volatile f32 result
		    = 0.5 * root * (3.0 - dist * (root * root)) * dist;
		dist = result;
	}
	unk6C->mChaseFrame
	    = dist
	      - CLBLinearInbetween(unk68->unk08, unk68->unk0C,
	                           mCurrentTarget.unk28);
}

void CPolarSubCamera::offMoveApproach_() { unk6C->mChaseFrame = 0.0f; }

void CPolarSubCamera::rotateY_ByStickX_(f32 stick)
{
	if (!SMS_IsMarioOpeningDoor()) {
		s16 speed = CLBLinearInbetween<s16>(
		    unk68->unk20, unk68->unk22, mCurrentTarget.unk28);
		mCurrentTarget.mYaw += (s16)(stick * speed);
	}
}

void CPolarSubCamera::rotateX_ByStickY_(f32 stick)
{
	if (!SMS_IsMarioOpeningDoor()) {
		mCurrentTarget.unk28 -= stick * unk68->unk1C;
		if (isLButtonCameraSpecifyMode(mMode)) {
			f32 ratio = mCurrentTarget.unk28;
			if (ratio > 1.0f)
				ratio = 1.0f;
			else if (ratio < 0.0f)
				ratio = 0.0f;
			mCurrentTarget.unk28 = ratio;
		} else {
			f32 min   = unk268;
			f32 max   = unk26C;
			f32 ratio = mCurrentTarget.unk28;
			if (ratio > max)
				ratio = max;
			else if (ratio < min)
				ratio = min;
			mCurrentTarget.unk28 = ratio;
		}
	}
}

static inline f32 clampCameraRatio(f32 value, f32 min, f32 max)
{
	if (value > max)
		value = max;
	else if (value < min)
		value = min;
	return value;
}

void CPolarSubCamera::calcNowTargetFromPosAndAt_(const Vec& pos, const Vec& at)
{
	f32 radius;
	s16 angleX;
	s16 angleY;
	CLBCrossToPolar(at, pos, &radius, &angleX, &angleY);
	mCurrentTarget.unk28 = CLBCalcRatio<s16>(unk68->unk18, unk68->unk1A, angleX);
	if (isLButtonCameraSpecifyMode(mMode))
		mCurrentTarget.unk28 = clampCameraRatio(mCurrentTarget.unk28, 0.0f, 1.0f);
	else
		mCurrentTarget.unk28 = clampCameraRatio(mCurrentTarget.unk28, unk268, unk26C);
	mCurrentTarget.mPitch = CLBLinearInbetween<s16>(
	    unk68->unk18, unk68->unk1A, mCurrentTarget.unk28);
	mCurrentTarget.mYaw = angleY;
	mCurrentTarget.mPosition.set(pos);
	mCurrentTarget.unk18.set(pos);
	mCurrentTarget.mTarget.set(at);
}

f32 CPolarSubCamera::calcDistFromXRotRatio_() const
{
	return CLBLinearInbetween(unk68->unk08, unk68->unk0C, mCurrentTarget.unk28);
}

s16 CPolarSubCamera::calcAngleXFromXRotRatio_() const
{
	return CLBLinearInbetween<s16>(unk68->unk18, unk68->unk1A,
	                                      mCurrentTarget.unk28);
}

JGeometry::TVec3<f32> CPolarSubCamera::getUsualLookat() const
{
	JGeometry::TVec3<f32> result;
	JGeometry::TVec3<f32>* lookat
	    = *(JGeometry::TVec3<f32>**)unk2B4;
	if (lookat != nullptr)
		result = *lookat;
	else
		result = *(JGeometry::TVec3<f32>*)gpCameraMario;
	return result;
}

MtxPtr CPolarSubCamera::getToroccoMtx_() const
{
	return gpMarioOriginal->mTorocco->unk4->mNodeMatrices[2];
}
bool CPolarSubCamera::isNowInbetween() const
{
	if (unk6C->mFrameCount > 0)
		return true;
	return false;
}

inline void CPolarSubCamera::startJetCoasterCam1()
{
	unk2B0->startDemo(cJetCoasterCam1BckName, nullptr);
	unk2B0->setFrame(0.5f * (f32)gpMarDirector->unk58);
}

static s32 JetCoasterDemoCallBack(u32 user_data, u32 event)
{
	if (event == 1)
		((CPolarSubCamera*)user_data)->startJetCoasterCam1();

	return 1;
}

void CPolarSubCamera::loadAfter()
{
	JDrama::TNameRef::loadAfter();

	unk5C = 0;
	if (gpMarDirector->mMap == 7) {
		unk5C = 0x14;
	} else if (SMS_isExMap()) {
		unk5C = 0x26;
	} else if (SMS_isMultiPlayerMap()) {
		unk5C = 2;
	} else if (unk64 & 0x1000) {
		unk5C = 0x2E;
	}

	TCameraMapTool* startTool = (TCameraMapTool*)gpCamMapToolTable->searchF(
	    JDrama::TNameRef::calcKeyCode(cStartCamName), cStartCamName);
	if (startTool != nullptr)
		changeCamModeSpecifyCamMapToolAndFrame_(startTool, 1);
	else
		changeCamModeSpecifyFrame_(unk5C, 1);

	const u8* savePtr = (const u8*)this + mMode * 4;
	unk68->copySaveParam(
	    **(const TCamSaveKindParam* const*)(savePtr + 0x2D8));
	mFovy = unk68->unk00;
	mNear = unk68->unk04;

	char startAfterName[0x40];
	snprintf(startAfterName, 0x40, "%s%d", cStartAfterCamName,
	         gpMarDirector->unkD0);
	TCameraMapTool* startAfterTool
	    = (TCameraMapTool*)gpCamMapToolTable->searchF(
	        JDrama::TNameRef::calcKeyCode(startAfterName), startAfterName);
	if (startAfterTool != nullptr) {
		f32 ratio = startAfterTool->mPosition.y;
		if (ratio > unk26C)
			ratio = unk26C;
		else if (ratio < unk268)
			ratio = unk268;
		mCurrentTarget.unk28 = ratio;
		mCurrentTarget.mYaw
		    = CLBRoundf<s16>(182.04445f * startAfterTool->mPitchYaw.y)
		        - 0x8000;
	} else {
		const u8* save = (const u8*)unk2D4;
		f32 ratio      = *(const f32*)(save + 0x18);
		if (ratio > unk26C)
			ratio = unk26C;
		else if (ratio < unk268)
			ratio = unk268;
		mCurrentTarget.unk28 = ratio;
		mCurrentTarget.mYaw = *gpMarioAngleY - 0x8000;
	}

	if ((unk64 & 0x1000) && unk2B8 != nullptr && (*(u8*)((u8*)unk2B8 + 0xC) & 1))
		setUpToLButtonCamera_(0x2E);

	unk270 = mCurrentTarget.unk28;
	mCurrentTarget.mPitch = CLBLinearInbetween<s16>(
	    unk68->unk18, unk68->unk1A, mCurrentTarget.unk28);

	JGeometry::TVec3<f32> marioPos = *gpMarioPos;
	f32 marioHeight;
	if (isNormalDeadDemo()) {
		marioHeight = 35.0f;
	} else {
		marioHeight = unk68->unk24 + mCurrentTarget.unk28 * unk68->unk28;
		if (SMS_GetMarioStatus() == 0x200345)
			marioHeight += 260.0f;
		if (mMode == 9)
			marioHeight += unk290;
	}
	marioPos.y += marioHeight;
	TCameraMarioData* marioData = gpCameraMario;
	marioData->mPosX = marioPos.x;
	marioData->mPosY = marioPos.y;
	marioData->mPosZ = marioPos.z;

	if (unk70 != nullptr) {
		unk70->calcPosAndAt(&mPosition, &mTarget);
	} else {
		mTarget.x = marioData->mPosX;
		mTarget.y = marioData->mPosY;
		mTarget.z = marioData->mPosZ;
		f32 dist = CLBLinearInbetween(unk68->unk08, unk68->unk0C,
		                                mCurrentTarget.unk28);
		CLBPolarToCross(mTarget, &mPosition, dist, mCurrentTarget.mPitch,
		                mCurrentTarget.mYaw);
	}

	calcSecureViewTarget_(mCurrentTarget.mYaw, &unk294, &unk298);
	mPosition.x += unk294;
	mPosition.z += unk298;
	mTarget.x += unk294;
	mTarget.z += unk298;

	mCurrentTarget.mPosition.x = mPosition.x;
	mCurrentTarget.mPosition.y = mPosition.y;
	mCurrentTarget.mPosition.z = mPosition.z;
	mCurrentTarget.unk18.x = mPosition.x;
	mCurrentTarget.unk18.y = mPosition.y;
	mCurrentTarget.unk18.z = mPosition.z;
	mCurrentTarget.mTarget.x = mTarget.x;
	mCurrentTarget.mTarget.y = mTarget.y;
	mCurrentTarget.mTarget.z = mTarget.z;
	if (SMS_isOptionMap()) {
		mCurrentTarget.mPosition.x = mPosition.x;
		mCurrentTarget.mPosition.y = mPosition.y;
		mCurrentTarget.mPosition.z = mPosition.z;
		mCurrentTarget.mTarget.x = mTarget.x;
		mCurrentTarget.mTarget.y = mTarget.y;
		mCurrentTarget.mTarget.z = mTarget.z;
		gpCameraOption = new TCameraOption(mPosition, &mCurrentTarget.mTarget);
	}

	unk256 = mCurrentTarget.mPitch;
	unk258 = mCurrentTarget.mYaw;
	mPreviousTarget.mPosition  = mCurrentTarget.mPosition;
	mPreviousTarget.mTarget  = mCurrentTarget.mTarget;
	mPreviousTarget.unk18  = mCurrentTarget.unk18;
	mPreviousTarget.mPitch  = mCurrentTarget.mPitch;
	mPreviousTarget.mYaw  = mCurrentTarget.mYaw;
	mPreviousTarget.unk28  = mCurrentTarget.unk28;
	mPreviousTarget.unk2C  = mCurrentTarget.unk2C;
	mPreviousTarget.unk30  = mCurrentTarget.unk30;
	unkE8  = mPreviousTarget.mPosition;
	unkF4  = mPreviousTarget.mTarget;
	unk100 = mPreviousTarget.unk18;
	unk10C = mPreviousTarget.mPitch;
	unk10E = mPreviousTarget.mYaw;
	unk110 = mPreviousTarget.unk28;
	unk114 = mPreviousTarget.unk2C;
	unk118 = mPreviousTarget.unk30;

	unk124.x = mPosition.x;
	unk124.y = mPosition.y;
	unk124.z = mPosition.z;
	unk130.x = mPosition.x;
	unk130.y = mPosition.y;
	unk130.z = mPosition.z;
	unk13C.x = mPosition.x;
	unk13C.y = mPosition.y;
	unk13C.z = mPosition.z;
	unk148.x = mTarget.x;
	unk148.y = mTarget.y;
	unk148.z = mTarget.z;
	unk154.x = mTarget.x;
	unk154.y = mTarget.y;
	unk154.z = mTarget.z;
	unk160.x = mTarget.x;
	unk160.y = mTarget.y;
	unk160.z = mTarget.z;

	if (unk64 & 0x1000) {
		JGeometry::TVec3<f32>* coasterPos
		    = (JGeometry::TVec3<f32>*)((u8*)unk2B8 + 0x10);
		coasterPos->x = mPosition.x;
		coasterPos->y = mPosition.y;
		coasterPos->z = mPosition.z;
		JGeometry::TVec3<f32>* coasterTarget
		    = (JGeometry::TVec3<f32>*)((u8*)unk2B8 + 0x1C);
		coasterTarget->x = mTarget.x;
		coasterTarget->y = mTarget.y;
		coasterTarget->z = mTarget.z;
	}

	unk6C->initCameraInbetween(mPosition, mTarget, *gpMarioPos);
	C_MTXPerspective(unk16C, mFovy, mAspect, mNear, mFar);
	C_MTXLookAt(unk1EC, &unk124, &mUp, &unk148);
	unk1AC[0][0] = unk16C[0][0];
	unk1AC[0][1] = unk16C[0][1];
	unk1AC[0][2] = unk16C[0][2];
	unk1AC[0][3] = unk16C[0][3];
	unk1AC[1][0] = unk16C[1][0];
	unk1AC[1][1] = unk16C[1][1];
	unk1AC[1][2] = unk16C[1][2];
	unk1AC[1][3] = unk16C[1][3];
	unk1AC[2][0] = unk16C[2][0];
	unk1AC[2][1] = unk16C[2][1];
	unk1AC[2][2] = unk16C[2][2];
	unk1AC[2][3] = unk16C[2][3];
	unk1AC[3][0] = unk16C[3][0];
	unk1AC[3][1] = unk16C[3][1];
	unk1AC[3][2] = unk16C[3][2];
	unk1AC[3][3] = unk16C[3][3];
	PSMTXCopy(unk1EC, unk21C);

	fabricatedInline2();

	if ((unk64 & 0x1000) && gpMarDirector->unk7D == 1) {
		gpMarDirector->fireStartDemoCamera(
		    cJetCoasterDemoBckName, nullptr, -1, 0.0f, true,
		    JetCoasterDemoCallBack, (u32)this, nullptr, JDrama::TFlagT<u16>(0));
	} else if (JKRFileLoader::getGlbResource(cStartCamBckFileName)
	           == nullptr) {
		calcInHouseNo_(true);
	}
}

void CPolarSubCamera::calcExternalData_() { }
void CPolarSubCamera::setMarioLookat_() { }
