#include <MoveBG/MapObjFence.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <MoveBG/MapObjMessenger.hpp>
#include <Map/MapCollisionManager.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <Enemy/Graph.hpp>
#include <Enemy/Conductor.hpp>
#include <Player/MarioAccess.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <M3DUtil/MActor.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <stdlib.h>
#include <string.h>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

f32 TRailFence::mFallHeight = 50000.0f;
int TRailFence::mWaitTime   = 240;

f32 TFenceWater::mWaterAccel     = 2.1f;
f32 TFenceWater::mBackSpeed      = 3.0f;
int TFenceWater::mTurnedWaitTime = 600;

f32 TRevolvingFenceInner::mSpeed = 4.0f;

static inline void MsMtxSetRotY(MtxPtr m, f32 deg)
{
	s16 a = (s16)(deg * 182.04445f);
	f32 s = JMASSin(a);
	f32 c = JMASCos(a);
	m[0][0] = c;
	m[0][1] = 0.0f;
	m[0][2] = s;
	m[0][3] = 0.0f;
	m[1][0] = 0.0f;
	m[1][1] = 1.0f;
	m[1][2] = 0.0f;
	m[1][3] = 0.0f;
	m[2][0] = -s;
	m[2][1] = 0.0f;
	m[2][2] = c;
	m[2][3] = 0.0f;
}

TRailFence::TRailFence(const char* name)
    : TFence(name)
    , unk13C(new TGraphTracer())
    , unk140(0.0f)
{
}

void TRailFence::load(JSUMemoryInputStream& stream)
{
	TMapObjBase::load(stream);

	char graphName[64];
	stream.readString(graphName, 64);
	TGraphWeb* web = gpConductor->getGraphByName(graphName);
	if (web && !web->isDummy()) {
		unk13C->unk0 = web;
		unk13C->setTo(web->findNearestNodeIndex(mPosition, 0xffffffff));
	}

	unk140   = 8.0f;
	mGravity = 0.3f;
}

void TRailFence::initMapCollisionData() { TMapObjBase::initMapCollisionData(); }

void TRailFence::control()
{
	TMapObjBase::control();
	switch (mState) {
	case 2:
		goOnRail();
		break;
	case 3:
		if (!isLifeTimerActive()) {
			removeMapCollision();
			if (gpMSound->gateCheck(0x3821))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x3821, (const Vec*)&mPosition, 0, nullptr, 0, 4);
			mState = 4;
		}
		break;
	case 4: {
		JGeometry::TVec3<f32> vel = mVelocity;
		mPosition.y += vel.y;
		mVelocity.y -= mGravity;
		if (mVelocity.y < -100.0f)
			mVelocity.y = -100.0f;
		if (mPosition.y < mInitialPosition.y - mFallHeight) {
			mPosition = mInitialPosition;
			setUpMapCollision(0);
			unk13C->setTo(
			    unk13C->unk0->findNearestNodeIndex(mPosition, 0xffffffff));
			appear();
			init(nullptr);
			getModel()->calc();
			onMapObjFlag(0x100);
		}
		break;
	}
	}
}

void TRailFence::goOnRail()
{
	TGraphTracer* tracer = unk13C;
	if (!tracer->unk0 || tracer->unk0->isDummy())
		return;

	JGeometry::TVec3<f32> nodePos = tracer->unk0->indexToPoint(tracer->mCurrIdx);
	JGeometry::TVec3<f32> diff;
	diff.sub(nodePos, mPosition);
	f32 dist = diff.length();
	if (dist < unk140) {
		tracer->unk0->getGraphNode(tracer->mCurrIdx).getPoint(mPosition);
		int next = tracer->unk0->getShortestNextIndex(tracer->mCurrIdx,
		                                              tracer->mPrevIdx,
		                                              0xffffffff);
		if (next < 0) {
			mState     = 3;
			mLifeTimer = mWaitTime;
			return;
		}
		tracer->moveTo(next);
	} else {
		f32 invLen = 1.0f / dist;
		mPosition.x += diff.x * invLen * unk140;
		mPosition.y += diff.y * invLen * unk140;
		mPosition.z += diff.z * invLen * unk140;
	}
}

BOOL TRailFence::receiveMessage(THitActor* sender, u32 message)
{
	if (message == 3) {
		if (gpMSound->gateCheck(0x3864))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x3864, (const Vec*)&mPosition, 0, nullptr, 0, 4);
		setUpMapCollision(1);
		offMapObjFlag(0x100);
		mState = 2;
		return TRUE;
	}
	return FALSE;
}

void TFenceWaterH::changeStatusToWait()
{
	unk140 = 0.0f;
	unk13C = 0.0f;
	mState = 1;
	setUpMapCollision(0);
}

void TFenceWaterH::changeStatusToGo()
{
	if (gpMSound->gateCheck(0x3838))
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x3838, (const Vec*)&mPosition, 0, nullptr, 0, 4);
	mState = 2;
	setUpMapCollision(1);
}

void TFenceWaterH::control()
{
	TMapObjBase::control();
	controlRotation();

	f32 phase = unk140 + mInitialRotation.z;
	while (phase >= 360.0f)
		phase -= 360.0f;
	while (phase < 0.0f)
		phase += 360.0f;
	mRotation.y = phase;

	s16 a              = (s16)(mRotation.y * 182.04445f);
	unk144->mPosition.x = 500.0f * JMASCos(a) + mPosition.x;
	unk144->mPosition.z = -500.0f * JMASSin(a) + mPosition.z;
}

void TFenceWater::initMapObj()
{
	if (strstr(unkF4, "bamboo"))
		unk138 = 1;
	TMapObjBase::initMapObj();

	unk144 = new TMapObjMessenger(
	    "\x92\x6E\x8C\x60\x83\x49\x83\x75\x83\x57\x83\x46\x83\x81\x83\x62\x83\x5A\x83\x93\x83\x57\x83\x83\x81\x5B");
	unk144->unk68 = (u32)this;
	unk144->initHitActor(mActorType, 1, 0, 0.0f, 0.0f, 100.0f, 300.0f);
	unk144->unk64 &= ~1;

	unk144->mPosition.x = mPosition.x;
	unk144->mPosition.y = mPosition.y - 150.0f;
	unk144->mPosition.z = mPosition.z;

	// register child messenger with name ref gen (list insertion omitted)
}

void TFenceWater::initMapCollisionData() { TMapObjBase::initMapCollisionData(); }

void TFenceWater::control()
{
	TMapObjBase::control();
	controlRotation();

	f32 phase = unk140 + mInitialRotation.y;
	while (phase >= 360.0f)
		phase -= 360.0f;
	while (phase < 0.0f)
		phase += 360.0f;
	mRotation.y = phase;

	s16 a               = (s16)(mRotation.y * 182.04445f);
	unk144->mPosition.x = 500.0f * JMASCos(a) + mPosition.x;
	unk144->mPosition.z = -500.0f * JMASSin(a) + mPosition.z;
}

void TFenceWater::controlRotation()
{
	switch (mState) {
	case 2:
		unk140 -= unk13C;
		if (unk140 <= -90.0f) {
			unk140     = -90.0f;
			unk13C     = 0.0f;
			mState     = 3;
			mLifeTimer = mTurnedWaitTime;
		}
		break;
	case 3:
		if (!isLifeTimerActive()) {
			if (gpMSound->gateCheck(0x3839))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x3839, (const Vec*)&mPosition, 0, nullptr, 0, 4);
			unk13C = mBackSpeed;
			mState = 4;
		}
		break;
	case 4:
		unk140 += unk13C;
		if (unk140 >= 0.0f)
			changeStatusToWait();
		break;
	}
}

void TFenceWater::changeStatusToWait()
{
	unk140 = 0.0f;
	unk13C = 0.0f;
	mState = 1;
}

void TFenceWater::changeStatusToGo()
{
	if (gpMSound->gateCheck(0x3838))
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x3838, (const Vec*)&mPosition, 0, nullptr, 0, 4);
	mState = 2;
}

BOOL TFenceWater::receiveMessage(THitActor* sender, u32 message)
{
	if (mState == 3 && message == 15) {
		unk13C = mWaterAccel;
		if (unk13C > 0.0f)
			changeStatusToGo();
		return TRUE;
	}
	return FALSE;
}

void TFenceWater::draw() const { }

void TRevolvingFenceInner::initMapObj()
{
	if (strstr(unkF4, "bamboo"))
		unk138 = 1;
	TMapObjBase::initMapObj();

	unk140 = (fabsf(mRotation.x) < 1.0f && fabsf(mRotation.z) < 1.0f) ? 1 : 0;

	Mtx mtx;
	MsMtxSetTRS(mtx, mPosition.x, mPosition.y, mPosition.z, mRotation.x,
	            mRotation.y, mRotation.z, mScaling.x, mScaling.y, mScaling.z);
	TMapCollisionBase* base = mMapCollisionManager->getUnk8();
	base->setMtx(mtx);
	base->setUp();
}

void TRevolvingFenceInner::initMapCollisionData()
{
	mMapCollisionManager
	    = new TMapCollisionManager(1, "mapObj", (const TLiveActor*)this);
	if (fabsf(mRotation.x) < 80.0f && fabsf(mRotation.z) < 80.0f)
		mMapCollisionManager->init("fence_revolve_inner_v_tool", 1, nullptr);
	else
		mMapCollisionManager->init("fence_revolve_inner_h_tool", 1, nullptr);
}

void TRevolvingFenceInner::control()
{
	TMapObjBase::control();
	if (unk140)
		controlWall();
	else
		controlGroundRoof();
}

void TRevolvingFenceInner::setGroundCollision()
{
	u8* yoshi = (u8*)SMS_GetYoshi();
	if (yoshi && *yoshi) {
		f32 yx = *(f32*)(yoshi + 0x20);
		f32 yz = *(f32*)(yoshi + 0x28);
		if (mPosition.x - mBodyRadius < yx
		    && mPosition.x + mBodyRadius > yx
		    && mPosition.z - mBodyRadius < yz
		    && mPosition.z + mBodyRadius > yz) {
			Mtx mtx;
			JGeometry::gekko_ps_copy12(mtx, getModel()->mNodeMatrices);
			TMapCollisionBase* coll = mMapCollisionManager->getUnk8();
			if (coll)
				coll->setMtx(mtx);
		}
	}
	TMapObjBase::setGroundCollision();
}

void TRevolvingFenceInner::controlGroundRoof()
{
	switch (mState) {
	case 3:
	case 5:
		if (mMActor->curAnmEndsNext(0, nullptr)) {
			mState = 2;
			mMActor->setFrameRate(0.0f, 0);
			mMActor->getFrameCtrl(0)->setRate(0.0f);
			mMActor->calc();
			onMapObjFlag(0x100);
		}
		break;
	case 4:
	case 6:
		if (mMActor->curAnmEndsNext(0, nullptr)) {
			mState = 1;
			mMActor->setFrameRate(0.0f, 0);
			mMActor->getFrameCtrl(0)->setRate(0.0f);
			mMActor->calc();
			onMapObjFlag(0x100);
		}
		break;
	}
}

void TRevolvingFenceInner::controlWall()
{
	switch (mState) {
	case 3: {
		unk13C += mSpeed;
		if (unk13C > 180.0f) {
			unk13C      = 180.0f;
			mRotation.y = unk13C + mInitialRotation.y;
			mState      = 2;
		}
		mRotation.y = unk13C + mInitialRotation.y;
		f32 rot     = MsWrap(mRotation.y, 0.0f, 360.0f);
		MsMtxSetRotY(getModel()->getBaseTRMtx(), rot);
		MtxPtr m = getModel()->getBaseTRMtx();
		m[0][3]  = mPosition.x;
		m[1][3]  = mPosition.y - mYOffset;
		m[2][3]  = mPosition.z;
		break;
	}
	case 5: {
		unk13C -= mSpeed;
		if (unk13C < -180.0f) {
			unk13C      = -180.0f;
			mRotation.y = unk13C + mInitialRotation.y;
			mState      = 1;
		}
		mRotation.y = unk13C + mInitialRotation.y;
		f32 rot     = MsWrap(mRotation.y, 0.0f, 360.0f);
		MsMtxSetRotY(getModel()->getBaseTRMtx(), rot);
		MtxPtr m = getModel()->getBaseTRMtx();
		m[0][3]  = mPosition.x;
		m[1][3]  = mPosition.y - mYOffset;
		m[2][3]  = mPosition.z;
		break;
	}
	case 4:
	case 6:
		break;
	}
}

BOOL TRevolvingFenceInner::receiveMessage(THitActor* sender, u32 message)
{
	if (message == 3 && unk140 == 0) {
		if (mState == 1) {
			if (gpMSound->gateCheck(0x3824))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x3824, (const Vec*)&mPosition, 0, nullptr, 0, 4);
			mState = 3;
			startBck("fence_revolve_inner_roll_down");
			offMapObjFlag(0x100);
			return TRUE;
		}
		if (mState == 2) {
			if (gpMSound->gateCheck(0x3825))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x3825, (const Vec*)&mPosition, 0, nullptr, 0, 4);
			mState = 4;
			startBck("fence_revolve_inner_roll_up");
			offMapObjFlag(0x100);
			return TRUE;
		}
	}
	if (message == 3 && unk140 != 0) {
		f32 angle = getRotYFromAxisZ(*gpMarioPos);
		f32 deg   = 180.0f * (angle / 3.14f) + mRotation.y;
		deg       = MsWrap(deg, -180.0f, 180.0f);
		bool fwd = (deg > -180.0f && deg < -90.0f)
		           || (deg > 0.0f && deg < 90.0f);
		if (fwd) {
			if (gpMSound->gateCheck(0x3824))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x3824, (const Vec*)&mPosition, 0, nullptr, 0, 4);
			mState = (mState == 1) ? 3 : 4;
		} else {
			if (gpMSound->gateCheck(0x3825))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x3825, (const Vec*)&mPosition, 0, nullptr, 0, 4);
			mState = (mState == 1) ? 5 : 6;
		}
		return TRUE;
	}
	return FALSE;
}

void TRevolvingFenceOuter::initMapCollisionData()
{
	mMapCollisionManager
	    = new TMapCollisionManager(1, "mapObj", (const TLiveActor*)this);
	if (fabsf(mRotation.x) < 1.0f && fabsf(mRotation.z) < 1.0f)
		mMapCollisionManager->init("fence_revolve_outer_v_tool", 0, nullptr);
	else
		mMapCollisionManager->init("fence_revolve_outer_h_tool", 0, nullptr);

	Mtx mtx;
	MsMtxSetTRS(mtx, mPosition.x, mPosition.y, mPosition.z, mRotation.x,
	            mRotation.y, mRotation.z, mScaling.x, mScaling.y, mScaling.z);
	TMapCollisionBase* base = mMapCollisionManager->getUnk8();
	base->setMtx(mtx);
	base->setUp();

	if (unk138) {
		JGeometry::TVec3<f32> ones(1.0f, 1.0f, 1.0f);
		unk13C = TMapObjBaseManager::newAndRegisterObj(
		    "bambooFence_revolve_inner", mPosition, mRotation, ones);
	} else {
		JGeometry::TVec3<f32> ones(1.0f, 1.0f, 1.0f);
		unk13C = TMapObjBaseManager::newAndRegisterObj(
		    "fence_revolve_inner", mPosition, mRotation, ones);
	}
	unk13C->initMapCollisionData();
}

BOOL TRevolvingFenceOuter::receiveMessage(THitActor* sender, u32 message)
{
	if (message == 3) {
		startBck("fence_revolve_outer_shake");
		unk13C->startBck("fence_revolve_inner_shake");
		return TRUE;
	}
	return FALSE;
}

void TFence::initMapObj()
{
	if (strstr(unkF4, "bamboo"))
		unk138 = 1;
	TMapObjBase::initMapObj();
}

void TFence::initMapCollisionData()
{
	mMapCollisionManager
	    = new TMapCollisionManager(1, "mapObj", (const TLiveActor*)this);

	if (strcmp(unkF4, "fence3x3") == 0) {
		if (fabsf(mRotation.x) < 1.0f && fabsf(mRotation.z) < 1.0f)
			mMapCollisionManager->init("fence_normal_v_tool", 0, nullptr);
		else
			mMapCollisionManager->init("fence_h_tool", 0, nullptr);
	} else {
		if (fabsf(mRotation.x) < 1.0f && fabsf(mRotation.z) < 1.0f)
			mMapCollisionManager->init("fence_half_v_tool", 0, nullptr);
		else
			mMapCollisionManager->init("fence_half_h_tool", 0, nullptr);
	}

	Mtx mtx;
	MsMtxSetTRS(mtx, mPosition.x, mPosition.y, mPosition.z, mRotation.x,
	            mRotation.y, mRotation.z, mScaling.x, mScaling.y, mScaling.z);
	TMapCollisionBase* base = mMapCollisionManager->getUnk8();
	base->setMtx(mtx);
	base->setUp();
}

BOOL TFence::receiveMessage(THitActor* sender, u32 message)
{
	if (message == 3) {
		startBck("fence_normal_shake");
		return TRUE;
	}
	return FALSE;
}
