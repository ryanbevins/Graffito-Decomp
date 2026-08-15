#include <Enemy/BossManta.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/Graph.hpp>
#include <GC2D/GCConsole2.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorAnm.hpp>
#include <Map/PollutionManager.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/ReinitGX.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/ModelWaterManager.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Strategy.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>
#include <System/Resolution.hpp>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// rogue includes needed for matching sinit & bss
#include <M3DUtil/InfectiousStrings.hpp>
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

// Infectious dummies for the Vec zero/one literals owned by this TU.
inline static void dummyVecZero(Vec* v)
{
	*v = (Vec) { 0.0f, 0.0f, 0.0f };
}

inline static void dummyVecOne(Vec* v)
{
	*v = (Vec) { 1.0f, 1.0f, 1.0f };
}

f32 TBossManta::sFrameRate[TBossManta::GENERATION_COUNT]
    = { 0.3f, 0.5f, 1.2f, 2.0f, 5.0f, 5.3f };
f32 TBossManta::sScale[TBossManta::GENERATION_COUNT]
    = { 20.0f, 10.0f, 5.0f, 2.0f, 1.0f, 1.0f };

u32 TBossManta::sCenterJointIndex;
u32 TBossManta::sBodyJointIndex;
u32 TBossManta::sRwingJointIndex;
u32 TBossManta::sLwingJointIndex;
bool TBossManta::sEscapeFromMario;

static JAISound* sDefeatSE;

namespace {

inline void AttackMario(THitActor* attacker)
{
	JGeometry::TVec3<f32> dir;
	dir.set(SMS_GetMarioPos());
	dir -= attacker->mPosition;
	dir.normalize();
	dir *= 2.0f;
	dir.y += 1.0f;
	SMS_SendMessageToMario(attacker, HIT_MESSAGE_ATTACK);
	SMS_SendMessageToMario(attacker, HIT_MESSAGE_UNK7);
	SMS_ThrowMario(dir, 60.0f);
}

} // namespace

inline const TNerveMantaAppearDemo& TNerveMantaAppearDemo::theNerve()
{
	static TNerveMantaAppearDemo instance;
	return instance;
}

inline const TNerveMantaDeath& TNerveMantaDeath::theNerve()
{
	static TNerveMantaDeath instance;
	return instance;
}

inline const TNerveMantaSpawn& TNerveMantaSpawn::theNerve()
{
	static TNerveMantaSpawn instance;
	return instance;
}

inline const TNerveMantaMove& TNerveMantaMove::theNerve()
{
	static TNerveMantaMove instance;
	return instance;
}
const TNerveMantaHitWater& TNerveMantaHitWater::theNerve()
{
	static TNerveMantaHitWater instance;
	return instance;
}
#define BLEND_MANTA_TARGET(rate)                                              \
	do {                                                                      \
		f32 ratio = (rate);                                                   \
		f32 delta = gpMarioPos->x - target.x;                                 \
		delta *= ratio;                                                       \
		target.x += delta;                                                    \
		delta = gpMarioPos->y - target.y;                                     \
		delta *= ratio;                                                       \
		target.y += delta;                                                    \
		delta = gpMarioPos->z - target.z;                                     \
		delta *= ratio;                                                       \
		target.z += delta;                                                    \
	} while (0)

BOOL TNerveMantaMove::execute(TSpineBase<TLiveActor>* spine) const
{
	TBossManta* self = (TBossManta*)spine->getBody();
	TGraphWeb* graph = self->unk124->getGraph();
	int time         = spine->getTime();

	if (time == 0) {
		if (self->mMActor->unkC)
			self->mMActor->unkC->initNormalMotionBlend();
		self->mMActor->setBckFromIndex(3);
		J3DAnmTransform* oldAnm = nullptr;
		MActorAnmDataEach<J3DAnmTransformKey>* bckData
		    = self->mMActorKeeper->mActorAnmData->getUnk2C();
		if (bckData->unk0 > 4)
			oldAnm = (J3DAnmTransform*)bckData->unkC[4];
		if (self->mMActor->unkC)
			self->mMActor->unkC->setOldMotionBlendAnmPtr(oldAnm);
		if (self->mMActor->unkC)
			self->mMActor->unkC->setMotionBlendRatio(0.5f);
		self->mMActor->setFrameRate(
		    SMSGetAnmFrameRate() * TBossManta::sFrameRate[self->mGeneration],
		    0);

		int idx = (int)(rand() * 0.000030517578f * graph->unk8);
		JGeometry::TVec3<f32> target = graph->indexToPoint(idx);
		BLEND_MANTA_TARGET(1.0f);
		self->mAttractor = target;
	}

	if (self->mGeneration < 3 && gpMSound->gateCheck(0x8199))
		MSoundSESystem::MSoundSE::startSoundActor(0x8199, &self->mPosition,
		                                          0, nullptr, 0, 4);

	if (self->mGeneration == 5 && time % 96 == self->mInstanceIndex)
		gpMSound->startSoundSet(0x899B, &self->mPosition, 0, 0.0f, 0, 0,
		                        4);

	JGeometry::TVec3<f32> toAttractor = self->mPosition;
	toAttractor.sub(self->mAttractor);
	if (toAttractor.length() < 500.0f || time % 150 == 0) {
		int idx = (int)(rand() * 0.000030517578f * graph->unk8);
		JGeometry::TVec3<f32> target = graph->indexToPoint(idx);

		if (self->unk1A4) {
			switch (self->mGeneration) {
			case 0:
				BLEND_MANTA_TARGET(1.0f);
				break;
			case 1:
				BLEND_MANTA_TARGET(1.0f);
				break;
			case 2:
				BLEND_MANTA_TARGET(1.0f);
				break;
			case 3:
				BLEND_MANTA_TARGET(1.0f);
				break;
			case 4:
				BLEND_MANTA_TARGET(1.0f);
				break;
			case 5:
				BLEND_MANTA_TARGET(1.0f);
				break;
			}
		} else {
			switch (self->mGeneration) {
			case 0:
				BLEND_MANTA_TARGET(1.0f);
				break;
			case 1:
				BLEND_MANTA_TARGET(0.8f);
				break;
			case 2:
				BLEND_MANTA_TARGET(0.8f);
				break;
			case 3:
				BLEND_MANTA_TARGET(0.6f);
				break;
			case 4:
				BLEND_MANTA_TARGET(0.8f);
				break;
			case 5:
				BLEND_MANTA_TARGET(1.0f);
				break;
			}
		}

		self->mAttractor = target;
	}

	if (time % self->mTurnTimer == 0)
		self->unk1A4 ^= 1;

	if (time % 100 == 0)
		self->updateAttractor();

	JGeometry::TVec3<f32> dir = self->mMoveDir;
	if (dir.squared() <= 0.0000038146973f)
		dir.zero();
	else
		dir.normalize();

	dir.scale(self->mMoveSpeed);
	self->mDirection.add(dir);
	if (self->mDirection.squared() <= 0.0000038146973f) {
		self->mDirection.zero();
	} else {
		self->mDirection.normalize();
	}

	return false;
}

#undef BLEND_MANTA_TARGET
BOOL TNerveMantaHitWater::execute(TSpineBase<TLiveActor>* spine) const
{
	TBossManta* self = (TBossManta*)spine->getBody();

	if (spine->getTime() == 0) {
		self->mMActor->setBckFromIndex(1);
		MActor* actor = self->mMActor;
		actor->setFrameRate(
		    SMSGetAnmFrameRate() * TBossManta::sFrameRate[self->mGeneration],
		    0);

		if (self->mMActor->unkC)
			self->mMActor->unkC->setMotionBlendRatio(0.0f);

		u32 sounds[] = { 0x8990, 0x8991, 0x8992, 0x8993, 0x899A, 0x899A };
		if (gpMSound->gateCheck(sounds[self->mGeneration]))
			MSoundSESystem::MSoundSE::startSoundActor(
			    sounds[self->mGeneration], &self->mPosition, 0, nullptr, 0,
			    4);
	}

	TBossMantaParams* params = (TBossMantaParams*)self->getSaveParam();
	int emitCount            = params->mSLDamageEffectNum.get();
	int effects[TBossManta::GENERATION_COUNT][3] = {
		{ 0x1CE, 0x1CF, 0x1D0 },
		{ 0x1CB, 0x1CC, 0x1CD },
		{ 0x1C9, -1, 0x1CA },
		{ 0x1C7, -1, 0x1C8 },
		{ 0x1C7, -1, 0x1C8 },
		{ 0x1C7, -1, 0x1C8 },
	};
	for (int i = 0; i < 3; ++i) {
		int effect = effects[self->mGeneration][i];
		if (effect <= 0)
			continue;

		for (int j = 0; j < emitCount; ++j) {
			gpMarioParticleManager->emitAndBindToPosPtr(
			    effect, &self->mCenterPos, 1,
			    (const void*)((u8*)self + j * sizeof(TBossManta)));
			if (i == 2)
				break;
		}
	}

	if (self->checkCurAnmEnd(0)) {
		self->unk150 = 0.5f;
		self->unk154 = 0;
		return true;
	}

	return false;
}
BOOL TNerveMantaSpawn::execute(TSpineBase<TLiveActor>* spine) const
{
	TBossManta* self = (TBossManta*)spine->getBody();

	if (spine->getTime() < 5)
		self->mScaling.scale(1.01f);
	else
		self->mScaling.scale(0.9f);

	if (spine->getTime() == 0) {
		int effects[] = { 0xFC, 0xFB, 0xFA, 0xF9 };
		int idx       = self->mGeneration;

		gpMarioParticleManager->emitAndBindToPosPtr(effects[idx],
		                                            &self->mCenterPos, 0,
		                                            self);
		u32 sounds[] = { 0x8994, 0x8995, 0x8996, 0x8997 };
		u32 sound    = sounds[self->mGeneration];
		if (gpMSound->gateCheck(sound))
			MSoundSESystem::MSoundSE::startSoundActor(
			    sound, &self->mPosition, 0, nullptr, 0, 4);

		((TBossMantaManager*)self->mManager)
		    ->spawn(self->mGeneration + 1, self->mPosition);
	}

	if (spine->getTime() == 30) {
		self->kill();
		return true;
	}

	return false;
}
BOOL TNerveMantaDeath::execute(TSpineBase<TLiveActor>* spine) const
{
	TBossManta* self = (TBossManta*)spine->getBody();

	if (spine->getTime() == 0) {
		self->mMActor->setBckFromIndex(0);
		MActor* actor = self->mMActor;
		actor->setFrameRate(SMSGetAnmFrameRate(), 0);

		if (self->mMActor->unkC)
			self->mMActor->unkC->setMotionBlendRatio(0.0f);
	}

	if (self->checkCurAnmEnd(0)) {
		gpMarioParticleManager->emitAndBindToPosPtr(0xF8, &self->mCenterPos,
		                                            0, self);
		if (gpMSound->gateCheck(0x8998))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x8998, &self->mPosition, 0, nullptr, 0, 4);
		self->kill();
	}

	return false;
}
BOOL TNerveMantaAppearDemo::execute(TSpineBase<TLiveActor>* spine) const
{
	TBossManta* self = (TBossManta*)spine->getBody();
	int time         = spine->getTime();

	if (time == 0) {
		self->mPosition.x      = 0.0f;
		self->mPosition.y      = 0.0f;
		TBossMantaParams* params = (TBossMantaParams*)self->getSaveParam();
		self->mPosition.z      = params->mSLAppearDemoInitialZ.get();
		self->mDirection.set(0.0f, 0.0f, -1.0f);
		self->mAttractor.set(0.0f, 0.0f, -10000.0f);
		if (self->mMActor->unkC)
			self->mMActor->unkC->initNormalMotionBlend();
		self->mMActor->setBckFromIndex(3);

		J3DAnmTransform* oldAnm;
		MActorAnmDataEach<J3DAnmTransformKey>* bckData
		    = self->mMActorKeeper->mActorAnmData->getUnk2C();
		if (bckData->unk0 > 4)
			oldAnm = (J3DAnmTransform*)bckData->unkC[4];
		else
			oldAnm = nullptr;

		if (self->mMActor->unkC)
			self->mMActor->unkC->setOldMotionBlendAnmPtr(oldAnm);
		if (self->mMActor->unkC)
			self->mMActor->unkC->setMotionBlendRatio(0.5f);

		MActor* actor = self->mMActor;
		actor->setFrameRate(
		    SMSGetAnmFrameRate() * TBossManta::sFrameRate[self->mGeneration],
		    0);
	}

	if (time == 720)
		MSBgm::startBGM(0x8001002B);

	if (time == 1680)
		return true;

	if (gpMSound->gateCheck(0x818E))
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x818E, &self->mPosition, 0, nullptr, 0, 4);

	TBossMantaParams* params = (TBossMantaParams*)self->getSaveParam();
	self->mAppearSpeed = params->mSLAppearDemoWalkSpeed.get();

	return false;
}
#pragma dont_inline on
TBossManta::TBossManta(const char* name)
    : TSpineEnemy(name)
    , mAttractor(0.0f, 0.0f, 0.0f)
    , mMoveDir(0.0f, 0.0f, 0.0f)
    , mDirection(0.0f, 0.0f, 1.0f)
    , mCenterPos(0.0f, 0.0f, 0.0f)
    , mTurnTimer(0)
    , mGeneration(0)
    , mAppearSpeed(0.0f)
    , mMoveSpeed(0.0f)
    , unk198(0.0f)
    , mWaterHitCount(0)
    , unk1A0(0)
{
}
#pragma dont_inline off
BOOL TBossManta::getIntoGraphVec(JGeometry::TVec3<f32>* out)
{
	TGraphWeb* graph = unk124->getGraph();

	for (int i = 0; i < 12; ++i) {
		const JGeometry::TVec3<f32>& current = graph->indexToPoint(i);
		f32 dx                              = current.x - mPosition.x;
		f32 dz                              = current.z - mPosition.z;
		const JGeometry::TVec3<f32>& next = graph->indexToPoint(i + 1);
		const JGeometry::TVec3<f32>& current2 = graph->indexToPoint(i);
		f32 edgeZ = current2.z - next.z;
		f32 edgeX = current2.x - next.x;
		if (dz * edgeX - dx * edgeZ < 0.0f) {
			JGeometry::TVec3<f32> up(0.0f, 1.0f, 0.0f);
			JGeometry::TVec3<f32> edge(edgeX, 0.0f, edgeZ);
			out->cross(up, edge);
			out->normalize();
			return true;
		}
	}

	const JGeometry::TVec3<f32>& current = graph->indexToPoint(12);
	f32 dx                              = current.x - mPosition.x;
	f32 dz                              = current.z - mPosition.z;
	const JGeometry::TVec3<f32>& next = graph->indexToPoint(0);
	const JGeometry::TVec3<f32>& current2 = graph->indexToPoint(12);
	f32 edgeZ                           = current2.z - next.z;
	f32 edgeX                           = current2.x - next.x;
	if (dz * edgeX - dx * edgeZ < 0.0f) {
		JGeometry::TVec3<f32> up(0.0f, 1.0f, 0.0f);
		JGeometry::TVec3<f32> edge(edgeX, 0.0f, edgeZ);
		out->cross(up, edge);
		out->normalize();
		return true;
	}

	return false;
}

void TBossManta::init(TLiveManager* manager)
{
	mManager = manager;
	manager->manageActor(this);

	mMActorKeeper = new TMActorKeeper(manager, 1);
	mMActor       = mMActorKeeper->createMActor("manta.bmd", 0);

	unk124->unk0 = gpConductor->getGraphByName("main");
	mHitPoints   = getSaveParam()
	                 ? ((TBossMantaParams*)getSaveParam())->mSLHitPointMax.get()
	                 : 1;
	mHeadHeight = 5000.0f;

	initHitActor(0x08000004, 1, 0x80000000, 0.0f, 0.0f, 0.0f, 0.0f);
	mDirection.set(0.0f, 0.0f, 1.0f);
	calcRootMatrix();
	reset();

	JDrama::TNameRefGen::search<TIdxGroupObj>("オブジェクトグループ")
	    ->getChildren()
	    .push_back(this);

	if (mInstanceIndex == 0) {
		JUTNameTab* jointName
		    = getModel()->getModelData()->getJointName();
		sCenterJointIndex = jointName->getIndex("center");
		sBodyJointIndex   = jointName->getIndex("jnt_body");
		sRwingJointIndex  = jointName->getIndex("jnt_Rwing2");
		sLwingJointIndex  = jointName->getIndex("jnt_Lwing2");

		TBossMantaManager* bossManager = (TBossMantaManager*)mManager;
		for (int i = 0; i < 8; ++i)
			bossManager->mCollisionSets[i]
			    = new TBossMantaAdditionalCollisionSet();
	}

	mLiveFlag |= LIVE_FLAG_UNK8;
	mLiveFlag &= ~LIVE_FLAG_UNK100;
	unk1A4 = 0;
	unk150 = 0.5f;
}

void TBossManta::moveObject()
{
	TLiveActor::moveObject();

	u8 stampGeneration[] = { 1, 1, 1, 1, 1, 1 };
	if (stampGeneration[mGeneration])
		gpPollution->stamp(1, mPosition.x, mPosition.y, mPosition.z,
		                    getPolluteRadius());

	for (int i = 0; i < mColCount; ++i) {
		THitActor* other = mCollisions[i];
		if (!other->isActorType(ACTOR_TYPE_PLAYER | 1))
			continue;

		AttackMario(this);
	}
}

bool TBossManta::collidedWithWater()
{
	if (unk1A0 > 0)
		return false;

	if (mSpine->getLatestNerve() != &TNerveMantaMove::theNerve()
	    && mSpine->getLatestNerve() != &TNerveMantaHitWater::theNerve())
		return false;

	bool inHitWater
	    = mSpine->getLatestNerve() == &TNerveMantaHitWater::theNerve();

	int damage[] = { 16, 8, 4, 2, 1, 1 };
	if (mWaterHitCount >= damage[mGeneration])
		return true;

	unk1A0 = 30;
	if (!inHitWater)
		mSpine->setNext(&TNerveMantaHitWater::theNerve());

	++mWaterHitCount;
	if (mWaterHitCount == damage[mGeneration]) {
		if (mGeneration >= 4)
			mSpine->pushAfterCurrent(&TNerveMantaDeath::theNerve());
		else
			mSpine->pushAfterCurrent(&TNerveMantaSpawn::theNerve());
	} else {
		mSpine->pushAfterCurrent(&TNerveMantaMove::theNerve());
	}

	return true;
}
BOOL TBossManta::receiveMessage(THitActor* sender, u32 message)
{
	if (message == HIT_MESSAGE_SPRAYED_BY_WATER) {
		u32 slot = *(u32*)((u8*)sender + 0x68);
		if (gpModelWaterManager->mParticleFlagSOA[slot] & 0x40)
			return collidedWithWater();
	}

	return false;
}
void TBossManta::initNthGeneration(int generation)
{
	f32 yScale[GENERATION_COUNT]
	    = { 10.0f, 5.0f, 1.0f, 0.42f, 0.42f, 0.42f };

	mGeneration = generation;
	mScaling.x = mScaling.z = sScale[mGeneration];
	mScaling.y              = yScale[mGeneration];
	mWaterHitCount = 0;
	unk150         = 0.0f;
	unk154         = 49;

	switch (mGeneration) {
	case 0:
		mSpine->initWith(&TNerveMantaAppearDemo::theNerve());
		mSpine->pushAfterCurrent(&TNerveMantaMove::theNerve());
		mTurnTimer    = 600;
		mAppearSpeed  = 0.0f;
		mMoveSpeed    = 0.0f;
		unk198        = 0.0f;
		unk1A0        = 0;
		break;
	case 1:
		mSpine->initWith(&TNerveMantaMove::theNerve());
		mTurnTimer   = 600 + (int)(rand() * 0.000030517578f * 100.0f);
		mAppearSpeed = 2.0f;
		mMoveSpeed   = 0.009f;
		unk198       = 1000.0f;
		unk1A0       = 120;
		break;
	case 2:
		mSpine->initWith(&TNerveMantaMove::theNerve());
		mTurnTimer   = 200 + (int)(rand() * 0.000030517578f * 100.0f);
		mAppearSpeed = 3.0f;
		mMoveSpeed   = 0.009f;
		unk198       = 1000.0f;
		unk1A0       = 120;
		break;
	case 3:
		mSpine->initWith(&TNerveMantaMove::theNerve());
		mTurnTimer   = 100 + (int)(rand() * 0.000030517578f * 100.0f);
		mAppearSpeed = 4.0f;
		mMoveSpeed   = 0.019f;
		unk198       = 700.0f;
		unk1A0       = 120;
		break;
	case 4:
		mSpine->initWith(&TNerveMantaMove::theNerve());
		mTurnTimer   = 100 + (int)(rand() * 0.000030517578f * 100.0f);
		mAppearSpeed = 7.0f;
		mMoveSpeed   = 0.03f;
		unk198       = 400.0f;
		unk1A0       = 360;
		break;
	case 5:
		mSpine->initWith(&TNerveMantaMove::theNerve());
		mTurnTimer   = 100 + (int)(rand() * 0.000030517578f * 100.0f);
		mAppearSpeed = 3.0f;
		mMoveSpeed   = 0.03f;
		unk198       = 200.0f;
		unk1A0       = 0;
		break;
	}

	if (mGeneration <= 2) {
		setHitParams(0.0f, 0.0f, 0.0f, 0.0f);
	} else {
		f32 radius = 80.0f * mScaling.x;
		setHitParams(radius, 100.0f, radius, 100.0f);
	}

	offHitFlag(HIT_FLAG_NO_COLLISION);
	mLiveFlag &= ~LIVE_FLAG_DEAD;

	if (mGeneration <= 2) {
		TBossMantaManager* manager = (TBossMantaManager*)mManager;
		for (int i = 0; i < 8; ++i) {
			TBossMantaAdditionalCollisionSet* set = manager->mCollisionSets[i];
			TBossManta* owner = set->mOwner;
			bool occupied;
			if (owner && !(owner->mLiveFlag & LIVE_FLAG_DEAD))
				occupied = true;
			else
				occupied = false;

			if (!occupied) {
				set->adapt(this);
				break;
			}
		}
	}
}
void TBossManta::control()
{
	if (unk1A0 > 0)
		--unk1A0;

	if (mMActor->checkCurBckFromIndex(3)) {
		JGeometry::TVec3<f32> dir = mMoveDir;
		dir.normalize();

		f32 blendRate[GENERATION_COUNT]
		    = { 0.005f, 0.008f, 0.01f, 0.03f, 0.05f, 0.05f };

		f32 cross  = mDirection.z * dir.x - mDirection.x * dir.z;
		f32 target = 0.5f - 0.4f * cross;
		f32 rate   = blendRate[mGeneration];
		unk150     = rate * target + (1.0f - rate) * unk150;

		f32 blendWave[50] = {
			0.0f,           0.0460000001f, 0.107000001f,  0.177000001f,
			0.226999998f,   0.256000012f,  0.273000002f,  0.279000014f,
			0.273999989f,   0.25999999f,   0.237000003f,  0.209000006f,
			0.174999997f,   0.137999997f,  0.100000001f,  0.0610000007f,
			0.0240000002f,  -0.0109999999f, -0.0430000015f, -0.0700000003f,
			-0.0920000002f, -0.108000003f, -0.119999997f, -0.126000002f,
			-0.127000004f,  -0.123000003f, -0.115000002f, -0.104000002f,
			-0.0900000036f, -0.074000001f, -0.057f,       -0.0399999991f,
			-0.0219999999f, -0.00499999989f, 0.00999999978f, 0.0240000002f,
			0.0350000001f,  0.0450000018f, 0.050999999f,  0.0560000017f,
			0.0579999983f,  0.057f,        0.0549999997f, 0.050999999f,
			0.0460000001f,  0.0390000008f, 0.0309999995f, 0.023f,
			0.0149999997f,  0.00700000022f,
		};

		f32 blend = unk150 + blendWave[unk154];
		if (blend < 0.0f)
			blend = 0.0f;
		else if (blend > 1.0f)
			blend = 1.0f;

		if (mMActor->unkC)
			mMActor->unkC->setMotionBlendRatio(blend);
	} else {
		if (mMActor->unkC)
			mMActor->unkC->setMotionBlendRatio(0.0f);
	}

	JGeometry::TVec3<f32> velocity = mDirection;
	velocity.scale(mAppearSpeed);
	JGeometry::TVec3<f32> oldVelocity = mVelocity;
	velocity.y                       = oldVelocity.y;
	mVelocity                        = velocity;

	TLiveActor::control();
}
void TBossManta::calcRootMatrix()
{
	if (unk154 < 49)
		++unk154;

	Mtx root;
	root[0][3] = mPosition.x;
	root[1][3] = mPosition.y;
	root[2][3] = mPosition.z;

	JGeometry::TVec3<f32> up(0.0f, 1.0f, 0.0f);
	JGeometry::TVec3<f32> side;
	side.cross(up, mDirection);

	root[0][0] = side.x;
	root[1][0] = side.y;
	root[2][0] = side.z;
	root[0][1] = up.x;
	root[1][1] = up.y;
	root[2][1] = up.z;
	root[0][2] = mDirection.x;
	root[1][2] = mDirection.y;
	root[2][2] = mDirection.z;

	MtxPtr center = getModel()->getAnmMtx(sCenterJointIndex);
	mCenterPos.set(center[0][3], mPosition.y, center[2][3]);
	getModel()->setBaseScale(mScaling);
	getModel()->setBaseTRMtx(root);
}
#pragma dont_inline on
f32 TBossManta::getPolluteRadius()
{
	switch (mGeneration) {
	case 0:
	case 1:
	case 2:
	case 3:
		return ((TBossMantaParams*)getSaveParam())->mSLPolluteRadius.get()
		       * mScaling.x;
	case 4:
	case 5:
		return 100.0f;
	default:
		return 0.0f;
	}
}
#pragma dont_inline off
void TBossManta::updateAttractor()
{
	JGeometry::TVec3<f32> force = mAttractor;
	force.sub(mPosition);
	force.y = 0.0f;
	force.normalize();
	force.scale(
	    ((TBossMantaParams*)getSaveParam())->mSLAttractorPower.get());

	JGeometry::TVec3<f32> ownLook = mDirection;
	ownLook.scale(
	    ((TBossMantaParams*)getSaveParam())->mSLEscapeLookPoint.get());
	JGeometry::TVec3<f32> ownPos = mPosition;
	ownPos.add(ownLook);
	ownPos.y = 0.0f;

	TBossMantaManager* manager = (TBossMantaManager*)mManager;
	for (int i = 0; i < manager->getActiveObjNum(); ++i) {
		TBossManta* other = (TBossManta*)manager->getObj(i);
		if (mLiveFlag & LIVE_FLAG_DEAD)
			continue;
		if (other->mLiveFlag & LIVE_FLAG_DEAD)
			continue;
		if (other->mInstanceIndex == mInstanceIndex)
			continue;

		JGeometry::TVec3<f32> diff = ownPos;
		JGeometry::TVec3<f32> otherLook = other->mDirection;
		otherLook.scale(
		    ((TBossMantaParams*)getSaveParam())->mSLEscapeLookedPoint.get());
		JGeometry::TVec3<f32> otherPos = other->mPosition;
		otherPos.add(otherLook);

		diff.sub(otherPos);
		diff.y   = 0.0f;
		if (diff.length() > 0.1f
		    && diff.length()
		           < ((TBossMantaParams*)getSaveParam())->mSLEscapeRegion.get()) {
			diff.normalize();
			f32 dist = diff.length();
			diff.scale(
			    ((TBossMantaParams*)getSaveParam())->mSLPusherPower.get()
			    / dist);
			force.add(diff);
		}
	}

	JGeometry::TVec3<f32> away = mPosition;
	away.sub(SMS_GetMarioPos());
	away.y = 0.0f;
	f32 awayDist = away.length();
	if ((TBossManta::sEscapeFromMario || mGeneration == 4)
	    && awayDist < 6000.0f)
		force.add(away);

	JGeometry::TVec3<f32> graphVec;
	force.y = 0.0f;
	if (getIntoGraphVec(&graphVec)) {
		graphVec.y = 0.0f;
		graphVec.scale(10000.0f);
		force.add(graphVec);
	}

	mMoveDir = force;
}
void TBossMantaManager::TMantaBattleState::update()
{
	switch (mState) {
	case 0:
		if (TFlagManager::smInstance->getBool(0x50007)) {
			TMarDirector* director = gpMarDirector;
			director->fireStartDemoCamera(
			    "sirena_manta", nullptr, -1, 0.0f, true, nullptr, 0,
			    nullptr, JDrama::TFlagT<u16>(0));
			TBossManta* manta = (TBossManta*)mManager->getObj(0);
			manta->initNthGeneration(0);
			MSBgm::stopTrackBGMs(7, 10);
			++mState;
		}
		break;
	case 1: {
		bool done  = true;
		for (int i = 0; i < mManager->getActiveObjNum(); ++i) {
			TBossManta* manta = (TBossManta*)mManager->getObj(i);
			if (manta->mLiveFlag & LIVE_FLAG_DEAD)
				continue;
			if (manta->mGeneration == 4)
				continue;
			done = false;
			break;
		}
		if (done) {
			for (int i = 0; i < mManager->getActiveObjNum(); ++i) {
				TBossManta* manta = (TBossManta*)mManager->getObj(i);
				if (!(manta->mLiveFlag & LIVE_FLAG_DEAD))
					manta->initNthGeneration(5);
			}
			++mState;
		}
		break;
	}
	case 2: {
		bool done = true;
		for (int i = 0; i < mManager->getActiveObjNum(); ++i) {
			TBossManta* manta = (TBossManta*)mManager->getObj(i);
			if (manta->mGeneration == 5
			    && !(manta->mLiveFlag & LIVE_FLAG_DEAD)) {
				done = false;
				break;
			}
		}
		if (done) {
			MSBgm::stopTrackBGMs(7, 10);
			sDefeatSE = nullptr;
			if (gpMSound->gateCheck(0x898F))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x898F, nullptr, 0, &sDefeatSE, 0, 4);
			++mState;
		}
		break;
	}
	case 3:
		if (!sDefeatSE) {
			JDrama::TActor* event
			    = JDrama::TNameRefGen::search<JDrama::TActor>(
			        "イベント（ホテル沈む）");
			*(u8*)((u8*)event + 0x64) = 1;
			++mState;
		}
		break;
	case 4:
		break;
	}
}
void TBossMantaManager::TMantaMessageState::update()
{
	switch (mState) {
	case 0: {
		TBossManta* manta = (TBossManta*)mManager->getObj(0);
		if (manta->mSpine->getLatestNerve()
		    == &TNerveMantaSpawn::theNerve()) {
			gpMarDirector->getConsole()->startAppearBalloon(0xE000C, true);
			++mState;
		}
		break;
	}
	case 1: {
		int i     = 0;
		int count = 0;
		for (; i < mManager->getActiveObjNum(); ++i) {
			TBossManta* manta = (TBossManta*)mManager->getObj(i);
			if (!(manta->mLiveFlag & LIVE_FLAG_DEAD))
				++count;
		}
		if (count > 50) {
			gpMarDirector->getConsole()->startAppearBalloon(0xE000D, true);
			++mState;
		}
		break;
	}
	case 2:
		if (mManager->mBattleState.mState == 2) {
			gpMarDirector->getConsole()->startAppearBalloon(0xE000E, true);
			++mState;
		}
		break;
	case 3:
		break;
	}
}
#pragma dont_inline on
TBossMantaAdditionalCollisionSet::TBossMantaAdditionalCollisionSet()
    : mOwner(nullptr)
{
	for (int i = 0; i < 3; ++i)
		mCollisions[i]
		    = new TBossMantaAdditionalCollision("マンタ追加コリジョン");
}
#pragma dont_inline on
void TBossMantaAdditionalCollisionSet::adapt(TBossManta* manta)
{
	mOwner = manta;

	f32 radius = mOwner->mScaling.x * 54.0f;
	mCollisions[0]->setHitParams(radius, 100.0f, radius, 100.0f);

	radius = mOwner->mScaling.x * 26.0f;
	mCollisions[1]->setHitParams(radius, 100.0f, radius, 100.0f);

	radius = mOwner->mScaling.x * 26.0f;
	mCollisions[2]->setHitParams(radius, 100.0f, radius, 100.0f);

	mCollisions[0]->mOwner = mOwner;
	mCollisions[1]->mOwner = mOwner;
	mCollisions[2]->mOwner = mOwner;
}
#pragma dont_inline off
void TBossMantaAdditionalCollisionSet::update(u32 flags,
                                              JDrama::TGraphics* graphics)
{
	if (!mOwner)
		return;

	if (mOwner->mLiveFlag & LIVE_FLAG_DEAD) {
		mOwner = nullptr;
		return;
	}

	for (int i = 0; i < 3; ++i)
		mCollisions[i]->perform(flags, graphics);

	MtxPtr center = mOwner->getModel()->getAnmMtx(TBossManta::sCenterJointIndex);
	f32 centerX   = center[0][3];
	f32 centerY   = center[1][3];
	f32 centerZ   = center[2][3];

	MtxPtr right = mOwner->getModel()->getAnmMtx(TBossManta::sRwingJointIndex);
	f32 rightX   = right[0][3];
	f32 rightY   = right[1][3];
	f32 rightZ   = right[2][3];

	MtxPtr left = mOwner->getModel()->getAnmMtx(TBossManta::sLwingJointIndex);
	f32 leftX   = left[0][3];
	f32 leftY   = left[1][3];
	f32 leftZ   = left[2][3];

	MtxPtr body = mOwner->getModel()->getAnmMtx(TBossManta::sBodyJointIndex);
	f32 bodyX   = body[0][3];
	f32 bodyY   = body[1][3];
	f32 bodyZ   = body[2][3];

	mCollisions[0]->mPosition.set(centerX + -0.15f * (rightX - centerX),
	                              centerY + -0.15f * (rightY - centerY),
	                              centerZ + -0.15f * (rightZ - centerZ));
	mCollisions[1]->mPosition.set(centerX + 0.75f * (leftX - centerX),
	                              centerY + 0.75f * (leftY - centerY),
	                              centerZ + 0.75f * (leftZ - centerZ));
	mCollisions[2]->mPosition.set(centerX + 0.75f * (bodyX - centerX),
	                              centerY + 0.75f * (bodyY - centerY),
	                              centerZ + 0.75f * (bodyZ - centerZ));
}
BOOL TBossMantaAdditionalCollision::receiveMessage(THitActor* sender,
                                                   u32 message)
{
	if (!mOwner)
		return false;

	return mOwner->receiveMessage(sender, message);
}
TBossMantaAdditionalCollision::TBossMantaAdditionalCollision(const char* name)
    : THitActor(name)
    , mOwner(nullptr)
{
	initHitActor(0x08000004, 1, 0x80000000, 0.0f, 0.0f, 0.0f, 0.0f);
	offHitFlag(HIT_FLAG_NO_COLLISION);

	JDrama::TNameRefGen::search<TIdxGroupObj>("オブジェクトグループ")
	    ->insert(this);
}
void TBossMantaAdditionalCollision::perform(u32 flags,
                                            JDrama::TGraphics* graphics)
{
	THitActor::perform(flags, graphics);

	if (!(flags & 1))
		return;

	for (int i = 0; i < mColCount; ++i) {
		THitActor* other = mCollisions[i];
		if (!other->isActorType(ACTOR_TYPE_PLAYER | 1))
			continue;

		AttackMario(this);
	}
}
TBossMantaManager::TBossMantaManager(const char* name)
    : TEnemyManager(name)
    , mEfbAlpha(nullptr)
    , unk80(0)
    , mBattleState(this)
    , mMessageState(this)
{
	mEfbAlpha = new (0x20) u8[(u16)SMSGetGameRenderWidth()
	                          * (u16)SMSGetGameRenderHeight()];
	mPalmPositions   = new JGeometry::TVec3<f32>[7];
	mEscapePositions = new JGeometry::TVec3<f32>[2];
}

void TBossMantaManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TBossMantaParams("/enemy/bossmanta.prm");
	TEnemyManager::load(stream);
}

void TBossMantaManager::loadAfter()
{
	static const char* onetimeFilenames[] = {
		"/scene/manta/jpa/ms_man_dead.jpa",
		"/scene/manta/jpa/ms_man_div1.jpa",
		"/scene/manta/jpa/ms_man_div2.jpa",
		"/scene/manta/jpa/ms_man_div3.jpa",
		"/scene/manta/jpa/ms_man_div4.jpa",
	};
	static const char* loopFilenames[] = {
		"/scene/manta/jpa/ms_man_hit1_a.jpa",
		"/scene/manta/jpa/ms_man_hit1_b.jpa",
		"/scene/manta/jpa/ms_man_hit2_a.jpa",
		"/scene/manta/jpa/ms_man_hit2_b.jpa",
		"/scene/manta/jpa/ms_man_hit3_a1.jpa",
		"/scene/manta/jpa/ms_man_hit3_a2.jpa",
		"/scene/manta/jpa/ms_man_hit3_b.jpa",
		"/scene/manta/jpa/ms_man_hit4_a1.jpa",
		"/scene/manta/jpa/ms_man_hit4_a2.jpa",
		"/scene/manta/jpa/ms_man_hit4_b.jpa",
	};
	for (int i = 0; i < 5; ++i)
		SMS_LoadParticle(onetimeFilenames[i], i + 0xF8);

	for (int i = 0; i < 10; ++i)
		SMS_LoadParticle(loopFilenames[i], i + 0x1C7);

	char buffer[0x40];
	for (int i = 0; i < 7; ++i) {
		snprintf(buffer, sizeof(buffer), "palmOugi %d", i);
		JDrama::TActor* palm
		    = JDrama::TNameRefGen::search<JDrama::TActor>(buffer);
		mPalmPositions[i].set(palm->mPosition.x, 0.0f, palm->mPosition.z);
	}

	mEscapePositions[0].set(1549.0f, 0.0f, -435.0f);
	mEscapePositions[1].set(-1457.0f, 0.0f, -533.0f);

	mBattleState.mState          = 0;
	mMessageState.mState         = 0;
	mShadowAlphaTimer            = 0;
	TBossManta::sEscapeFromMario = false;
}
void TBossMantaManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "manta.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}
TSpineEnemy* TBossMantaManager::createEnemyInstance()
{
	return new TBossManta("ボスマンタ");
}
void TBossMantaManager::drawMantaShadow(JDrama::TGraphics* graphics)
{
	setupEfbAlpha(graphics);

	for (int i = 0; i < getActiveObjNum(); ++i) {
		TBossManta* manta = (TBossManta*)getObj(i);
		if (manta->mLiveFlag & (LIVE_FLAG_DEAD | LIVE_FLAG_CLIPPED_OUT))
			continue;

		GXSetCullMode(GX_CULL_FRONT);
		GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ONE, GX_LO_NOOP);
		for (u16 j = 0; j < manta->getModel()->getModelData()->getShapeNum();
		     ++j)
			manta->getModel()->getShapePacket(j)->draw();

		GXSetCullMode(GX_CULL_BACK);
		GXSetBlendMode(GX_BM_SUBTRACT, GX_BL_ONE, GX_BL_ONE, GX_LO_NOOP);
		for (u16 j = 0; j < manta->getModel()->getModelData()->getShapeNum();
		     ++j)
			manta->getModel()->getShapePacket(j)->draw();
	}

	GXSetTexCopySrc(0, 0, SMSGetGameRenderWidth(), SMSGetGameRenderHeight());
	GXSetTexCopyDst(SMSGetGameRenderWidth(), SMSGetGameRenderHeight(), GX_TF_I8,
	                GX_FALSE);
	GXCopyTex(mEfbAlpha, GX_FALSE);
	GXPixModeSync();

	f32 screenWidth  = (u16)SMSGetGameRenderWidth();
	f32 screenHeight = (u16)SMSGetGameRenderHeight();

	Mtx44 ortho;
	C_MTXOrtho(ortho, screenHeight, 0.0f, 0.0f, screenWidth, 0.0f,
	           1000.0f);
	GXSetProjection(ortho, GX_ORTHOGRAPHIC);
	GXSetNumTevStages(1);
	GXSetNumChans(0);
	GXSetNumTexGens(1);
	GXSetZCompLoc(GX_FALSE);

	GXTexObj texObj;
	GXInitTexObj(&texObj, mEfbAlpha, SMSGetGameRenderWidth(),
	             SMSGetGameRenderHeight(), GX_TF_I8, GX_CLAMP, GX_CLAMP,
	             GX_FALSE);
	GXInitTexObjLOD(&texObj, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f,
	                GX_FALSE, GX_FALSE, GX_ANISO_1);

	GXSetTevKColorSel(GX_TEVSTAGE0, GX_TEV_KCSEL_K0);
	GXSetTevKAlphaSel(GX_TEVSTAGE0, GX_TEV_KASEL_K0_A);

	if (mBattleState.mState == 2) {
		int timer = mShadowAlphaTimer + 1;
		if (timer > 15)
			timer = 15;
		mShadowAlphaTimer = timer;
	}

	TBossMantaParams* params = (TBossMantaParams*)unk38;
	f32 ratio                = mShadowAlphaTimer / 15.0f;
	f32 invRatio             = 1.0f - ratio;
	GXColor color;
	color.r = ratio * params->mSLAngryMantaRed.get()
	          + invRatio * params->mSLMantaRed.get();
	color.g = ratio * params->mSLAngryMantaGreen.get()
	          + invRatio * params->mSLMantaGreen.get();
	color.b = ratio * params->mSLAngryMantaBlue.get()
	          + invRatio * params->mSLMantaBlue.get();
	color.a = ratio * params->mSLAngryMantaAlpha.get()
	          + invRatio * params->mSLMantaAlpha.get();
	GXSetTevKColor(GX_KCOLOR0, color);

	GXLoadTexObj(&texObj, GX_TEXMAP0);
	GXSetDstAlpha(GX_TRUE, 0xff);
	GXSetZMode(GX_FALSE, GX_NEVER, GX_FALSE);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
	GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD_NULL, GX_TEXMAP_NULL,
	              GX_COLOR_NULL);
	GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY,
	                  GX_FALSE, GX_PTIDENTITY);
	GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_ZERO, GX_CA_KONST,
	                GX_CA_ZERO);
	GXSetTevAlphaOp(GX_TEVSTAGE0, (GXTevOp)8, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXA, GX_CC_ZERO, GX_CC_KONST,
	                GX_CC_ZERO);
	GXSetTevColorOp(GX_TEVSTAGE0, (GXTevOp)8, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetDstAlpha(GX_FALSE, 0);
	GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_INVSRCALPHA, GX_LO_NOOP);
	GXSetAlphaUpdate(GX_FALSE);
	GXSetColorUpdate(GX_TRUE);

	Mtx identity;
	PSMTXIdentity(identity);
	GXLoadPosMtxImm(identity, GX_PNMTX0);
	GXSetCurrentMtx(GX_PNMTX0);
	GXSetCullMode(GX_CULL_NONE);
	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	GXBegin(GX_QUADS, GX_VTXFMT0, 4);
	GXPosition3f32(0.0f, (u16)SMSGetGameRenderHeight(), -10.0f);
	GXTexCoord2f32(0.0f, 0.0f);
	GXPosition3f32((u16)SMSGetGameRenderWidth(),
	               (u16)SMSGetGameRenderHeight(), -10.0f);
	GXTexCoord2f32(1.0f, 0.0f);
	GXPosition3f32((u16)SMSGetGameRenderWidth(), 0.0f, -10.0f);
	GXTexCoord2f32(1.0f, 1.0f);
	GXPosition3f32(0.0f, 0.0f, -10.0f);
	GXTexCoord2f32(0.0f, 1.0f);
	GXEnd();

	GXSetProjection(graphics->mProjMtx.mMtx, GX_PERSPECTIVE);
}
void TBossMantaManager::updateMantaEscape()
{
	TBossManta::sEscapeFromMario = 0;

	JGeometry::TVec3<f32> marioPos2 = SMS_GetMarioPos();
	JGeometry::TVec3<f32> marioPos(marioPos2.x, 0.0f, marioPos2.z);

	for (int i = 0; i < 7; ++i) {
		if (mPalmPositions[i].distance(marioPos) < 350.0f)
			TBossManta::sEscapeFromMario = 1;
	}

	for (int i = 0; i < 2; ++i) {
		if (mEscapePositions[i].distance(marioPos) < 820.0f)
			TBossManta::sEscapeFromMario = 1;
	}
}
void TBossMantaManager::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TEnemyManager::perform(flags, graphics);

	if (flags & 8)
		drawMantaShadow(graphics);

	if (flags & 1) {
		mBattleState.update();
		mMessageState.update();
		updateMantaEscape();
	}

	for (int i = 0; i < 8; ++i)
		mCollisionSets[i]->update(flags, graphics);
}
void TBossMantaManager::setupEfbAlpha(JDrama::TGraphics* graphics)
{
	ReInitializeGX();

	f32 width  = (u16)SMSGetGameRenderWidth();
	f32 height = (u16)SMSGetGameRenderHeight();

	Mtx44 ortho;
	Mtx identity;
	C_MTXOrtho(ortho, height, 0.0f, 0.0f, width, 0.0f, 1000.0f);

	GXSetProjection(ortho, GX_ORTHOGRAPHIC);
	GXSetColorUpdate(GX_FALSE);
	GXSetAlphaUpdate(GX_TRUE);
	GXSetDstAlpha(GX_TRUE, 0);
	GXSetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
	PSMTXIdentity(identity);
	GXLoadPosMtxImm(identity, GX_PNMTX0);
	GXSetCurrentMtx(GX_PNMTX0);
	GXSetCullMode(GX_CULL_NONE);

	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);

	GXBegin(GX_QUADS, GX_VTXFMT0, 4);
	GXPosition3f32(0.0f, (u16)SMSGetGameRenderHeight(), -10.0f);
	GXPosition3f32((u16)SMSGetGameRenderWidth(),
	               (u16)SMSGetGameRenderHeight(), -10.0f);
	GXPosition3f32((u16)SMSGetGameRenderWidth(), 0.0f, -10.0f);
	GXPosition3f32(0.0f, 0.0f, -10.0f);
	GXEnd();

	GXSetNumChans(1);
	GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetNumTexGens(0);
	GXSetNumTevStages(1);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL,
	              GX_COLOR0A0);
	GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

	GXSetChanMatColor(GX_COLOR0A0,
	                  (GXColor) { 0xff, 0xff, 0xff, 0xff });
	GXSetAlphaUpdate(GX_TRUE);
	GXSetDstAlpha(GX_FALSE, 0);
	GXSetZMode(GX_TRUE, GX_GEQUAL, GX_FALSE);
	GXSetAlphaUpdate(GX_TRUE);
	GXSetProjection(graphics->mProjMtx.mMtx, GX_PERSPECTIVE);
}
void TBossMantaManager::createEnemies(int count)
{
	if (mObjNum + count > mCapacity)
		count = mCapacity - mObjNum;

	if (unk38 && mObjNum + count > unk38->mSLInstanceNum.get())
		count = unk38->mSLInstanceNum.get() - mObjNum;

	if (count < 0)
		return;

	for (int i = 0; i < count; ++i) {
		TSpineEnemy* enemy = createEnemyInstance();
		if (!enemy)
			continue;

		JDrama::TNameRefGen::search<TIdxGroupObj>("オブジェクトグループ")
		    ->add(enemy);

		enemy->init(this);
	}
}
#pragma dont_inline on
void TBossMantaManager::spawn(int generation,
                              const JGeometry::TVec3<f32>& position)
{
	int spawnCounts[] = { 1, 2, 3, 4, 4 };

	int* count = &spawnCounts[generation];
	f32 start  = 3.1415927f * (2.0f * (rand() * 0.000030517578f));

	for (int i = 0; i < *count; ++i) {
		TBossManta* manta = (TBossManta*)getDeadEnemy();
		if (!manta)
			break;

		TRotation3f rot;
		JGeometry::TVec3<f32> dir(0.0f, 0.0f, 1.0f);
		f32 angle = start + ((2.0f * i) * 3.1415927f) / *count;
		rot.setEularY(angle);
		rot.mult(dir, dir);
		manta->mDirection = dir;
		manta->mPosition = position;
		manta->initNthGeneration(generation);
	}
}
#pragma dont_inline off
TBossMantaParams::TBossMantaParams(const char* path)
    : TSpineEnemyParams(path)
    , PARAM_INIT(mSLPolluteRadius, 100.0f)
    , PARAM_INIT(mSLDamageEffectNum, 3)
    , PARAM_INIT(mSLAppearDemoInitialZ, 7600.0f)
    , PARAM_INIT(mSLAppearDemoWalkSpeed, 3.0f)
    , PARAM_INIT(mSLMantaRed, 0xC4)
    , PARAM_INIT(mSLMantaGreen, 0x80)
    , PARAM_INIT(mSLMantaBlue, 0x5F)
    , PARAM_INIT(mSLMantaAlpha, 0x80)
    , PARAM_INIT(mSLAngryMantaRed, 0xD2)
    , PARAM_INIT(mSLAngryMantaGreen, 0x1E)
    , PARAM_INIT(mSLAngryMantaBlue, 0x5A)
    , PARAM_INIT(mSLAngryMantaAlpha, 0x80)
    , PARAM_INIT(mSLAttractorPower, 622.0f)
    , PARAM_INIT(mSLPusherPower, 23559.0f)
    , PARAM_INIT(mSLEscapeLookPoint, 1000.0f)
    , PARAM_INIT(mSLEscapeLookedPoint, 1000.0f)
    , PARAM_INIT(mSLEscapeRegion, 500.0f)
{
	TParams::load(mPrmPath);
}
