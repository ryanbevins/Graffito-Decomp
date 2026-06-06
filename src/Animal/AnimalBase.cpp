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

JGeometry::TQuat4<f32> SMS_Eular2Quat(const JGeometry::TVec3<f32>& angles)
{
	f32 hz = 0.5f * (angles.z * (3.14159265f / 180.0f));
	f32 sz = sinf(hz);
	f32 cz = cosf(hz);

	f32 hy = 0.5f * (angles.y * (3.14159265f / 180.0f));
	f32 sy = sinf(hy);
	f32 cy = cosf(hy);

	f32 hx = 0.5f * (angles.x * (3.14159265f / 180.0f));
	f32 sx = sinf(hx);
	f32 cx = cosf(hx);

	JGeometry::TQuat4<f32> q;
	q.x = sx * cy * cz + cx * sy * sz;
	q.y = cx * sy * cz - sx * cy * sz;
	q.z = cx * cy * sz - sx * sy * cz;
	q.w = cx * cy * cz + sx * sy * sz;
	return q;
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
		if (sharedNum == 0) {
			if (!(mLiveFlag & 6)) {
				mMActor->calc();
			}
		} else if (mInstanceIndex < sharedNum) {
			mMActor->calc();
		}
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
				Mtx* sharedMtx[2];

				model->swapDrawMtx();
				model->swapNrmMtx();

				sharedMtx[0] = sharedModel->mNodeMatrices;
				sharedMtx[1] = sharedModel->unk5C;

				u16 count = data->getDrawMtxNum();
				for (u16 i = 0; i < count; ++i) {
					u8 flag = data->getDrawMtxFlag(i);
					u16 index = data->getDrawMtxIndex(i);
					PSMTXConcat(j3dSys.mViewMtx, sharedMtx[flag][index],
					            model->getDrawMtx(i));
				}

				model->calcNrmMtx();
				DCStoreRange(model->getDrawMtxPtr(), count * sizeof(Mtx));
				DCStoreRange(model->getNrmMtxPtr(), count * sizeof(Mtx33));
				model->prepareShapePackets();
			}
			PSMTXCopy(j3dSys.mViewMtx, tmp1);
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
	TAnimalSaveIndividual* save
	    = ((TAnimalManagerBase*)mManager)->mAnimalSave;

	if (moving) {
		f32 accel  = save->mSLMarchAccel.value * SMSGetAnmFrameRate();
		f32 chase  = accel * SMSGetAnmFrameRate();
		f32 target = save->mSLMaxMarchSpeed.value * SMSGetAnmFrameRate();
		CLBChaseGeneralConstantSpecifySpeed<f32>(&mMarchSpeed, target, chase);
	} else {
		f32 dec    = save->mSLMarchDecrease.value * SMSGetAnmFrameRate();
		f32 chase  = dec * SMSGetAnmFrameRate();
		CLBChaseGeneralConstantSpecifySpeed<f32>(&mMarchSpeed, 0.0f, chase);
	}

	if (mMarchSpeed < 0.001f) {
		mTurnSpeed = save->mSLWaitTurnSpeed.value * SMSGetAnmFrameRate();
	} else {
		mTurnSpeed = save->mSLWalkTurnSpeed.value * SMSGetAnmFrameRate();
	}

	JGeometry::TVec3<f32> diff;
	if (unkF4.unk0 != NULL) {
		diff.x = unkF4.unk0->mPosition.x;
		diff.y = unkF4.unk0->mPosition.y;
		diff.z = unkF4.unk0->mPosition.z;
	} else {
		diff.x = unkF4.unk4.x;
		diff.y = unkF4.unk4.y;
		diff.z = unkF4.unk4.z;
	}
	diff.x -= mPosition.x;
	diff.y -= mPosition.y;
	diff.z -= mPosition.z;

	f32 distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
	if (distSq <= 0.0f)
		return;

	f32 dist = JGeometry::TUtil<f32>::sqrt(distSq);
	if (dist < 100.0f)
		return;

	f32 radius = calcMinimumTurnRadius(mMarchSpeed, mTurnSpeed);
	if (dist <= 2.0f * radius) {
		mTurnSpeed = calcTurnSpeedToReach(mMarchSpeed, 0.5f * dist);
	}

	JGeometry::TVec3<f32> targetRot = MsGetRotFromZaxis(diff);
	f32 wrappedTargetY = MsWrap<f32>(targetRot.y, 0.0f, 360.0f);
	f32 currY          = MsWrap<f32>(mRotation.y, wrappedTargetY - 180.0f,
	                                  wrappedTargetY + 180.0f);

	f32 deltaY = wrappedTargetY - currY;
	f32 clampedDelta;
	if (deltaY < -mTurnSpeed)
		clampedDelta = -mTurnSpeed;
	else if (deltaY > mTurnSpeed)
		clampedDelta = mTurnSpeed;
	else
		clampedDelta = deltaY;

	mRotation.y += clampedDelta;
	mRotation.y = MsWrap<f32>(mRotation.y, 0.0f, 360.0f);

	f32 tilt = -clampedDelta * 30.0f;
	if (tilt > 45.0f)
		tilt = 45.0f;
	else if (tilt < -45.0f)
		tilt = -45.0f;
	f32 chaseSpeed = 0.1f * mTurnSpeed;
	CLBChaseGeneralConstantSpecifySpeed<f32>(&mRotation.z, tilt, chaseSpeed);

	f32 wrappedTargetX = MsWrap<f32>(targetRot.x, -180.0f, 180.0f);
	mRotation.x        = MsWrap<f32>(mRotation.x, -180.0f, 180.0f);
	CLBChaseGeneralConstantSpecifySpeed<f32>(&mRotation.x, wrappedTargetX,
	                                          chaseSpeed);

	JGeometry::TQuat4<f32> q = SMS_Eular2Quat(mRotation);
	JGeometry::TVec3<f32> velocity(0.0f, 0.0f, mMarchSpeed);
	q.rotate(velocity, velocity);
	mLinearVelocity = velocity;
}

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
void TAnimalBase::initNoLoad_(TAnimalBase* pNew)
{
	pNew->mPosition.x = mPosition.x + 1000.0f * (MsRandF() - 0.5f);
	pNew->mPosition.z = mPosition.z + 1000.0f * (MsRandF() - 0.5f);
	if (mActorType == 0x00800001) {
		pNew->mPosition.y = mPosition.y + 1000.0f * (MsRandF() - 0.5f);
	} else {
		pNew->mPosition.y = mPosition.y - 250.0f * MsRandF();
	}

	pNew->mScaling.x = mScaling.x;
	pNew->mScaling.y = mScaling.y;
	pNew->mScaling.z = mScaling.z;
	pNew->mRotation.x = 0.0f;

	f32 newY = 150.0f * (MsRandF() - 0.5f) + mRotation.y;
	while (newY >= 360.0f)
		newY -= 360.0f;
	while (newY < 0.0f)
		newY += 360.0f;
	pNew->mRotation.y = newY;
	pNew->mRotation.z = 0.0f;

	pNew->unk3C            = unk3C;
	pNew->unk124->unk0     = unk124->unk0;
	pNew->mGroundPlane     = TMap::getIllegalCheckData();
	pNew->init(mManager);

	JDrama::TViewObj* group = (JDrama::TViewObj*)
	    JDrama::TNameRefGen::instance->mRootNameRef->searchF(
	        JDrama::TNameRef::calcKeyCode("\x93\x47\x83\x4F\x83\x8B\x81\x5B\x83\x76"),
	        "\x93\x47\x83\x4F\x83\x8B\x81\x5B\x83\x76");
	JGadget::TList<void*>* list
	    = (JGadget::TList<void*>*)((char*)group + 0x10);
	list->insert(list->end(), (void*)pNew);
}
#pragma dont_inline off
