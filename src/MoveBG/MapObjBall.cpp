#include <MoveBG/MapObjBall.hpp>
#include <MoveBG/MapObjBase.hpp>
#include <Strategic/HitActor.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <Player/ModelWaterManager.hpp>
#include <System/Particles.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <stdio.h>
#include <Map/MapData.hpp>
#include <Player/MarioAccess.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JDrama/JDRActor.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
// Rogue includes for static init (JALList<*> instantiations)
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSSetSound.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

u32 TResetFruit::mFruitLivingTime;
u32 TResetFruit::mScaleUpSpeed;
u32 TResetFruit::mRottingScaleSpeed;
u32 TResetFruit::mBreakingScaleSpeed;
u32 TResetFruit::mFruitWaitTimeToAppear;
u32 TResetFruit::mRottenColor;

TMapObjBall::TMapObjBall(const char* name)
    : TMapObjGeneral(name)
{
	unk148 = 0.0f;
	unk14C = 0.0f;
	unk150 = 0.0f;
	unk154 = 0.0f;
	unk158 = 0.0f;
	unk15C = 0.0f;
	unk160 = 0.0f;
	unk164 = 0.0f;
	unk168 = 0.0f;
	unk16C = 0.0f;
	unk170 = 0.0f;
	unk174 = 0.0f;
	unk178 = 0.0f;
	unk17C = 0.0f;
	unk180 = 0.0f;
	unk184 = 0.0f;
	unk188 = 0.0f;
	unk18C = 0.0f;
	unk190 = 0.0f;
	unk194 = 0;
	mInitialScaling.z = 0.0f;
	mInitialScaling.y = 0.0f;
	mInitialScaling.x = 0.0f;
}

TResetFruit::TResetFruit(const char* name)
    : TMapObjBall(name)
{
	unk198 = 0.0f;
	unk1A4 = 0;
	unk19C = 0xFF;
	unk19E = 0xFF;
	unk1A0 = 0xFF;
	unk1A2 = 0xFF;
}

TRandomFruit::TRandomFruit(const char* name)
    : TResetFruit(name)
{
}

void TRandomFruit::initMapObj()
{
	int r = (int)(MsRandF() * 5.0f);
	switch (r) {
	case 0: snprintf(mFruitName, 32, "FruitCoconut"); break;
	case 1: snprintf(mFruitName, 32, "FruitDurian"); break;
	case 2: snprintf(mFruitName, 32, "FruitPapaya"); break;
	case 3: snprintf(mFruitName, 32, "FruitPine"); break;
	default: snprintf(mFruitName, 32, "FruitPine"); break;
	}
	unkF4 = mFruitName;
	TMapObjBall::initMapObj();
	SMS_InitPacket_OneTevColor(getModel(), 0, GX_TEVREG0,
	                           (GXColorS10*)&unk19C);
}

void TCoverFruit::loadAfter()
{
	TMapObjBase::loadAfter();
}

void TResetFruit::touchActor(THitActor* actor)
{
	if (isState(2))
		return;
	if (isState(3))
		return;
	if (isState(0xC))
		return;
	if (isState(0xA))
		return;
	TMapObjBall::touchActor(actor);
	if (mLiveFlag & 0x04000000)
		return;
	if (!isState(1))
		return;
	if (mLiveFlag & 0x10)
		return;
	if (!isUnk104Positive()) {
		unkF8 |= 0x40000;
		unk104 = getLivingTime();
	}
	mLiveFlag &= ~0x10;
	mState = 0xB;
}

void TResetFruit::hold(TTakeActor* taker)
{
	JGeometry::TVec3<f32> v = mVelocity;
	if (v.length() > 10.0f) {
		TMapObjGeneral::hold(taker);
		mVelocity.x = 0.0f;
		mVelocity.y = 0.0f;
		mVelocity.z = 0.0f;
	}
	mVelocity.x = 0.0f;
	mVelocity.y = 0.0f;
	mVelocity.z = 0.0f;
	mLiveFlag |= 0x10;
	if (mLiveFlag & 0x04000000)
		return;
	if (!isUnk104Positive()) {
		unkF8 |= 0x40000;
		unk104 = getLivingTime();
	}
}

void TResetFruit::makeObjLiving()
{
	u8 hasTimer;
	if (unk104 > 0)
		hasTimer = 1;
	else
		hasTimer = 0;
	if (!hasTimer) {
		unkF8 |= 0x40000;
		unk104 = getLivingTime();
	}
	mLiveFlag &= ~0x10;
	mState = 0xB;
}

void TResetFruit::thrown()
{
	TMapObjGeneral::thrown();
	mState = 0xB;
}

void TResetFruit::initMapObj()
{
	TMapObjBall::initMapObj();
	SMS_InitPacket_OneTevColor(getModel(), 0, GX_TEVREG0,
	                           (GXColorS10*)&unk19C);
}

void TMapObjBall::makeObjDefault()
{
	TMapObjBase::makeObjDefault();
	Mtx* m = getModel()->mNodeMatrices;
	(*m)[0][3] = mPosition.x;
	(*m)[1][3] = mPosition.y + mBodyRadius;
	(*m)[2][3] = mPosition.z;
}

void TMapObjBall::put()
{
	TMapObjGeneral::put();
	calcCurrentMtx();
}

void TMapObjBall::hold(TTakeActor* taker)
{
	JGeometry::TVec3<f32> v = mVelocity;
	if (v.length() > 10.0f) {
		TMapObjGeneral::hold(taker);
		mVelocity.x = 0.0f;
		mVelocity.y = 0.0f;
		mVelocity.z = 0.0f;
	}
}

void TMapObjBall::makeObjAppeared()
{
	TMapObjBase::makeObjAppeared();
	calcCurrentMtx();
	Mtx* m       = getModel()->mNodeMatrices;
	(*m)[0][3]   = mPosition.x;
	(*m)[1][3]   = mPosition.y + mBodyRadius;
	(*m)[2][3]   = mPosition.z;
	if (isActorType(0x40000394)) {
		if ((*m)[1][1] > 0.0f) {
			(*m)[1][3] = (*m)[1][3] - 50.0f * (*m)[1][1];
		}
	}
	if (isActorType(0x40000392)) {
		(*m)[1][3] = (*m)[1][3] - 10.0f * (1.0f - (*m)[1][1]);
	}
	unkE8 = 0;
}

BOOL TMapObjBall::receiveMessage(THitActor* sender, u32 message)
{
	if (TMapObjGeneral::receiveMessage(sender, message))
		return 1;
	if (message == 4 && (unkF8 & 0x100000)) {
		hold((TTakeActor*)sender);
		return 1;
	}
	if (sender->isActorType(0x80000001) && !isActorType(0x400000D0)
	    && message != 4) {
		kicked();
		return 1;
	}
	return 0;
}

void TMapObjBall::touchActor(THitActor* actor)
{
	if (unk194 != 0)
		return;
	{
		u8 t;
		if (mState == 6)
			t = 1;
		else
			t = 0;
		if (t)
			return;
	}
	if (isHideObj(actor))
		return;
	if (actor->isActorType(0x08000083))
		return;
	if (actor->isActorType(0x400000CA))
		return;
	if (actor->isActorType(0x400000CC))
		return;

	if (actor->isActorType(0x80000001) && isActorType(0x400000D0)
	    && *gpMarioSpeedY != 0.0f) {
		kicked();
	} else {
		boundByActor(actor);
	}
}

void TMapObjBall::touchRoof(JGeometry::TVec3<f32>* pos)
{
	if (pos->y > unk140) {
		pos->y = unk140;
	}
	calcReflectingVelocity(unk13C, mMapObjData->mPhysical->unk4->unk4,
	                       &mVelocity);
}

void TMapObjBall::rebound(JGeometry::TVec3<f32>* pos)
{
	calcReflectingVelocity(mGroundPlane,
	                       mMapObjData->mPhysical->unk4->unk4, &mVelocity);
	pos->y                                = mGroundHeight;
	mLiveFlag |= 0x80;
	if (isActorType(0x400000D0)) {
		if (mScaling.y >= 5.0f) {
			f32 vol = fabsf(mGroundPlane->mNormal.y);
			if (gpMSound->gateCheck(0x3889)) {
				MSoundSESystem::MSoundSE::startSoundActorWithInfo(
				    0x3889, (Vec*)&mPosition, nullptr, vol, 0, 0, nullptr, 0, 4);
			}
		} else {
			f32 vol = fabsf(mGroundPlane->mNormal.y);
			if (gpMSound->gateCheck(0x388C)) {
				MSoundSESystem::MSoundSE::startSoundActorWithInfo(
				    0x388C, (Vec*)&mPosition, nullptr, vol, 0, 0, nullptr, 0, 4);
			}
		}
	} else {
		u32 soundId = mMapObjData->mSound->unk4->unk0[4];
		if (gpMSound->gateCheck(soundId)) {
			MSoundSESystem::MSoundSE::startSoundActorWithInfo(
			    soundId, (Vec*)&mPosition, (Vec*)&mVelocity, 0.0f, 0, 0,
			    nullptr, 0, 4);
		}
	}
}

void TMapObjBall::touchPollution()
{
	kill();
}

void TMapObjBall::touchWaterSurface()
{
	kill();
}

TBigWatermelon::TBigWatermelon(const char* name)
    : TMapObjBall(name)
{
	unk198 = 0;
	unk19C = 0;
	unk1A0 = 0.0f;
}

u32 TResetFruit::getLivingTime() const
{
	return mFruitLivingTime;
}

void TResetFruit::killByTimer(int timer)
{
	unk104 = timer;
	unkF8 |= 0x40000;
	mState = 0xB;
}

void TBigWatermelon::checkWallCollision(JGeometry::TVec3<f32>* pos)
{
	TMapObjGeneral::checkWallCollision(pos);
}

void TBigWatermelon::touchGround(JGeometry::TVec3<f32>* pos)
{
	TMapObjBall::touchGround(pos);
}

void TBigWatermelon::touchWall(JGeometry::TVec3<f32>* pos,
                               TBGWallCheckRecord* record)
{
	TMapObjGeneral::touchWall(pos, record);
}

void TBigWatermelon::loadAfter()
{
	TMapObjGeneral::loadAfter();
	JDrama::TActor* shine
	    = JDrama::TNameRefGen::search<JDrama::TActor>("シャイン（お化けスイカ用）");
	shine->mPosition.x = -4659.0f;
	shine->mPosition.y = 460.0f;
	shine->mPosition.z = 13620.0f;
}

void TBigWatermelon::initMapObj()
{
	TMapObjBall::initMapObj();
	if (!gParticleFlagLoaded[0x5d]) {
		gpResourceManager->load("/scene/mapObj/watermelon_bomb.jpa", 0x5d);
		gParticleFlagLoaded[0x5d] = 1;
	}
	if (!gParticleFlagLoaded[0x5e]) {
		gpResourceManager->load("/scene/mapObj/watermelon_bomb_a.jpa", 0x5e);
		gParticleFlagLoaded[0x5e] = 1;
	}
	if (!gParticleFlagLoaded[0x5f]) {
		gpResourceManager->load("/scene/mapObj/watermelon_bomb_b.jpa", 0x5f);
		gParticleFlagLoaded[0x5f] = 1;
	}
	if (!gParticleFlagLoaded[0x6b]) {
		gpResourceManager->load("/scene/mapObj/watermelon_shrink_a.jpa", 0x6b);
		gParticleFlagLoaded[0x6b] = 1;
	}
	if (!gParticleFlagLoaded[0x6c]) {
		gpResourceManager->load("/scene/mapObj/watermelon_shrink_b.jpa", 0x6c);
		gParticleFlagLoaded[0x6c] = 1;
	}
	unk198 = (u32) new TWaterEmitInfo("/watermelon.prm");
}

BOOL TBigWatermelon::receiveMessage(THitActor* sender, u32 message)
{
	if (sender->isActorType(0x80000001)) {
		boundByActor(sender);
		return 1;
	}
	if (TMapObjGeneral::receiveMessage(sender, message))
		return 1;
	if (message == 4 && (unkF8 & 0x100000)) {
		hold((TTakeActor*)sender);
		return 1;
	}
	if (sender->isActorType(0x80000001) && !isActorType(0x400000D0)
	    && message != 4) {
		kicked();
		return 1;
	}
	return 0;
}

void TBigWatermelon::touchWaterSurface()
{
	emitColumnWater();
	if (gpMSound->gateCheck(0x3875)) {
		MSoundSESystem::MSoundSE::startSoundActor(0x3875, (Vec*)&mPosition, 0,
		                                          nullptr, 0, 4);
	}
	kill();
}
