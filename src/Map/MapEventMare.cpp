#include <Map/MapEventMare.hpp>
#include <Camera/CameraShake.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JDrama/JDRViewObjPtrList.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <MSound/MSSetSound.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <Map/JointObj.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <Map/PollutionManager.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/Particles.hpp>
#include <stdio.h>

f32 TMareWallRock::mAppearSpeed        = 3.0f;
f32 TMareWallRock::mDepressSpeed       = 3.0f;
u32 TMareWallRock::mCleanedDegree      = 50;
int TMareWallRock::mWaitTimeToAppear   = 240;
int TMareWallRock::mWaitTimeToDepress  = 1200;
f32 TMareEventDepressWall::mDepressSpeed = 1.0f;
f32 TMareEventDepressWall::mRiseSpeed    = 5.0f;
int TMareEventDepressWall::mWaitTimeToWatch = 120;

void TMareWallRock::appear()
{
	mCollisions[0]->setUp();
	mJointObj->awake();
	JGeometry::TVec3<f32> trans(0.0f, 0.0f, mSinkDepth);
	mCollisions[0]->moveTrans(trans);
	f32 rotY = mEffectRotY;
	f32 zero = 0.0f;

	JPABaseEmitter* emitter
	    = gpMarioParticleManager->emit(0x69, &mEffectPos, 0, &mEffectPos);
	if (emitter != nullptr) {
		emitter->setRotation((s16)zero, (s16)rotY, (s16)zero);
		emitter->unk154.x = mEffectScale.x;
		emitter->unk154.y = mEffectScale.y;
		emitter->unk154.z = mEffectScale.z;
	}

	emitter = gpMarioParticleManager->emit(0x1e5, &mEffectPos, 2,
	                                       &mEffectPos);
	if (emitter != nullptr) {
		emitter->setRotation((s16)zero, (s16)rotY, (s16)zero);
		emitter->unk154.x = mEffectScale.x;
		emitter->unk154.y = mEffectScale.y;
		emitter->unk154.z = mEffectScale.z;
	}

	mState = 2;
}

void TMareWallRock::movement()
{
	switch (mState) {
	case 0:
		if (gpPollution->getLayer(mLayerIndex)->getPollutionDegree()
		    < mCleanedDegree) {
			appear();
			gpPollution->offLayer(mLayerIndex);
		}
		break;
	case 2: {
		J3DJoint* joint              = mJointObj->getJoint();
		J3DTransformInfo& transform = joint->getTransformInfo();
		if (!TMapObjBase::isDemo()) {
			transform.mTranslate.z -= mAppearSpeed;
			if (TMapObjBase::marioIsOn(this))
				mPosition.z -= mAppearSpeed;

			if (gpMSound->gateCheck(0x3008)) {
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x3008, &mPosition, 0, nullptr, 0, 4);
			}
			SMSRumbleMgr->start(0x13, -1, (f32*)nullptr);
			gpCameraShake->keepShake(CAM_SHAKE_MODE_UNK5, 0.5f);
		}
		mJointObj->getJoint()->setTransformInfo(transform);
		gpMap->getModelManager()->getJointModel(0)->getModel()->calc();
		f32 z = transform.mTranslate.z;
		if (z < 0.0f) {
			mTimer                = mWaitTimeToDepress;
			mCollisions[1]->setUp();
			mState = 1;
			SMSRumbleMgr->stop(0x13);
			return;
		}
		JGeometry::TVec3<f32> trans(0.0f, 0.0f, z);
		mCollisions[0]->moveTrans(trans);
		break;
	}
	case 3:
		if (!TMapObjBase::isDemo()) {
			--mTimer;
			if (mTimer <= 0)
				appear();
		}
		break;
	case 4: {
		J3DJoint* joint              = mJointObj->getJoint();
		J3DTransformInfo& transform = joint->getTransformInfo();
		if (!TMapObjBase::isDemo()) {
			transform.mTranslate.z += mDepressSpeed;
			if (TMapObjBase::marioIsOn(this))
				mPosition.z += mDepressSpeed;
		}
		mJointObj->getJoint()->setTransformInfo(transform);
		gpMap->getModelManager()->getJointModel(0)->getModel()->calc();
		f32 z = transform.mTranslate.z;
		if (z > mSinkDepth) {
			mCollisions[0]->remove();
			mJointObj->sleep();
			mTimer = mWaitTimeToAppear;
			mState = 3;
			return;
		}
		JGeometry::TVec3<f32> trans(0.0f, 0.0f, z);
		mCollisions[0]->moveTrans(trans);
		break;
	}
	}
}

void TMareWallRock::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 1)
		movement();

	THitActor::perform(flags, graphics);
}

void TMareWallRock::initEffect()
{
	switch (mIndex + 1) {
	case 1:
		mEffectScale.set(1.3f, 4.0f, 1.0f);
		mEffectPos.set(-7085.0f, 1390.0f, 3600.0f);
		break;
	case 2:
		mEffectScale.set(1.0f, 1.0f, 1.0f);
		mEffectPos.set(-8300.0f, 2650.0f, 4300.0f);
		break;
	case 3:
		mEffectScale.set(0.9f, 1.2f, 1.0f);
		mEffectPos.set(-7120.0f, 3430.0f, 3820.0f);
		break;
	case 4:
		mEffectScale.set(2.3f, 1.0f, 1.0f);
		mEffectPos.set(-5150.0f, 4700.0f, 4100.0f);
		break;
	case 5:
		mEffectScale.set(1.1f, 0.6f, 1.0f);
		mEffectPos.set(-7606.0f, 5480.0f, 5400.0f);
		break;
	case 6:
		mEffectScale.set(1.4f, 1.6f, 1.0f);
		mEffectPos.set(-5000.0f, 7160.0f, 4950.0f);
		break;
	case 7:
		mEffectScale.set(1.3f, 1.3f, 1.0f);
		mEffectPos.set(-6880.0f, 7780.0f, 5300.0f);
		break;
	}
}

void TMareWallRock::loadAfter()
{
	JDrama::TNameRef::loadAfter();

	mCollisions    = new TMapCollisionBase*[2];
	mCollisions[0] = new TMapCollisionMove;
	mCollisions[1] = new TMapCollisionWarp;

	char path[256];
	snprintf(path, sizeof(path), "/map/map/building%02d", mIndex + 1);
	mCollisions[0]->init(path, 0, this);
	mCollisions[1]->init(path, 0, this);

	mJointObj   = TMapObjBase::getBuildingJointObj(mIndex + 1);
	mLayerIndex = mIndex;

	J3DJoint* joint = mJointObj->getJoint();
	JGeometry::TVec3<f32> min = joint->mMin;
	JGeometry::TVec3<f32> max = joint->mMax;
	mPosition.x             = (min.x + max.x) * 0.5f;
	mPosition.y             = (min.y + max.y) * 0.5f;
	mPosition.z             = (min.z + max.z) * 0.5f;
	mSinkDepth              = 100.0f + (max.z - min.z);

	TMapObjBase::moveJoint(joint, 0.0f, 0.0f, mSinkDepth);
	mJointObj->sleep();

	initHitActor(0x4000022c, 1, 0, 0.0f, 0.0f, 0.0f, 0.0f);
	initEffect();
}

void TMareWallRock::load(JSUMemoryInputStream& stream)
{
	JDrama::TActor::load(stream);
}

TMareWallRock::TMareWallRock()
    : TLiveActor("マーレ壁の岩")
    , mState(0)
    , mIndex(0)
    , mTimer(0)
    , mJointObj(nullptr)
    , mLayerIndex(0)
    , mCollisions(nullptr)
{
	mEffectRotY = 0.0f;
	mEffectScale.set(1.0f, 1.0f, 1.0f);
	mEffectPos.zero();
}

void TMareEventWallRock::load(JSUMemoryInputStream& stream)
{
	JDrama::TNameRef::load(stream);

	mRocks = new TMareWallRock[mRockNum];

	JDrama::TViewObjPtrListT<JDrama::TViewObj>* group
	    = JDrama::TNameRefGen::search<
	        JDrama::TViewObjPtrListT<JDrama::TViewObj> >("マップグループ");

	for (int i = 0; i < mRockNum; ++i) {
		mRocks[i].mIndex = i;
		group->getChildren().push_back((JDrama::TViewObj*)&mRocks[i]);
	}
}

TMareEventWallRock::TMareEventWallRock(const char* name)
    : JDrama::TViewObj(name)
    , mRockNum(7)
    , mRocks(nullptr)
{
}

void TMareEventDepressWall::rising()
{
	f32 current
	    = TMapObjBase::getJointTransX(mJoints[mCurrentIndex]);
	JGeometry::TVec3<f32>* pos = &mPositions[mCurrentIndex];

	if (gpMSound->gateCheck(0x3008)) {
		MSoundSESystem::MSoundSE::startSoundActor(0x3008, pos, 0,
		                                          nullptr, 0, 4);
	}

	JPABaseEmitter* emitter = gpMarioParticleManager->emit(
	    0x15b, &mPositions[mCurrentIndex], 1, &mPositions[mCurrentIndex]);
	if (emitter != nullptr) {
		JGeometry::TVec3<f32>* effectDir = &mEffectDirs[mCurrentIndex];
		emitter->unk154.x                = effectDir->x;
		emitter->unk154.y                = effectDir->y;
		emitter->unk154.z                = effectDir->z;
		emitter->unk174.x                = effectDir->x;
		emitter->unk174.y                = effectDir->y;
		emitter->unk174.z                = effectDir->z;
		emitter->mChildSpawnRate = mParticleScales[mCurrentIndex];
		f32 childRate            = mParticleChildRates[mCurrentIndex];
		emitter->unk174.x        = childRate;
		emitter->unk174.y        = childRate;
		emitter->unk174.z        = childRate;
	}

	if (mDirections[mCurrentIndex]) {
		if (current > 0.0f) {
			current -= mRiseSpeed;
			TMapObjBase::setJointTransX(mJoints[mCurrentIndex], current);
			JGeometry::TVec3<f32> trans(current, 0.0f, 0.0f);
			mMoveCollisions[mCurrentIndex].moveTrans(trans);
			return;
		}

		JGeometry::TVec3<f32> trans(0.0f, 0.0f, 0.0f);
		mMoveCollisions[mCurrentIndex].remove();
		mWarpCollisions[mCurrentIndex].setUpTrans(trans);
		TMapObjBase::setJointTransX(mJoints[mCurrentIndex], 0.0f);
		SMSRumbleMgr->stop(0x13);

		++mCurrentIndex;
		if (mCurrentIndex == mWallNum) {
			mWaitTimer = mWaitTimeToWatch;
			mState     = 4;
		} else {
			mWaitTimer = mWaitTimes[mCurrentIndex];
			mState     = 2;
		}
	} else {
		if (current < 0.0f) {
			current += mRiseSpeed;
			TMapObjBase::setJointTransX(mJoints[mCurrentIndex], current);
			JGeometry::TVec3<f32> trans(current, 0.0f, 0.0f);
			mMoveCollisions[mCurrentIndex].moveTrans(trans);
			return;
		}

		JGeometry::TVec3<f32> trans(0.0f, 0.0f, 0.0f);
		mMoveCollisions[mCurrentIndex].remove();
		mWarpCollisions[mCurrentIndex].setUpTrans(trans);
		TMapObjBase::setJointTransX(mJoints[mCurrentIndex], 0.0f);
		SMSRumbleMgr->stop(0x13);

		++mCurrentIndex;
		if (mCurrentIndex == mWallNum) {
			mWaitTimer = mWaitTimeToWatch;
			mState     = 4;
		} else {
			mWaitTimer = mWaitTimes[mCurrentIndex];
			mState     = 2;
		}
	}
}

void TMareEventDepressWall::depressing()
{
	int finished = 0;

	for (int i = 0; i < mWallNum; ++i) {
		f32 current = TMapObjBase::getJointTransX(mJoints[i]);
		if (mDirections[i]) {
			if (current < mTargets[i]) {
				if (!TMapObjBase::isDemo()) {
					current += mDepressSpeed;

					SMSRumbleMgr->start(0x13, -1, (f32*)nullptr);
					gpCameraShake->keepShake(CAM_SHAKE_MODE_UNK5, 0.5f);
					JGeometry::TVec3<f32>* pos = &mPositions[i];
					if (gpMSound->gateCheck(0x3008)) {
						MSoundSESystem::MSoundSE::startSoundActor(
						    0x3008, pos, 0, nullptr, 0, 4);
					}

					JPABaseEmitter* emitter = gpMarioParticleManager->emit(
					    0x15b, &mPositions[i], 1, &mPositions[i]);
					if (emitter != nullptr) {
						JGeometry::TVec3<f32>* effectDir = &mEffectDirs[i];
						emitter->unk154.x                = effectDir->x;
						emitter->unk154.y                = effectDir->y;
						emitter->unk154.z                = effectDir->z;
						emitter->unk174.x                = effectDir->x;
						emitter->unk174.y                = effectDir->y;
						emitter->unk174.z                = effectDir->z;
						emitter->mChildSpawnRate = mParticleScales[i];
						f32 childRate            = mParticleChildRates[i];
						emitter->unk174.x        = childRate;
						emitter->unk174.y        = childRate;
						emitter->unk174.z        = childRate;
					}
				} else {
					SMSRumbleMgr->stop(0x13);
				}

				TMapObjBase::setJointTransX(mJoints[i], current);
				JGeometry::TVec3<f32> trans(current, 0.0f, 0.0f);
				mMoveCollisions[i].moveTrans(trans);

				if (current >= mTargets[i]) {
					mMoveCollisions[i].remove();
					JGeometry::TVec3<f32> trans2(current, 0.0f, 0.0f);
					mWarpCollisions[i].setUpTrans(trans2);
					++finished;
				}
			} else {
				++finished;
			}
		} else {
			if (current > -mTargets[i]) {
				if (!TMapObjBase::isDemo()) {
					current -= mDepressSpeed;

					SMSRumbleMgr->start(0x13, -1, (f32*)nullptr);
					gpCameraShake->keepShake(CAM_SHAKE_MODE_UNK5, 0.5f);
					JGeometry::TVec3<f32>* pos = &mPositions[i];
					if (gpMSound->gateCheck(0x3008)) {
						MSoundSESystem::MSoundSE::startSoundActor(
						    0x3008, pos, 0, nullptr, 0, 4);
					}

					JPABaseEmitter* emitter = gpMarioParticleManager->emit(
					    0x15b, &mPositions[i], 1, &mPositions[i]);
					if (emitter != nullptr) {
						JGeometry::TVec3<f32>* effectDir = &mEffectDirs[i];
						emitter->unk154.x                = effectDir->x;
						emitter->unk154.y                = effectDir->y;
						emitter->unk154.z                = effectDir->z;
						emitter->unk174.x                = effectDir->x;
						emitter->unk174.y                = effectDir->y;
						emitter->unk174.z                = effectDir->z;
						emitter->mChildSpawnRate = mParticleScales[i];
						f32 childRate            = mParticleChildRates[i];
						emitter->unk174.x        = childRate;
						emitter->unk174.y        = childRate;
						emitter->unk174.z        = childRate;
					}
				} else {
					SMSRumbleMgr->stop(0x13);
				}

				TMapObjBase::setJointTransX(mJoints[i], current);
				JGeometry::TVec3<f32> trans(current, 0.0f, 0.0f);
				mMoveCollisions[i].moveTrans(trans);

				if (current <= -mTargets[i]) {
					mMoveCollisions[i].remove();
					JGeometry::TVec3<f32> trans2(current, 0.0f, 0.0f);
					mWarpCollisions[i].setUpTrans(trans2);
					++finished;
				}
			} else {
				++finished;
			}
		}
	}

	if (finished == mWallNum) {
		if (gpMSound->gateCheck(0x484d)) {
			MSoundSESystem::MSoundSE::startSoundSystemSE(0x484d, 0,
			                                             nullptr, 0);
		}
		SMSRumbleMgr->stop(0x13);
		mCurrentIndex = 0;
		mWaitTimer    = mWaitTimes[mCurrentIndex];
		mState        = 2;
	}
}

void TMareEventDepressWall::perform(u32 flags, JDrama::TGraphics*)
{
	if (!(flags & 1))
		return;

	switch (mState) {
	case 0:
		break;
	case 1:
		depressing();
		((TMapObjBase*)this)->calcMap();
		break;
	case 2:
		if (!TMapObjBase::isDemo()) {
			if (mWaitTimer == 0) {
				mMoveCollisions[mCurrentIndex].setUp();
				mWarpCollisions[mCurrentIndex].remove();
				mState = 3;
			} else {
				--mWaitTimer;
			}
		}
		break;
	case 3:
		rising();
		((TMapObjBase*)this)->calcMap();
		break;
	case 4:
		if (!TMapObjBase::isDemo()) {
			if (mWaitTimer == 0)
				mState = 0;
			else
				--mWaitTimer;
		}
		break;
	}
}

bool TMareEventDepressWall::startEvent()
{
	if (mState != 0)
		return false;

	for (int i = 0; i < mWallNum; ++i) {
		mMoveCollisions[i].setUp();
		mWarpCollisions[i].remove();
	}

	mState = 1;
	return true;
}

void TMareEventDepressWall::initCommon()
{
	mWaitTimes          = new int[mWallNum];
	mTargets            = new f32[mWallNum];
	mDirections         = new bool[mWallNum];
	for (int i = 0; i < mWallNum; ++i) {
		mTargets[i]    = 400.0f;
		mDirections[i] = true;
	}

	mWarpCollisions     = new TMapCollisionWarp[mWallNum];
	mMoveCollisions     = new TMapCollisionMove[mWallNum];
	for (int i = 0; i < mWallNum; ++i) {
		int buildingIndex = mStartBuildingIndex + i;
		char path[256];
		snprintf(path, sizeof(path), "/scene/map/map/building%02d.col",
		         buildingIndex);

		mWarpCollisions[i].init(path, 0, nullptr);
		mWarpCollisions[i].setUp();
		mMoveCollisions[i].init(path, 0, nullptr);
		mMoveCollisions[i].setUp();
		mMoveCollisions[i].remove();
	}

	mJoints             = new J3DJoint*[mWallNum];
	mPositions          = new JGeometry::TVec3<f32>[mWallNum];
	mEffectDirs         = new JGeometry::TVec3<f32>[mWallNum];
	mParticleScales     = new f32[mWallNum];
	mParticleChildRates = new f32[mWallNum];
	int skip            = 0x43 - (mStartBuildingIndex + (mWallNum - 1));
	J3DJoint* joint     = (J3DJoint*)gpMap->getModelManager()
	                      ->getJointModel(0)
	                      ->getModelData()
	                      ->getJointNodePointer(0)
	                      ->getChild()
	                      ->getYounger()
	                      ->getChild();
	for (int i = 0; i < skip; ++i)
		joint = (J3DJoint*)joint->getYounger();

	for (int i = 0; i < mWallNum; ++i) {
		int index      = mWallNum - 1 - i;
		mJoints[index] = joint;
		mPositions[index].x = (joint->mMin.x + joint->mMax.x) * 0.5f;
		mPositions[index].y = (joint->mMin.y + joint->mMax.y) * 0.5f;
		mPositions[index].z = (joint->mMin.z + joint->mMax.z) * 0.5f;

		mEffectDirs[index].x = (joint->mMax.x - joint->mMin.x) / 100.0f;
		mEffectDirs[index].y = (joint->mMax.y - joint->mMin.y) / 100.0f;
		mEffectDirs[index].z = (joint->mMax.z - joint->mMin.z) / 100.0f;

		f32 volume = mEffectDirs[index].x * mEffectDirs[index].y
		             * mEffectDirs[index].z;
		f32 root = JGeometry::TUtil<f32>::sqrt(volume);

		mParticleScales[index]     = root * 0.1f;
		mParticleChildRates[index] = volume * 0.002f + 1.5f;
		joint = (J3DJoint*)joint->getYounger();
	}

	SMS_LoadParticle("/scene/map/map/ms_mare_blockup.jpa", 0x15b);
}

void TMareEventDepressWall::init3rdEvent()
{
	mWallNum            = 7;
	mStartBuildingIndex = 0x2b;
	initCommon();

	TMareEventDepressWall* event
	    = JDrama::TNameRefGen::search<TMareEventDepressWall>("mareEP2");
	if (event != nullptr) {
		*(TMareEventDepressWall**)((u8*)event + 0x68) = this;

		mWaitTimes[0] = 2400;
		for (int i = 1; i < mWallNum; ++i)
			mWaitTimes[i] = 120;

		mTargets[0x2b - mStartBuildingIndex]    = 900.0f;
		mTargets[0x2c - mStartBuildingIndex]    = 1300.0f;
		mTargets[0x2d - mStartBuildingIndex]    = 1300.0f;
		mTargets[0x2e - mStartBuildingIndex]    = 1300.0f;
		mTargets[0x2f - mStartBuildingIndex]    = 400.0f;
		mTargets[0x30 - mStartBuildingIndex]    = 300.0f;
		mTargets[0x31 - mStartBuildingIndex]    = 200.0f;
		mDirections[0x2f - mStartBuildingIndex] = false;
		mDirections[0x30 - mStartBuildingIndex] = false;
		mDirections[0x31 - mStartBuildingIndex] = false;
	}
}

void TMareEventDepressWall::init2ndEvent()
{
	mWallNum            = 15;
	mStartBuildingIndex = 0x13;
	initCommon();

	TMareEventDepressWall* event
	    = JDrama::TNameRefGen::search<TMareEventDepressWall>("mareEP1");
	if (event != nullptr) {
		*(TMareEventDepressWall**)((u8*)event + 0x68) = this;

		mWaitTimes[0] = 3600;
		for (int i = 1; i < mWallNum; ++i)
			mWaitTimes[i] = 120;
	}
}

void TMareEventDepressWall::init1stEvent()
{
	mWallNum            = 11;
	mStartBuildingIndex = 8;
	initCommon();

	TMareEventDepressWall* event
	    = JDrama::TNameRefGen::search<TMareEventDepressWall>("mareEP0");
	if (event != nullptr) {
		*(TMareEventDepressWall**)((u8*)event + 0x68) = this;

		mWaitTimes[0] = 3600;
		for (int i = 1; i < mWallNum; ++i)
			mWaitTimes[i] = 120;
	}
}

TMareEventDepressWall::TMareEventDepressWall(const char* name)
    : JDrama::TViewObj(name)
    , mWallNum(0)
    , mStartBuildingIndex(0)
    , mWaitTimes(nullptr)
    , mDirections(nullptr)
    , mTargets(nullptr)
    , mWarpCollisions(nullptr)
    , mMoveCollisions(nullptr)
    , mJoints(nullptr)
    , mPositions(nullptr)
    , mEffectDirs(nullptr)
    , mParticleScales(nullptr)
    , mParticleChildRates(nullptr)
    , mState(0)
    , mCurrentIndex(0)
    , mWaitTimer(0)
{
}

u32 TMareEventBumpyWall::touchWater(THitActor*)
{
	switch (mBumpDirection) {
	case 0:
		mState = 2;
		break;
	case 2:
		mState = 3;
		break;
	case 1:
		mState = 4;
		break;
	case 3:
		mState = 5;
		break;
	}

	mWarpCollision->remove();
	mMoveCollision->setUp();
	return 1;
}

void TMareEventBumpyWall::bumpDownZ()
{
	f32 current = TMapObjBase::getJointTransZ(mJoint);
	JGeometry::TVec3<f32> trans(0.0f, 0.0f, current);
	f32 limit = mBumpLimit;
	limit = -limit;
	if (current > limit) {
		if (!TMapObjBase::isDemo()) {
			current -= mBumpSpeed;
			SMSRumbleMgr->start(0x13, -1, (f32*)nullptr);
			gpCameraShake->keepShake(CAM_SHAKE_MODE_UNK5, 0.5f);
			if (gpMSound->gateCheck(0x3008)) {
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x3008, &mPosition, 0, nullptr, 0, 4);
			}
		} else {
			SMSRumbleMgr->stop();
		}

		TMapObjBase::setJointTransZ(mJoint, current);
		mMoveCollision->moveTrans(trans);
	} else {
		trans.z = limit;
		TMapObjBase::setJointTransZ(mJoint, limit);
		mMoveCollision->remove();
		mWarpCollision->setUpTrans(trans);
		SMSRumbleMgr->stop(0x13);
		kill();
	}
}

void TMareEventBumpyWall::bumpUpZ()
{
	f32 current = TMapObjBase::getJointTransZ(mJoint);
	JGeometry::TVec3<f32> trans(0.0f, 0.0f, current);
	f32 limit = mBumpLimit;
	if (current < limit) {
		if (!TMapObjBase::isDemo()) {
			current += mBumpSpeed;
			SMSRumbleMgr->start(0x13, -1, (f32*)nullptr);
			gpCameraShake->keepShake(CAM_SHAKE_MODE_UNK5, 0.5f);
			if (gpMSound->gateCheck(0x3008)) {
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x3008, &mPosition, 0, nullptr, 0, 4);
			}
		} else {
			SMSRumbleMgr->stop(0x13);
		}

		TMapObjBase::setJointTransZ(mJoint, current);
		mMoveCollision->moveTrans(trans);
	} else {
		trans.z = limit;
		TMapObjBase::setJointTransZ(mJoint, limit);
		mMoveCollision->remove();
		mWarpCollision->setUpTrans(trans);
		SMSRumbleMgr->stop(0x13);
		kill();
	}
}

void TMareEventBumpyWall::bumpDownX()
{
	f32 current = TMapObjBase::getJointTransX(mJoint);
	JGeometry::TVec3<f32> trans(current, 0.0f, 0.0f);
	f32 limit = mBumpLimit;
	limit = -limit;
	if (current > limit) {
		if (!TMapObjBase::isDemo()) {
			current -= mBumpSpeed;
			SMSRumbleMgr->start(0x13, -1, (f32*)nullptr);
			gpCameraShake->keepShake(CAM_SHAKE_MODE_UNK5, 0.5f);
			if (gpMSound->gateCheck(0x3008)) {
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x3008, &mPosition, 0, nullptr, 0, 4);
			}
		} else {
			SMSRumbleMgr->stop(0x13);
		}

		TMapObjBase::setJointTransX(mJoint, current);
		mMoveCollision->moveTrans(trans);
	} else {
		trans.x = limit;
		TMapObjBase::setJointTransX(mJoint, limit);
		mMoveCollision->remove();
		mWarpCollision->setUpTrans(trans);
		SMSRumbleMgr->stop(0x13);
		kill();
	}
}

void TMareEventBumpyWall::bumpUpX()
{
	f32 current = TMapObjBase::getJointTransX(mJoint);
	JGeometry::TVec3<f32> trans(current, 0.0f, 0.0f);
	f32 limit = mBumpLimit;
	if (current < limit) {
		if (!TMapObjBase::isDemo()) {
			current += mBumpSpeed;
			SMSRumbleMgr->start(0x13, -1, (f32*)nullptr);
			gpCameraShake->keepShake(CAM_SHAKE_MODE_UNK5, 0.5f);
			if (gpMSound->gateCheck(0x3008)) {
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x3008, &mPosition, 0, nullptr, 0, 4);
			}
		} else {
			SMSRumbleMgr->stop(0x13);
		}

		TMapObjBase::setJointTransX(mJoint, current);
		mMoveCollision->moveTrans(trans);
	} else {
		trans.x = limit;
		TMapObjBase::setJointTransX(mJoint, limit);
		mMoveCollision->remove();
		mWarpCollision->setUpTrans(trans);
		SMSRumbleMgr->stop(0x13);
		kill();
	}
}

void TMareEventBumpyWall::control()
{
	TMapObjBase::control();

	switch (mState) {
	case 1:
		break;
	case 2:
		bumpUpX();
		break;
	case 3:
		bumpUpZ();
		break;
	case 4:
		bumpDownX();
		break;
	case 5:
		bumpDownZ();
		break;
	}

	if (!isState(1))
		calcMap();
}

void TMareEventBumpyWall::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);
	stream.read(&mBuildingIndex, 4);

	mJoint         = TMapObjBase::getBuildingJoint(mBuildingIndex);
	mWarpCollision = TMapObjBase::newAndInitBuildingCollisionWarp(
	    mBuildingIndex, nullptr);
	mMoveCollision = TMapObjBase::newAndInitBuildingCollisionMove(
	    mBuildingIndex, nullptr);
	mWarpCollision->setUp();

	mBumpSpeed = 2.0f;
	mBumpLimit = 400.0f;

	switch (mBuildingIndex) {
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x38:
	case 0x39:
	case 0x3A:
	case 0x3F:
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
		mBumpDirection = 0;
		break;
	case 0x22:
	case 0x23:
	case 0x24:
	case 0x25:
	case 0x26:
	case 0x2A:
		mBumpDirection = 1;
		break;
	case 0x27:
	case 0x28:
	case 0x29:
		mBumpDirection = 2;
		break;
	case 0x3B:
	case 0x3C:
	case 0x3D:
	case 0x3E:
		mBumpDirection = 3;
		break;
	}
}

TMareEventBumpyWall::TMareEventBumpyWall(const char* name)
    : TMapObjBase(name)
    , mBuildingIndex(0)
    , mJoint(nullptr)
    , mBumpSpeed(0.0f)
    , mBumpLimit(0.0f)
    , mWarpCollision(nullptr)
    , mMoveCollision(nullptr)
    , mBumpDirection(0)
{
}
