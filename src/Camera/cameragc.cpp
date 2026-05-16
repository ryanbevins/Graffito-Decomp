// cameragc.cpp -- main CPolarSubCamera implementation. Large TU (~12KB).
// Stub for portability; key functions need full decomp:
// - perform, ctrlGameCamera_, calcPosAndAt_, calcFinalPosAndAt_,
//   calcNowTargetFromPosAndAt_, calcSlopeAngleX_, rotateX/Y_ByStickXY_,
//   __ct__, ~CPolarSubCamera, loadAfter

#include <Camera/Camera.hpp>
#include <Player/MarioMain.hpp>
#include <Player/MarioAccess.hpp>
#include <M3DUtil/MActor.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>

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

void CPolarSubCamera::getFinalAngleZ() const { }
s16 CPolarSubCamera::getOffsetAngleY() const { return 0; }
void CPolarSubCamera::getOffsetAngleX() const { }

void CPolarSubCamera::ctrlGameCamera_() { }
void CPolarSubCamera::calcFinalPosAndAt_() { }
void CPolarSubCamera::calcPosAndAt_() { }
void CPolarSubCamera::calcSlopeAngleX_(s16* out) { (void)out; }

bool CPolarSubCamera::isMomentDefinite_() const { return false; }
void CPolarSubCamera::execInvalidAutoChase_() { }
bool CPolarSubCamera::isMarioCrabWalk_() const { return false; }
bool CPolarSubCamera::isMarioAimWithGun_() const { return false; }
bool CPolarSubCamera::isMarioReadyGun_() const { return false; }

void CPolarSubCamera::onMoveApproach_() { }
void CPolarSubCamera::offMoveApproach_() { }

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
bool CPolarSubCamera::isNowInbetween() const { return false; }

void CPolarSubCamera::loadAfter() { }

void CPolarSubCamera::calcExternalData_() { }
void CPolarSubCamera::setMarioLookat_() { }
void CPolarSubCamera::startJetCoasterCam1() { }
