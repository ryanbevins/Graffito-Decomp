#include <MoveBG/MapObjTree.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <Map/MapCollisionManager.hpp>
#include <Map/MapData.hpp>
#include <Map/MapEventSink.hpp>
#include <Map/PollutionManager.hpp>
#include <Strategic/HitActor.hpp>
#include <Player/MarioAccess.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>
#include <System/EmitterViewObj.hpp>
#include <Camera/CameraShake.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JGeometry/JGMatrix34.hpp>
#include <dolphin/mtx.h>
#include <stdlib.h>
#include <stdio.h>
#include <MSound/MSoundBGM.hpp>

static int sWaitTime = 1;

f32 TMapObjTree::mBananaTreeJumpPower = 1000.0f;

f32 TMapObjTreeScale::mScaleSpeedY        = 0.005f;
f32 TMapObjTreeScale::mStatusChangeScaleY = 0.3f;
f32 TMapObjTreeScale::mScaleSpeedXZ       = 0.007f;
f32 TMapObjTreeScale::mScaleMin           = 0.1f;

//
// TMapObjLeaf
//

TMapObjLeaf::TMapObjLeaf()
    : mAngle(0.0f)
    , mAngleVel(0.0f)
    , mCollision(nullptr)
{
	mMtx.identity();
	mCollision = new TMapCollisionMove();
}

//
// TMapObjTree
//

TMapObjTree::TMapObjTree(const char* name)
    : TMapObjGeneral(name)
{
	unk148               = 0.0f;
	unk14C               = 0.0f;
	mLeafCount           = 0;
	mLeaves              = nullptr;
	mIsResting           = true;
	mMarioIsOnRotImpulse = 0.0f;
	mHipAttackRotImpulse = 0.0f;
	mSpring              = 0.0f;
	mDamping             = 0.0f;
	unk16C               = 0;
}

f32 TMapObjTree::getRadiusAtY(f32 y) const
{
	return unk148
	     + (unk14C - unk148) * (mPosition.y + mDamageHeight - y)
	           / mDamageHeight;
}

void TMapObjTree::initMapObj()
{
	TMapObjGeneral::initMapObj();
	initEach();
	mLeaves = new TMapObjLeaf[mLeafCount];
	char buf[0x40];
	for (int i = 0; i < mLeafCount; i++) {
		TMapObjLeaf& leaf = mLeaves[i];
		leaf.mCollision = new TMapCollisionMove();
		bool isPalm = (mActorType == 0x40000038) ? true : false;
		if (isPalm) {
			snprintf(buf, 0x100, "/mapObj/palmLeaf%02d", i + 1);
		} else {
			snprintf(buf, 0x100, "/mapObj/%sLeaf%02d", unkF4, i + 1);
		}
		leaf.mCollision->init(buf, 0, this);
		leaf.mCollision->setAllData(i);
		leaf.mCollision->remove();
		int leafIdx = mLeafCount - i;
		leaf.mMtx.set(getModel()->getAnmMtx(leafIdx));
		TMapCollisionMove* collision = leaf.mCollision;
		PSMTXCopy(leaf.mMtx, collision->unk20);
		collision->setUp();
	}
	if (mMapCollisionManager != nullptr)
		mMapCollisionManager->unk10 = nullptr;
}

void TMapObjTree::initEach()
{
	switch (mActorType) {
	case 0x40000034:
		unk148               = 20.0f;
		unk14C               = 95.0f;
		mLeafCount           = 12;
		mMarioIsOnRotImpulse = 0.001f;
		mHipAttackRotImpulse = 0.006f;
		mSpring              = 0.01f;
		mDamping             = 0.97f;
		break;
	case 0x40000035:
		unk148               = 20.0f;
		unk14C               = 100.0f;
		mLeafCount           = 8;
		mMarioIsOnRotImpulse = 0.001f;
		mHipAttackRotImpulse = 0.006f;
		mSpring              = 0.01f;
		mDamping             = 0.97f;
		break;
	case 0x40000036:
		unk148               = 50.0f;
		unk14C               = 100.0f;
		mLeafCount           = 12;
		mMarioIsOnRotImpulse = 0.001f;
		mHipAttackRotImpulse = 0.006f;
		mSpring              = 0.01f;
		mDamping             = 0.97f;
		break;
	case 0x40000037:
		unk148               = 95.0f;
		unk14C               = 60.0f;
		mLeafCount           = 8;
		mMarioIsOnRotImpulse = 0.001f;
		mHipAttackRotImpulse = 0.006f;
		mSpring              = 0.01f;
		mDamping             = 0.97f;
		break;
	case 0x40000039:
		unk148               = 70.0f;
		unk14C               = 100.0f;
		mLeafCount           = 8;
		mMarioIsOnRotImpulse = 0.004f;
		mHipAttackRotImpulse = 0.008f;
		mSpring              = 0.03f;
		mDamping             = 0.9f;
		break;
	}
}

void TMapObjTree::perform(u32 msg, JDrama::TGraphics* graphics)
{
	if (!mIsResting && (msg & 1)) {
		int restingCount = 0;
		for (int i = 0; i < mLeafCount; i++)
			restingCount += controlLeaf(i);
		if (mColCount == 0 && restingCount == mLeafCount)
			mIsResting = true;
	}
	TMapObjGeneral::perform(msg, graphics);
}

int TMapObjTree::controlLeaf(int i)
{
	TMapObjLeaf& leaf = mLeaves[i];
	if (leaf.mAngleVel == 0.0f) {
		if (*gpMarioSpeedY <= 0.0f) {
			TMtx34f mtx;
			mtx.set(leaf.mMtx);
			leaf.mCollision->moveMtx(mtx);
		}
		return 1;
	}

	leaf.mAngle += leaf.mAngleVel;
	leaf.mAngleVel -= leaf.mAngle * mSpring;
	leaf.mAngleVel *= mDamping;

	TMtx34f rotation;
	rotation.identity();
	TMtx34f mtx;
	JGeometry::TVec3<f32> axis(1.0f, 0.0f, 0.0f);
	PSMTXRotAxisRad(rotation, axis, leaf.mAngle);

	mtx.set(leaf.mMtx);
	MTXConcat(mtx, rotation, mtx);
	getModel()->setAnmMtx(mLeafCount - i, mtx);

	if (*gpMarioSpeedY <= 0.0f)
		leaf.mCollision->moveMtx(mtx);

	if (fabsf(leaf.mAngle) < mMarioIsOnRotImpulse
	    && fabsf(leaf.mAngle) < mMarioIsOnRotImpulse)
		return 1;
	return 0;
}

void TMapObjTree::touchPlayer(THitActor* sender)
{
	mIsResting = false;
	const TBGCheckData* gp = *gpMarioGroundPlane;
	s16 leafIdx            = gp->mData;
	if (gp->mActor != this || leafIdx < 0 || leafIdx >= mLeafCount)
		return;

	if (marioHipAttack()) {
		mLeaves[leafIdx].mAngleVel += mHipAttackRotImpulse;
	} else if (marioIsOn()) {
		mLeaves[leafIdx].mAngleVel += mMarioIsOnRotImpulse;
	}
}

//
// TMapObjTreeScale
//

TMapObjTreeScale::TMapObjTreeScale(const char* name)
    : TMapObjTree(name)
    , mParticleIndex(0)
    , mWaitTimer(0)
    , mEventSink(nullptr)
{
	for (int i = 0; i < 30; i++) {
		mParticlePos[i].x = 0.0f;
		mParticlePos[i].y = 0.0f;
		mParticlePos[i].z = 0.0f;
	}
}

void TMapObjTreeScale::loadAfter()
{
	TMapObjGeneral::loadAfter();
	if (gpMarDirector->mMap == 4
	    || gpPollution->isPolluted(mPosition.x, mPosition.y, mPosition.z)) {
		mScaling.z = mScaling.y = mScaling.x = mScaleMin;
		sleep();
		unk64 &= ~1;
		unk64 |= 2;
		setObjHitData(0);
		mDamageRadius = mAttackRadius;
		calcEntryRadius();
		mDamageHeight = 30.0f;
		calcEntryRadius();
		removeMapCollision();
		unkF8 &= ~0x100;
		mActorType = 0x4000003b;
		mState     = 11;
		SMS_HideAllShapePacket(getModel());
	}
	mEventSink = (TMapEventSink*)JDrama::TNameRefGen::instance->mRootNameRef
	                 ->search("イベント（地形沈むビアンコ）");
}

void TMapObjTreeScale::control()
{
	switch (mState) {
	case 11:
		if (gpMarDirector->mMap == 4)
			break;
		if (gpPollution->isPolluted(mPosition.x, mPosition.y, mPosition.z))
			break;
		awake();
		mActorType = 0x40000039;
		removeMapCollision();
		mState = 12;
		break;
	case 12:
		if (gpMSound->gateCheck(0x300f))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x300f, (Vec*)&mPosition, 0, 0, 0, 4);
		mScaling.y += mScaleSpeedY;
		if (mScaling.y > mStatusChangeScaleY)
			mState = 13;
		break;
	case 13:
		if (gpMSound->gateCheck(0x300f))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x300f, (Vec*)&mPosition, 0, 0, 0, 4);
		if (mScaling.y < 1.0f)
			mScaling.y += mScaleSpeedY;
		else
			mScaling.y = 1.0f;
		if (mScaling.x < 1.0f) {
			mScaling.x += mScaleSpeedXZ;
			mScaling.z += mScaleSpeedXZ;
		} else {
			mScaling.x = 1.0f;
			mScaling.z = 1.0f;
			unkF8 |= 0x100;
			getModel()->calc();
			unk64 &= ~2;
			setUpCurrentMapCollision();
			mState = 1;
		}
		break;
	default:
		TMapObjGeneral::control();
		break;
	}

	bool isStateActive = (mState == 12) ? true : false;
	if (!isStateActive) {
		bool isStateActive13 = (mState == 13) ? true : false;
		if (!isStateActive13)
			return;
	}

	setObjHitData(0);

	bool inMapEvent = false;
	if (gpMarDirector->mMap == 2) {
		u8 game = gpMarDirector->unk124;
		if (game == 3 || game == 4)
			inMapEvent = true;
	}

	if (!inMapEvent) {
		if (mEventSink == NULL || mEventSink->isBuried(1)) {
			SMSRumbleMgr->start(0x13, (Vec*)&mPosition);
			gpCameraShake->keepShake(CAM_SHAKE_MODE_UNK5, 1.0f);
		}
	}

	if (mWaitTimer > sWaitTime) {
		mParticlePos[mParticleIndex].set(
		    (rand() * (1.0f / 32768.0f) * 400.0f + mPosition.x) - 200.0f,
		    mPosition.y,
		    (rand() * (1.0f / 32768.0f) * 400.0f + mPosition.z) - 200.0f);
		gpMarioParticleManager->emit(
		    0x1db, &mParticlePos[mParticleIndex], 2, this);
		mParticleIndex++;
		if (mParticleIndex >= 30)
			mParticleIndex = 0;
		mWaitTimer = 0;
	}
	mWaitTimer++;
}

u32 TMapObjTreeScale::touchWater(THitActor* sender)
{
	if (0.0f == mScaling.x)
		return TMapObjGeneral::touchWater(sender);
	bool isState11 = (mState == 11) ? true : false;
	if (isState11) {
		awake();
		mActorType = 0x40000039;
		removeMapCollision();
		mState = 12;
	}
	return 1;
}
