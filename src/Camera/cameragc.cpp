// cameragc.cpp -- main CPolarSubCamera implementation. Large TU (~12KB).
// Stub for portability; key functions need full decomp:
// - perform, ctrlGameCamera_, calcPosAndAt_, calcFinalPosAndAt_,
//   calcNowTargetFromPosAndAt_, calcSlopeAngleX_, rotateX/Y_ByStickXY_,
//   __ct__, ~CPolarSubCamera, loadAfter

#include <Camera/Camera.hpp>
#include <Camera/CameraInbetween.hpp>
#include <Camera/CameraKindParam.hpp>
#include <Camera/CameraShake.hpp>
#include <Camera/cameralib.hpp>
#include <Player/MarioMain.hpp>
#include <Player/MarioAccess.hpp>
#include <M3DUtil/MActor.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>

template <> f32 CLBLinearInbetween<f32>(f32, f32, f32);

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
void CPolarSubCamera::calcSlopeAngleX_(s16* out) { (void)out; }

bool CPolarSubCamera::isMomentDefinite_() const
{
	bool result = false;
	if (!(unk64 & 0x100) && isNormalCameraCompletely() && unk250 > 0.0f
	    && CLBLinearInbetween(unk68->unk10, unk68->unk14, unkA8) > 0.0f)
		result = true;
	return result;
}

void CPolarSubCamera::execInvalidAutoChase_() { unk284 = unk68->unk68; }
bool CPolarSubCamera::isMarioCrabWalk_() const { return false; }
bool CPolarSubCamera::isMarioAimWithGun_() const { return false; }
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

void CPolarSubCamera::rotateY_ByStickX_(f32 stick) { (void)stick; }
void CPolarSubCamera::rotateX_ByStickY_(f32 stick) { (void)stick; }

void CPolarSubCamera::calcNowTargetFromPosAndAt_(const Vec& pos, const Vec& at)
{
	(void)pos;
	(void)at;
}

f32 CPolarSubCamera::calcDistFromXRotRatio_() const { return 0.0f; }
s16 CPolarSubCamera::calcAngleXFromXRotRatio_() const { return 0; }

JGeometry::TVec3<f32> CPolarSubCamera::getUsualLookat() const
{
	JGeometry::TVec3<f32> v(0.0f, 0.0f, 0.0f);
	return v;
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
