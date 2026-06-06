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
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <System/MarioGamePad.hpp>
#include <dolphin/gx.h>
#include <stdio.h>

template <> f32 CLBLinearInbetween<f32>(f32, f32, f32);
template <> f32 CLBCalcRatio<s16>(s16, s16, s16);
template <> BOOL CLBChaseGeneralConstantSpecifySpeed<s16>(s16*, s16, s16);

extern Vec CLBConstUpVec;

const char* cStartCamName
    = "\x8A\x4A\x8E\x6E\x83\x4A\x83\x81\x83\x89";
const char* cStartAfterCamName
    = "\x8A\x4A\x8E\x6E\x8C\xE3\x83\x4A\x83\x81\x83\x89";
const char* cJetCoasterCam0BckName  = "pinna2_camera";
const char* cJetCoasterCam1BckName  = "tinkoopa_camera";
const char* cJetCoasterDemoBckName  = "tinkoopa_killer_camera";
const char* cStartCamBckFileName    = "/scene/map/camera/StartCamera.bck";

CPolarSubCamera::CPolarSubCamera(const char* name)
    : JDrama::TLookAtCamera()
{
	(void)name;
	// TODO: full ctor
}

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

			f32 dz = mPosition.z - mTarget.z;
			f32 dx = mPosition.x - mTarget.x;
			unk256 = matan(MsSqrtf(dx * dx + dz * dz),
			               mPosition.y - mTarget.y);
			unk258 = matan(dz, dx);
			unk25C.set(unk148.x - unk124.x, unk148.y - unk124.y,
			           unk148.z - unk124.z);
			unk25C.setLength(unk25C, JGeometry::TUtil<f32>::one());
			unk270 = MsClamp<f32>(
			    CLBCalcRatio<s16>(unk68->unk18, unk68->unk1A, unk256),
			    0.0f, 1.0f);
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
					((TCameraBck*)unk2B0)
					    ->updateDemo(
					        (JGeometry::TVec3<f32>*)((u8*)unk2B8 + 0x10),
					        (JGeometry::TVec3<f32>*)((u8*)unk2B8 + 0x1C),
					        (JGeometry::TVec3<f32>*)((u8*)unk2B8 + 0x28),
					        (f32*)((u8*)unk2B8 + 0x34));
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
		PSMTXCopy(unk1EC, gfx->mViewMtx.mMtx);
		gfx->mNearPlane = mNear;
		gfx->mFarPlane  = mFar;
		if (flags & 0x10)
			GXSetProjection(gfx->mProjMtx.mMtx, GX_PERSPECTIVE);
	}
}

s16 CPolarSubCamera::getFinalAngleZ() const
{
	return unk254 + gpCameraShake->mYaw;
}
s16 CPolarSubCamera::getOffsetAngleY() const { return unk68->unk5A; }
s16 CPolarSubCamera::getOffsetAngleX() const { return unk68->unk58; }

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
		marioHeight = unk68->unk24 + unkA8 * unk68->unk28;
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
	marioData->calcAndSetMarioData();

	unkB4 = unk80;
	unkC0 = unk8C;
	unkCC = unk98;
	unkD8   = unkA4;
	unkDA   = unkA6;
	unkDC   = unkA8;
	unkE0   = unkAC;
	unkE4   = unkB0;

	if (gpMarDirector->mState == 4 && !(unk64 & 0x400)) {
		if (isTalkCameraSpecifyMode(mMode)) {
			if (gpMarDirector->unk124 != 1 && gpMarDirector->unk124 != 2) {
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
	const TCamSaveKindParam* const* saveTable
	    = (const TCamSaveKindParam* const*)((u8*)this + 0x2D8);
	kindParam.copySaveParam(*saveTable[mMode]);

	s32 frameCount = unk6C->mFrameCount;
	if (frameCount > 0) {
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
			if (!isFixCameraSpecifyMode(mMode) || unk6C->mFrameCount > 0) {
				if (isHellDeadDemo()) {
					const u8* save = (const u8*)unk2D4;
					if (unk256 > *(const s16*)(save + 0x1D0)) {
						unk7C = 1;
						unk64 |= 0x40;
					}
				} else {
					const u8* save = (const u8*)unk2D4;
					CLBRevisionLookatByAngleX(
					    *(const s16*)(save + 0x1BC),
					    *(const s16*)(save + 0x1D0), unk124, &unk148);
				}
			}
		}
	}

	bool sameXY  = false;
	bool sameAll = false;
	if (unk124.x == unk148.x && unk124.y == unk148.y)
		sameXY = true;
	if (sameXY && unk124.z == unk148.z)
		sameAll = true;

	if (sameAll) {
		unk124.x = unk130.x;
		unk124.y = unk130.y;
		unk124.z = unk130.z;
		unk148.x = unk154.x;
		unk148.y = unk154.y;
		unk148.z = unk154.z;
	} else {
		unk130.x = unk124.x;
		unk130.y = unk124.y;
		unk130.z = unk124.z;
		unk154.x = unk148.x;
		unk154.y = unk148.y;
		unk154.z = unk148.z;
	}
}
void CPolarSubCamera::calcPosAndAt_() { }
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
			Vec toMario;
			toMario.x = gpMarioPos->x - mPosition.x;
			toMario.y = 0.0f;
			toMario.z = gpMarioPos->z - mPosition.z;
			if (toMario.x * toMario.x + toMario.y * toMario.y
			        + toMario.z * toMario.z
			    > 0.0000038146973f) {
				const u8* save     = (const u8*)unk2D4;
				f32 marioGroundY   = SMS_GetMarioGrLevel();
				f32 forwardDist    = *(const f32*)(save + 0x20C);
				s16 maxSlopeAngle  = *(const s16*)(save + 0x1E4);
				Vec forwardOffset;
				MsVECNormalize(&toMario, &forwardOffset);
				forwardOffset.x *= forwardDist;
				forwardOffset.y *= forwardDist;
				forwardOffset.z *= forwardDist;

				Vec checkPos = *gpMarioPos;
				checkPos.x += forwardOffset.x;
				checkPos.y += forwardOffset.y;
				checkPos.z += forwardOffset.z;

				const TBGCheckData* checkData;
				f32 checkY = gpMarioPos->y + 10.0f
				             + forwardDist
				                 * (JMASSin(maxSlopeAngle)
				                    * (1.0f / JMASCos(maxSlopeAngle)));
				f32 groundY = gpMap->checkGroundIgnoreWaterSurface(
				    checkPos.x, checkY, checkPos.z, &checkData);
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

	const u8* save = (const u8*)unk2D4;
	CLBChaseGeneralConstantSpecifySpeed<s16>(
	    &unk28C, slopeAngle, *(const s16*)(save + 0x1F8));
	*out -= unk28C;
	s16 angle = *out;
	s16 max   = *(const s16*)(save + 0x1D0);
	s16 min   = *(const s16*)(save + 0x1BC);
	if (angle > max)
		angle = max;
	else if (angle < min)
		angle = min;
	*out = angle;
}

#pragma dont_inline on
BOOL TMario::checkStatusType(long status) const
{
	if (mAction & status)
		return TRUE;
	return FALSE;
}
#pragma dont_inline off

bool CPolarSubCamera::isMomentDefinite_() const
{
	bool result = false;
	if (!(unk64 & 0x100) && isNormalCameraCompletely() && unk250 > 0.0f
	    && CLBLinearInbetween(unk68->unk10, unk68->unk14, unkA8) > 0.0f)
		result = true;
	return result;
}

void CPolarSubCamera::execInvalidAutoChase_() { unk284 = unk68->unk68; }
bool CPolarSubCamera::isMarioCrabWalk_() const
{
	bool canUseNozzle = false;
	bool result       = false;
	if (gpMarioOriginal->mState & MARIO_FLAG_HAS_FLUDD) {
		if (gpMarioOriginal->checkStatusType(0x8000))
			canUseNozzle = true;
	}
	if (canUseNozzle && (unk120->mMeaning & TMarioGamePad::MEANING_0x8000))
		result = true;
	return result;
}

bool CPolarSubCamera::isMarioAimWithGun_() const
{
	bool canUseNozzle = false;
	bool result       = false;
	if (gpMarioOriginal->mState & MARIO_FLAG_HAS_FLUDD) {
		if (gpMarioOriginal->checkStatusType(0x8000))
			canUseNozzle = true;
	}
	if (canUseNozzle && (unk120->mMeaning & TMarioGamePad::MEANING_0x400))
		result = true;
	return result;
}
bool CPolarSubCamera::isMarioReadyGun_() const { return false; }

void CPolarSubCamera::onMoveApproach_()
{
	f32 dx   = mPosition.x - mTarget.x;
	f32 dy   = mPosition.y - mTarget.y;
	f32 dz   = mPosition.z - mTarget.z;
	f32 dist = JGeometry::TUtil<f32>::sqrt(dx * dx + dy * dy + dz * dz);
	unk6C->mChaseFrame
	    = dist - CLBLinearInbetween(unk68->unk08, unk68->unk0C, unkA8);
}

void CPolarSubCamera::offMoveApproach_() { unk6C->mChaseFrame = 0.0f; }

void CPolarSubCamera::rotateY_ByStickX_(f32 stick)
{
	if (!SMS_IsMarioOpeningDoor()) {
		s16 speed = CLBLinearInbetween<s16>(unk68->unk20, unk68->unk22, unkA8);
		unkA6 += (s16)(stick * speed);
	}
}

void CPolarSubCamera::rotateX_ByStickY_(f32 stick)
{
	if (!SMS_IsMarioOpeningDoor()) {
		unkA8 -= stick * unk68->unk1C;
		if (isLButtonCameraSpecifyMode(mMode)) {
			f32 ratio = unkA8;
			if (ratio > 1.0f)
				ratio = 1.0f;
			else if (ratio < 0.0f)
				ratio = 0.0f;
			unkA8 = ratio;
		} else {
			f32 max   = unk26C;
			f32 ratio = unkA8;
			f32 min   = unk268;
			if (ratio > max)
				ratio = max;
			else if (ratio < min)
				ratio = min;
			unkA8 = ratio;
		}
	}
}

void CPolarSubCamera::calcNowTargetFromPosAndAt_(const Vec& pos, const Vec& at)
{
	f32 dist;
	s16 angleX;
	s16 angleY;
	CLBCrossToPolar(at, pos, &dist, &angleX, &angleY);
	unkA8 = CLBCalcRatio<s16>(unk68->unk18, unk68->unk1A, angleX);
	if (isLButtonCameraSpecifyMode(mMode)) {
		f32 ratio = unkA8;
		if (ratio > 1.0f)
			ratio = 1.0f;
		else if (ratio < 0.0f)
			ratio = 0.0f;
		unkA8 = ratio;
	} else {
		f32 max   = unk26C;
		f32 ratio = unkA8;
		f32 min   = unk268;
		if (ratio > max)
			ratio = max;
		else if (ratio < min)
			ratio = min;
		unkA8 = ratio;
	}
	unkA4 = CLBLinearInbetween<s16>(unk68->unk18, unk68->unk1A, unkA8);
	unkA6 = angleY;
	unk80.x = pos.x;
	unk80.y = pos.y;
	unk80.z = pos.z;
	unk98.x = pos.x;
	unk98.y = pos.y;
	unk98.z = pos.z;
	unk8C.x = at.x;
	unk8C.y = at.y;
	unk8C.z = at.z;
}

f32 CPolarSubCamera::calcDistFromXRotRatio_() const
{
	return CLBLinearInbetween(unk68->unk08, unk68->unk0C, unkA8);
}

s16 CPolarSubCamera::calcAngleXFromXRotRatio_() const
{
	return CLBLinearInbetween<s16>(unk68->unk18, unk68->unk1A, unkA8);
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

static s32 JetCoasterDemoCallBack(u32 user_data, u32 event)
{
	if (event == 1) {
		CPolarSubCamera* camera = (CPolarSubCamera*)user_data;
		((TCameraBck*)camera->unk2B0)->startDemo(cJetCoasterCam1BckName,
		                                         nullptr);
		((TCameraBck*)camera->unk2B0)
		    ->setFrame(0.5f * (f32)gpMarDirector->unk58);
	}
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

	const TCamSaveKindParam* const* saveTable
	    = (const TCamSaveKindParam* const*)((u8*)this + 0x2D8);
	unk68->copySaveParam(*saveTable[mMode]);
	mFovy = unk68->unk00;
	mNear = unk68->unk04;

	char startAfterName[0x40];
	snprintf(startAfterName, 0x40, "%s%d", cStartAfterCamName,
	         gpMarDirector->unkD0);
	TCameraMapTool* startAfterTool
	    = (TCameraMapTool*)gpCamMapToolTable->searchF(
	        JDrama::TNameRef::calcKeyCode(startAfterName), startAfterName);
	if (startAfterTool != nullptr) {
		f32 ratio = startAfterTool->unkC.y;
		if (ratio > unk26C)
			ratio = unk26C;
		else if (ratio < unk268)
			ratio = unk268;
		unkA8 = ratio;
		unkA6 = CLBRoundf<s16>(182.04445f * startAfterTool->unk18.y)
		        - 0x8000;
	} else {
		const u8* save = (const u8*)unk2D4;
		f32 ratio      = *(const f32*)(save + 0x18);
		if (ratio > unk26C)
			ratio = unk26C;
		else if (ratio < unk268)
			ratio = unk268;
		unkA8 = ratio;
		unkA6 = *gpMarioAngleY - 0x8000;
	}

	if ((unk64 & 0x1000) && unk2B8 != nullptr && (*(u8*)((u8*)unk2B8 + 0xC) & 1))
		setUpToLButtonCamera_(0x2E);

	unk270 = unkA8;
	unkA4  = CLBLinearInbetween<s16>(unk68->unk18, unk68->unk1A, unkA8);

	JGeometry::TVec3<f32> marioPos = *gpMarioPos;
	f32 marioHeight;
	if (isNormalDeadDemo()) {
		marioHeight = 35.0f;
	} else {
		marioHeight = unk68->unk24 + unkA8 * unk68->unk28;
		if (SMS_GetMarioStatus() == 0x200345)
			marioHeight += 260.0f;
		if (mMode == 9)
			marioHeight += unk290;
	}
	marioPos.y += marioHeight;
	gpCameraMario->mPosX = marioPos.x;
	gpCameraMario->mPosY = marioPos.y;
	gpCameraMario->mPosZ = marioPos.z;

	if (unk70 != nullptr) {
		unk70->calcPosAndAt(&mPosition, &mTarget);
	} else {
		mTarget.x = gpCameraMario->mPosX;
		mTarget.y = gpCameraMario->mPosY;
		mTarget.z = gpCameraMario->mPosZ;
		f32 dist  = CLBLinearInbetween(unk68->unk08, unk68->unk0C, unkA8);
		CLBPolarToCross(mTarget, &mPosition, dist, unkA4, unkA6);
	}

	calcSecureViewTarget_(unkA6, &unk294, &unk298);
	mPosition.x += unk294;
	mPosition.z += unk298;
	mTarget.x += unk294;
	mTarget.z += unk298;

	unk80  = mPosition;
	unk98  = mPosition;
	unk8C  = mTarget;
	if (SMS_isOptionMap()) {
		unk80 = mPosition;
		unk8C = mTarget;
		gpCameraOption = new TCameraOption(mPosition, &unk8C);
	}

	unk256 = unkA4;
	unk258 = unkA6;
	unkB4  = unk80;
	unkC0  = unk8C;
	unkCC  = unk98;
	unkD8  = unkA4;
	unkDA  = unkA6;
	unkDC  = unkA8;
	unkE0  = unkAC;
	unkE4  = unkB0;
	unkE8  = unkB4;
	unkF4  = unkC0;
	unk100 = unkCC;
	unk10C = unkD8;
	unk10E = unkDA;
	unk110 = unkDC;
	unk114 = unkE0;
	unk118 = unkE4;

	unk124 = mPosition;
	unk130 = mPosition;
	unk13C = mPosition;
	unk148 = mTarget;
	unk154 = mTarget;
	unk160 = mTarget;

	if (unk64 & 0x1000) {
		*(JGeometry::TVec3<f32>*)((u8*)unk2B8 + 0x10) = mPosition;
		*(JGeometry::TVec3<f32>*)((u8*)unk2B8 + 0x1C) = mTarget;
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

	f32 dz = mPosition.z - mTarget.z;
	f32 dx = mPosition.x - mTarget.x;
	unk256 = matan(MsSqrtf(dx * dx + dz * dz), mPosition.y - mTarget.y);
	unk258 = matan(dz, dx);
	unk25C.set(unk148.x - unk124.x, unk148.y - unk124.y,
	           unk148.z - unk124.z);
	unk25C.setLength(unk25C, 1.0f);
	unk270 = MsClamp<f32>(
	    CLBCalcRatio<s16>(unk68->unk18, unk68->unk1A, unk256), 0.0f, 1.0f);

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
void CPolarSubCamera::startJetCoasterCam1() { }
