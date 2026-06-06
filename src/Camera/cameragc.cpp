// cameragc.cpp -- main CPolarSubCamera implementation. Large TU (~12KB).
// Stub for portability; key functions need full decomp:
// - perform, ctrlGameCamera_, calcPosAndAt_, calcFinalPosAndAt_,
//   calcNowTargetFromPosAndAt_, calcSlopeAngleX_, rotateX/Y_ByStickXY_,
//   __ct__, ~CPolarSubCamera, loadAfter

#include <Camera/Camera.hpp>
#include <Camera/CameraInbetween.hpp>
#include <Camera/CameraKindParam.hpp>
#include <Camera/CameraMarioData.hpp>
#include <Camera/CameraShake.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/JMath.hpp>
#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <Player/MarioMain.hpp>
#include <Player/MarioAccess.hpp>
#include <M3DUtil/MActor.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <System/MarioGamePad.hpp>

template <> f32 CLBLinearInbetween<f32>(f32, f32, f32);
template <> f32 CLBCalcRatio<s16>(s16, s16, s16);
template <> BOOL CLBChaseGeneralConstantSpecifySpeed<s16>(s16*, s16, s16);

CPolarSubCamera::CPolarSubCamera(const char* name)
    : JDrama::TLookAtCamera()
{
	(void)name;
	// TODO: full ctor
}

CPolarSubCamera::~CPolarSubCamera() { }

void CPolarSubCamera::perform(u32 flags, JDrama::TGraphics* gfx)
{
	(void)flags;
	(void)gfx;
}

s16 CPolarSubCamera::getFinalAngleZ() const
{
	return unk254 + gpCameraShake->mYaw;
}
s16 CPolarSubCamera::getOffsetAngleY() const { return unk68->unk5A; }
s16 CPolarSubCamera::getOffsetAngleX() const { return unk68->unk58; }

void CPolarSubCamera::ctrlGameCamera_() { }
void CPolarSubCamera::calcFinalPosAndAt_() { }
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

void CPolarSubCamera::loadAfter() { }

void CPolarSubCamera::calcExternalData_() { }
void CPolarSubCamera::setMarioLookat_() { }
void CPolarSubCamera::startJetCoasterCam1() { }
