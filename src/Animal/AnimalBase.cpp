#include <Animal/AnimalBase.hpp>
#include <Enemy/PathNode.hpp>
#include <Strategic/SolidStack.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <Camera/cameralib.hpp>
#include <Animal/AnimalManager.hpp>
#include <Animal/AnimalSave.hpp>
#include <Animal/AnimalNerve.hpp>
#include <Enemy/Graph.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/ObjModel.hpp>
#include <M3DUtil/MActor.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <stdlib.h>

extern "C" {
f32 SMSGetAnmFrameRate();
}

TAnimalBase::TAnimalBase(u32 type, const char* name)
    : TSpineEnemy(name)
{
	mActorType = type;
}

void TAnimalBase::load(JSUMemoryInputStream& stream)
{
	TSpineEnemy::load(stream);
	int count;
	stream.read(&count, 4);
	int n = count - 1;
	for (int i = 0; i < n; i++) {
		TAnimalBase* clone = new TAnimalBase(mActorType, mName);
		initNoLoad_(clone);
	}
}

void TAnimalBase::loadAfter()
{
	JDrama::TNameRef::loadAfter();
	if (mActorType == 0x00800001) {
		MSoundSESystem::MSRandPlay::registerTrans(0x3813, &mPosition);
	}
}

void TAnimalBase::perform(u32 flags, JDrama::TGraphics* gfx)
{
	(void)flags;
	(void)gfx;
}

BOOL TAnimalBase::receiveMessage(THitActor* sender, u32 msg)
{
	(void)sender;
	(void)msg;
	return 0;
}

void TAnimalBase::calcRootMatrix() { }

void TAnimalBase::execWalk(bool flag) { (void)flag; }

void TAnimalBase::getRotationFlyToDir(JGeometry::TVec3<f32>* outRot,
                                      const JGeometry::TVec3<f32>& dir,
                                      f32 turnSpeed, f32 maxYawDelta)
{
	JGeometry::TVec3<f32> targetRot = MsGetRotFromZaxis(dir);
	JGeometry::TVec3<f32> wrapped   = targetRot;

	wrapped.y = MsWrap<f32>(wrapped.y, 0.0f, 360.0f);
	outRot->y = MsWrap<f32>(outRot->y, wrapped.y - 180.0f, wrapped.y + 180.0f);

	f32 delta = wrapped.y - outRot->y;
	f32 clampedDelta;
	if (delta < -maxYawDelta)
		clampedDelta = -maxYawDelta;
	else if (delta > maxYawDelta)
		clampedDelta = maxYawDelta;
	else
		clampedDelta = delta;

	outRot->y += clampedDelta;
	outRot->y = MsWrap<f32>(outRot->y, 0.0f, 360.0f);

	f32 targetTilt = -clampedDelta * 30.0f;
	if (targetTilt > 45.0f)
		targetTilt = 45.0f;
	else if (targetTilt < -45.0f)
		targetTilt = -45.0f;

	f32 chaseSpeed = 0.1f * turnSpeed;
	CLBChaseGeneralConstantSpecifySpeed<f32>(&outRot->z, targetTilt, chaseSpeed);

	wrapped.x = MsWrap<f32>(wrapped.x, -180.0f, 180.0f);
	outRot->x = MsWrap<f32>(outRot->x, -180.0f, 180.0f);
	CLBChaseGeneralConstantSpecifySpeed<f32>(&outRot->x, wrapped.x, chaseSpeed);
}

void TAnimalBase::resetRandomCurPathNode()
{
	TPathNode cur = unkF4;
	if (cur.unk0 != NULL)
		return;

	JGeometry::TVec3<f32> point = cur.getPoint();

	point.x += 1000.0f * (MsRandF() - 0.5f);
	point.z += 1000.0f * (MsRandF() - 0.5f);

	if (mActorType == 0x00800001) {
		if (point.y <= 1000.0f)
			point.y += 1000.0f * MsRandF();
		else
			point.y += 1000.0f * (MsRandF() - 0.5f);
	} else {
		point.y -= 250.0f * MsRandF();
	}

	TPathNode newNode(point);
	unkF4    = newNode;
	unk104   = newNode;
	unk114.clear();
}

void TAnimalBase::init(TLiveManager* mgr)
{
	mManager = mgr;
	mgr->manageActor(this);

	mMActorKeeper = new TMActorKeeper(mgr);
	mMActorKeeper->createMActorFromAllBmd(3);
	mMActor = mMActorKeeper->mActors[0];

	initHitActor(mActorType, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f);
	onHitFlag(1);

	mBodyScale  = 1.0f;
	mMarchSpeed = 0.0f;
	mBodyRadius = 10.0f;
	mWallRadius = 20.0f;

	mLiveFlag |= 0x38;

	mSpine->initWith(&TNerveAnimalGraphWander::theNerve());

	unk124->mPrevIdx = -1;
	goToShortestNextGraphNode();

	mFrameTimer = new int[2];
	if (mFrameTimer) {
		mFrameTimer[0] = 0;
		mFrameTimer[1] = 1;
	}

	mMActor->setBckFromIndex(0);
	mMarchSpeed = 0.0f;

	TAnimalSaveIndividual* save
	    = ((TAnimalManagerBase*)mManager)->mAnimalSave;
	mTurnSpeed = save->mSLWalkTurnSpeed.value * SMSGetAnmFrameRate();

	J3DFrameCtrl* fc = mMActor->getFrameCtrl(0);
	if (fc) {
		f32 frac;
		int n = save->mSLSharedAnmNum.value;
		if (n == 0) {
			frac = MsRandF();
		} else {
			frac = (f32)(mInstanceIndex % n) / (f32)n;
		}
		fc->setFrame(frac * (f32)fc->getEnd());
	}

	J3DFrameCtrl* fc3 = mMActor->getFrameCtrl(3);
	if (fc3) {
		fc3->setFrame((f32)fc3->getEnd() * MsRandF());
	}
}

#pragma dont_inline on
void TAnimalBase::initNoLoad_(TAnimalBase*) { }
#pragma dont_inline off
