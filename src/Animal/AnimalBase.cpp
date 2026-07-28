#include <Animal/AnimalBase.hpp>
#include <MSound/MSoundBGM.hpp>
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
#include <Strategic/Strategy.hpp>
#include <M3DUtil/MActor.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DTransform.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JGadget/std-list.hpp>
#include <Map/Map.hpp>
#include <JSystem/JGeometry/JGUtil.hpp>
#include <dolphin/os/OSCache.h>
#include <stdlib.h>
#include <math.h>

JGeometry::TQuat4<f32> SMS_Eular2Quat(const JGeometry::TVec3<f32>&);

static inline f32 callMsWrap(f32 t, f32 l, f32 r)
{
	return MsWrap<f32>(t, l, r);
}

JGeometry::TQuat4<f32> SMS_Eular2Quat(const JGeometry::TVec3<f32>& rot)
{
	JGeometry::TQuat4<f32> qz;
	qz.setEulerZ(0.017453294f * rot.z);
	JGeometry::TQuat4<f32> qy;
	qy.setEulerY(0.017453294f * rot.y);
	(void)&qy;
	JGeometry::TQuat4<f32> qx;
	qx.setEulerX(0.017453294f * rot.x);

	JGeometry::TQuat4<f32> result2;
	result2.mul(qx, qz);
	JGeometry::TQuat4<f32> result;
	result.mul(qy, result2);
	return result;
}

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
	s32 count = stream.readS32() - 1;
	for (int i = 0; i < count; ++i) {
		TAnimalBase* animal = new TAnimalBase(getActorType(), getName());
		initNoLoad_(animal);
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
	if (flags & 1) {
		if (gfx->unk0 & 2) {
			mLinearVelocity.z = 0.0f;
			mLinearVelocity.y = 0.0f;
			mLinearVelocity.x = 0.0f;
			control();
			mPosition.x += mLinearVelocity.x;
			mPosition.y += mLinearVelocity.y;
			mPosition.z += mLinearVelocity.z;
			if (mActorType == 0x00800001
			    && gpMSound->gateCheck(0x3813)) {
				MSoundSESystem::MSRandPlay::startSeRandPlay(
				    0x3813, (u32)mInstanceIndex);
			}
		}
		flags &= ~1;
	}

	TAnimalManagerBase* mgr  = (TAnimalManagerBase*)mManager;
	TAnimalSaveIndividual* p = mgr->mAnimalSave;
	int sharedNum            = p->mSLSharedAnmNum.value;

	if (flags & 2) {
		updateAnmSound();
		mMActor->frameUpdate();
		if (!(mLiveFlag & 6)) {
			calcRootMatrix();
		}
		if ((sharedNum != 0 && mInstanceIndex < sharedNum)
		    || (sharedNum == 0 && !(mLiveFlag & 6)))
			mMActor->calc();
		flags &= ~2;
	}

	if (flags & 4) {
		if (!(mLiveFlag & 6)) {
			Mtx tmp1, tmp2, tmp3;
			PSMTXCopy(j3dSys.mViewMtx, tmp1);
			CLBCalcRotateZXYTranslateMatrix(tmp2, mRotation, mPosition);
			PSMTXConcat(tmp1, tmp2, tmp3);
			PSMTXCopy(tmp3, j3dSys.mViewMtx);

			if (sharedNum == 0 || mInstanceIndex < sharedNum) {
				mMActor->viewCalc();
			} else {
				TLiveActor* sharedAnimal
				    = mgr->getObj(mInstanceIndex % sharedNum);
				J3DModel* sharedModel = sharedAnimal->getModel();
				J3DModel* model       = getModel();
				J3DModelData* data    = model->getModelData();
				int count             = data->getDrawMtxNum();
				Mtx* sharedMtx[2];

				model->swapDrawMtx();
				model->swapNrmMtx();

				sharedMtx[0] = sharedModel->mNodeMatrices;
				sharedMtx[1] = sharedModel->unk5C;

				for (int i = 0; i < count; ++i) {
					u8 flag = data->getDrawMtxFlag(i);
					u16 index = data->getDrawMtxIndex(i);
					PSMTXConcat(tmp3, sharedMtx[flag][index],
					            model->getDrawMtx(i));
				}

				model->calcNrmMtx();
				DCStoreRange(model->getDrawMtxPtr(), count * sizeof(Mtx));
				DCStoreRange(model->getNrmMtxPtr(), count * sizeof(Mtx33));
				model->prepareShapePackets();
			}
			PSMTXCopy(tmp1, j3dSys.mViewMtx);
		}
		flags &= ~4;
	}

	TSpineEnemy::perform(flags, gfx);
}

BOOL TAnimalBase::receiveMessage(THitActor* sender, u32 msg)
{
	(void)sender;
	(void)msg;
	return 0;
}

void TAnimalBase::calcRootMatrix() { }

void TAnimalBase::execWalk(bool moving)
{
	TAnimalSaveIndividual* save = ((TAnimalManagerBase*)mManager)->mAnimalSave;

	if (moving) {
		f32 speed = save->mSLMaxMarchSpeed.get() * SMSGetAnmFrameRate();
		f32 accel = save->mSLMarchAccel.get() * SMSGetAnmFrameRate()
		            * SMSGetAnmFrameRate();
		CLBChaseGeneralConstantSpecifySpeed<f32>(&mMarchSpeed, speed, accel);
	} else {
		f32 decel = save->mSLMarchDecrease.get() * SMSGetAnmFrameRate()
		            * SMSGetAnmFrameRate();
		CLBChaseGeneralConstantSpecifySpeed<f32>(&mMarchSpeed, 0.0f, decel);
	}

	if (mMarchSpeed < 0.001f) {
		f32 waitSpeed = save->mSLWaitTurnSpeed.get();
		mTurnSpeed    = waitSpeed * SMSGetAnmFrameRate();
	} else {
		f32 walkSpeed = save->mSLWalkTurnSpeed.get();
		mTurnSpeed    = walkSpeed * SMSGetAnmFrameRate();
	}

	f32 turnSpeed  = mTurnSpeed;
	f32 marchSpeed = mMarchSpeed;

	JGeometry::TVec3<f32> diff = unkF4.getPoint();
	diff -= mPosition;

	f32 dist = diff.length();

	if (dist < 100.0f)
		return;

	if (dist <= 2.0f * calcMinimumTurnRadius(marchSpeed, turnSpeed))
		turnSpeed = calcTurnSpeedToReach(marchSpeed, 0.5f * dist);

	getRotationFlyToDir(&mRotation, diff, marchSpeed, turnSpeed);

	JGeometry::TQuat4<f32> quat = SMS_Eular2Quat(mRotation);
	JGeometry::TVec3<f32> tmp;
	quat.rotate(JGeometry::TVec3<f32>(0.0f, 0.0f, marchSpeed), tmp);
	mLinearVelocity = tmp;
}

void TAnimalBase::getRotationFlyToDir(JGeometry::TVec3<f32>* current_rot,
	                                  const JGeometry::TVec3<f32>& target_diff,
	                                  f32 speedX, f32 speedY)
{
	JGeometry::TVec3<f32> rot = MsGetRotFromZaxis(target_diff);
	rot.y                     = MsWrap<f32>(rot.y, 0.0f, 360.0f);

	f32 clampedDelta = JGeometry::TUtil<f32>::clamp(
	    MsAngleDiff(rot.y, current_rot->y), -speedY, speedY);

	current_rot->y += clampedDelta;
	current_rot->y = MsWrap<f32>(current_rot->y, 0.0f, 360.0f);

	f32 targetRoll = MsClamp<f32>(30.0f * -clampedDelta, -45.0f, 45.0f);
	CLBChaseGeneralConstantSpecifySpeed<f32>(&current_rot->z, targetRoll,
	                                         0.1f * speedX);

	rot.x          = MsWrap<f32>(rot.x, -180.0f, 180.0f);
	current_rot->x = MsWrap<f32>(current_rot->x, -180.0f, 180.0f);
	CLBChaseGeneralConstantSpecifySpeed<f32>(&current_rot->x, rot.x,
	                                         0.1f * speedX);
}

void TAnimalBase::resetRandomCurPathNode()
{
	TPathNode cur = unkF4;
	if (cur.unk0 != NULL)
		return;

	THitActor* curActor = cur.unk0;
	const JGeometry::TVec3<f32>* curPos;
	if (curActor != NULL)
		curPos = &curActor->getPosition();
	else
		curPos = &cur.unk4;
	JGeometry::TVec3<f32> point = *curPos;

	point.x += 1000.0f * (MsRandF() - 0.5f);
	point.z += 1000.0f * (MsRandF() - 0.5f);

	if (mActorType == 0x00800001) {
		point.y += 1000.0f
		           * (point.y <= 1000.0f ? MsRandF() : (MsRandF() - 0.5f));
	} else {
		point.y -= 250.0f * MsRandF();
	}

	setGoalPath(point);
}

void TAnimalBase::init(TLiveManager* mgr)
{
	mManager = mgr;
	mgr->manageActor(this);

	mMActorKeeper = new TMActorKeeper(mgr);
	mMActorKeeper->createMActorFromAllBmd(3);
	mMActor = mMActorKeeper->mActors[0];

	initHitActor(mActorType, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f);
	unk64 |= 1;

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
			frac = (1.0f / n) * (mInstanceIndex % n);
		}
		fc->setFrame(frac * (f32)fc->getEnd());
	}

	J3DFrameCtrl* fc3 = mMActor->getFrameCtrl(3);
	if (fc3) {
		fc3->setFrame((f32)fc3->getEnd() * MsRandF());
	}
}

void TAnimalBase::initNoLoad_(TAnimalBase* pNew)
{
	pNew->mPosition.x = 1000.0f * (MsRandF() - 0.5f) + mPosition.x;
	pNew->mPosition.z = 1000.0f * (MsRandF() - 0.5f) + mPosition.z;
	if (mActorType == 0x00800001) {
		pNew->mPosition.y = 1000.0f * MsRandF() + mPosition.y;
	} else {
		pNew->mPosition.y = mPosition.y - 250.0f * MsRandF();
	}

	pNew->mScaling = mScaling;
	pNew->mRotation.x = 0.0f;

	f32 newY = 150.0f * (MsRandF() - 0.5f) + mRotation.y;
	while (newY >= 360.0f)
		newY -= 360.0f;
	while (newY < 0.0f)
		newY += 360.0f;
	pNew->mRotation.y = newY;
	pNew->mRotation.z = 0.0f;

	pNew->unk3C            = unk3C;
	pNew->unk124->setGraph(unk124->getGraph());
	pNew->mGroundPlane     = TMap::getIllegalCheckData();
	pNew->init(mManager);

	JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ")
	    ->getChildren()
	    .push_back(pNew);
}
