#include <Animal/BeeHive.hpp>
#include <Animal/BoidLeader.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JDrama/JDRViewObjPtrList.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <M3DUtil/MActor.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <MSound/MSSetSound.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MoveBG/MapObjBase.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>
#include <System/Particles.hpp>
#include <math.h>

namespace {
f32 cAngleLimit = 1.1780972f;

static void playBeeHiveSound(const JGeometry::TVec3<f32>& pos, f32 volume)
{
	if (gpMSound->gateCheck(0x28f7))
		MSoundSESystem::MSoundSE::startSoundActorWithInfo(
		    0x28f7, (const Vec*)&pos, nullptr, volume, 0, 0, nullptr, 0, 4);
}

static void setBoidLeaderWaitParams(TBoidLeader* leader)
{
	leader->mParam20 = 25.0f;
	leader->mParam24 = 80.0f;
	leader->mParam28 = 8.0f;
	leader->mParam2C = 8.0f;
	leader->mParam30 = 85.0f;
	leader->mParam34 = 0.001f;
}

static BOOL isMarioWaterIn()
{
	u32 flag = *gpMarioFlag;

	if ((flag & 2) || (flag & 0x10000))
		return TRUE;

	if ((*gpMarioGroundPlane)->isWaterSurface())
		return TRUE;

	if (flag & 0x20000)
		return TRUE;

	return FALSE;
}

static void setBoidLeaderMarioGoal(TBoidLeader* leader, f32 offset_y)
{
	setBoidLeaderWaitParams(leader);

	leader->mGoalTarget = (THitActor*)gpMarioAddress;
	leader->mGoalOffset.set(0.0f, offset_y, 0.0f);
}

static void setBoidLeaderHomeGoal(TBeeHive* hive)
{
	TBoidLeader* leader = hive->mBoidLeader;
	setBoidLeaderWaitParams(leader);

	leader->mGoalTarget = hive->mPosition;
	leader->mGoalOffset.set(0.0f, 0.0f, 0.0f);
}
}

DEFINE_NERVE(TNerveBeeHiveReset, TLiveActor)
{
	TBeeHive* hive = (TBeeHive*)spine->getBody();
	if (spine->getTime() == 0) {
		hive->reset();
		hive->offLiveFlag(2);
		hive->mScaling.set(0.0f, 0.0f, 0.0f);
	}

	BOOL grown = FALSE;
	hive->mScaling.x += 0.01f;
	if (hive->mScaling.x >= 1.0f) {
		hive->mScaling.set(1.0f, 1.0f, 1.0f);
		grown = TRUE;
	} else {
		hive->mScaling.y += 0.01f;
		hive->mScaling.z += 0.01f;
	}

	if (grown) {
		spine->pushAfterCurrent(&TNerveBeeHiveWait::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveBeeHiveMarioWaterIn, TLiveActor)
{
	TBeeHive* hive = (TBeeHive*)spine->getBody();
	if (spine->getTime() == 0)
		setBoidLeaderMarioGoal(hive->mBoidLeader, 500.0f);

	if (hive->getBeeParams()->mGiveupTimer.get() < spine->getTime()) {
		spine->pushAfterCurrent(&TNerveBeeHiveReset::theNerve());
		return TRUE;
	}

	if (!isMarioWaterIn()) {
		spine->pushAfterCurrent(&TNerveBeeHiveAttack::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBeeHiveAttack, TLiveActor)
{
	TBeeHive* hive = (TBeeHive*)spine->getBody();
	if (spine->getTime() == 0)
		setBoidLeaderMarioGoal(hive->mBoidLeader, 200.0f);

	if (isMarioWaterIn()) {
		spine->pushAfterCurrent(&TNerveBeeHiveMarioWaterIn::theNerve());
		return TRUE;
	}

	if (hive->mWaitTimer != 0) {
		JGeometry::TVec3<f32> diff = *gpMarioPos;
		diff -= hive->getCenterOfGravity();
		f32 giveupRange = hive->getBeeParams()->mGiveupRange.get();
		if (diff.squared() <= giveupRange * giveupRange) {
			spine->pushAfterCurrent(&TNerveBeeHiveReset::theNerve());
			return TRUE;
		}
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBeeHiveBreak, TLiveActor)
{
	TBeeHive* hive = (TBeeHive*)spine->getBody();
	if (spine->getTime() == 0) {
		hive->mMActor = hive->mMActorKeeper->getMActor("bee_nest_break.bmd");
		hive->mMActor->setBck("bee_nest_break");
		hive->onHitFlag(HIT_FLAG_NO_COLLISION);

		JGeometry::TVec3<f32> scale(2.0f, 2.0f, 2.0f);
		SMS_EasyEmitParticle<E_SMS_EFFECT_ONETIME_NORMAL>(
		    (E_SMS_EFFECT_ONETIME_NORMAL)0xe4, &hive->mPosition, hive, scale);
		SMS_EasyEmitParticle<E_SMS_EFFECT_ONETIME_NORMAL>(
		    (E_SMS_EFFECT_ONETIME_NORMAL)0xe6, &hive->mPosition, hive, scale);
		if (gpMSound->gateCheck(0x28f6))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x28f6, &hive->mPosition, 0, nullptr, 0, 4);

		for (int i = 0; i < hive->mBoidLeader->mNumActors; ++i)
			hive->appearBee(i);

		hive->mWaitTimer = hive->mBoidLeader->mNumActors;
		setBoidLeaderMarioGoal(hive->mBoidLeader, 200.0f);
	}

	if (hive->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveBeeHiveAttack::theNerve());
		hive->onLiveFlag(2);
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveBeeHiveFall, TLiveActor)
{
	TBeeHive* hive = (TBeeHive*)spine->getBody();
	if (spine->getTime() == 0) {
		hive->offLiveFlag(0x10);
		hive->onLiveFlag(LIVE_FLAG_AIRBORNE);
		if (gpMSound->gateCheck(0x28f4))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x28f4, &hive->mPosition, 0, nullptr, 0, 4);

		JGeometry::TVec3<f32> forward;
		hive->mCurrentQuat.getZDir(forward);
		hive->mAngularVelocity.y
		    = hive->getBeeParams()->mFallAngularVel.get();
		hive->mVelocity = forward;

		TMapObjBase* obj = hive->mBreakObj;
		if (obj) {
			if (obj->mActorType == 0x2000000e)
				obj = gpItemManager->makeObjAppear(0x2000000e);

			if (obj) {
				obj->appear();
				obj->JSGSetTranslation(hive->mPosition);
				obj->mVelocity.set(0.0f, 0.0f, 0.0f);
				obj->offLiveFlag(0x10);
				obj->onLiveFlag(LIVE_FLAG_AIRBORNE);
			}
			hive->mBreakObj = nullptr;
		}
	}

	hive->mAngularVelocity.x += hive->mAngularVelocity.y;

	if (!hive->checkLiveFlag(LIVE_FLAG_AIRBORNE)) {
		spine->pushAfterCurrent(&TNerveBeeHiveBreak::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveBeeHiveWait, TLiveActor)
{
	TBeeHive* hive = (TBeeHive*)spine->getBody();
	if (spine->getTime() == 0) {
		hive->onLiveFlag(0x90);
		if (hive->mWaitTimer < 3) {
			for (int i = 0; i < 3; ++i)
				hive->appearBee(i);
			hive->mWaitTimer = 3;
		}
		setBoidLeaderHomeGoal(hive);
	}

	if (hive->doWait()) {
		spine->pushAfterCurrent(&TNerveBeeHiveFall::theNerve());
		return TRUE;
	}

	if (hive->getBeeParams()->mDecrimentTimer.get() < spine->getTime()) {
		if (hive->mWaitTimer > 3) {
			hive->mWaitTimer -= 1;
			TRealoidActor* actor = hive->mActors[hive->mWaitTimer];
			if ((actor->unk74 & 2) == 0) {
				actor->unk74 |= 2;
				actor->onHitFlag(HIT_FLAG_NO_COLLISION);
			}
		}
		spine->pushAfterCurrent(this);
		return TRUE;
	}

	return FALSE;
}

TBeeHiveParams::TBeeHiveParams(const char* prm)
    : TSpineEnemyParams(prm)
    , PARAM_INIT(mGiveupTimer, 600)
    , PARAM_INIT(mGiveupRange, 1750.0f)
    , PARAM_INIT(mDecrimentTimer, 60)
    , PARAM_INIT(mRebound, 0.007f)
    , PARAM_INIT(mDecay, 0.98f)
    , PARAM_INIT(mAngleMaxAdd, 0.08f)
    , PARAM_INIT(mShakePower, 0.003f)
    , PARAM_INIT(mFallAngularVel, 0.01f)
    , PARAM_INIT(mSearchRange, 800.0f)
{
	load(mPrmPath);
}

void TBeeHiveManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "bee_body.bmd", 0x10210000, 0 },
		{ "bee_nest.bmd", 0x10210000, 0 },
		{ "bee_nest_break.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TBeeHiveManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TBeeHiveParams("/Animal/beehive.prm");
	TEnemyManager::load(stream);
}

TBeeHiveManager::TBeeHiveManager(const char* name)
    : TEnemyManager(name)
{
}

JGeometry::TVec3<f32> TBeeHive::getCenterOfGravity() const
{
	int count = mWaitTimer;
	if (count == 0)
		return JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f);

	JGeometry::TVec3<f32> center(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < count; ++i) {
		const TBoid& boid = mBoidLeader->mBoidData[i];
		center.x += boid.mPosition.x;
		center.y += boid.mPosition.y;
		center.z += boid.mPosition.z;
	}

	f32 inv = 1.0f / count;
	center.x *= inv;
	center.y *= inv;
	center.z *= inv;
	return center;
}

void TBeeHive::appearBee(int index)
{
	TBee* bee = (TBee*)mActors[index];
	if (bee->unk74 & 4)
		return;
	if ((bee->unk74 & 2) == 0)
		return;

	bee->unk74 &= ~2;
	bee->offHitFlag(HIT_FLAG_NO_COLLISION);
	mBoidLeader->mBoidData[index].mPosition = mPosition;
}

BOOL TBeeHive::doWait()
{
	TBeeHiveParams* params = getBeeParams();
	f32 oldVelocity       = mAngularVelocity.y;
	mAngularVelocity.y += mAngularVelocity.x * -params->mRebound.get();
	mAngularVelocity.y *= params->mDecay.get();
	mAngularVelocity.x += mAngularVelocity.y;

	if (mAngularVelocity.x < -mAngularVelocity.z
	    || mAngularVelocity.x > mAngularVelocity.z) {
		mAngularVelocity.z += params->mAngleMaxAdd.get();
		mAngularVelocity.z = JGeometry::TUtil<f32>::clamp(
		    mAngularVelocity.z, 0.0f, 1.5707964f);
		mAngularVelocity.x = JGeometry::TUtil<f32>::clamp(
		    mAngularVelocity.x, -mAngularVelocity.z, mAngularVelocity.z);
		mAngularVelocity.y = 0.0f;
		playBeeHiveSound(mPosition, fabsf(mAngularVelocity.x));
	}

	if (!JGeometry::TUtil<f32>::epsilonEquals(mAngularVelocity.y, 0.0f,
	                                           JGeometry::TUtil<f32>::epsilon())
	    && oldVelocity * mAngularVelocity.y <= 0.0f) {
		playBeeHiveSound(mPosition, fabsf(mAngularVelocity.x));
	}

	mCurrentQuat.slerp(mInitialQuat, 0.01f);
	mCurrentQuat.normalize();

	JGeometry::TVec3<f32> diff = *gpMarioPos;
	diff -= mPosition;

	setBoidLeaderWaitParams(mBoidLeader);
	if (diff.squared() <= params->mSearchRange.get() * params->mSearchRange.get()) {
		mBoidLeader->mGoalTarget = (THitActor*)gpMarioAddress;
		mBoidLeader->mGoalOffset.set(0.0f, 200.0f, 0.0f);
	} else {
		mBoidLeader->mGoalTarget = mPosition;
		mBoidLeader->mGoalOffset.set(0.0f, 0.0f, 0.0f);
	}

	return fabsf(mAngularVelocity.x) >= cAngleLimit;
}

void TBeeHive::calcRootMatrix()
{
	JGeometry::TQuat4<f32> base = mCenterQuat;
	JGeometry::TQuat4<f32> quat;
	quat.mul(base, mCurrentQuat);

	f32 angle = 0.5f * mAngularVelocity.x;
	JGeometry::TQuat4<f32> roll(sinf(angle), 0.0f, 0.0f, cosf(angle));
	quat.mul(quat, roll);

	TRotation3f mtx;
	mtx.setSQ(mScaling, quat);
	mtx.mMtx[0][3] = mPosition.x;
	mtx.mMtx[1][3] = mPosition.y + 120.0f;
	mtx.mMtx[2][3] = mPosition.z;

	PSMTXCopy(mtx, (MtxPtr)((u8*)getModel() + 0x20));
}

void TBeeHive::controlSound()
{
	if (mWaitTimer == 0)
		return;

	f32 x     = 0.0f;
	f32 y     = 0.0f;
	f32 z     = 0.0f;
	int count = 0;
	for (int i = 0; i < mWaitTimer; ++i) {
		TRealoidActor* actor = mActors[i];
		if ((actor->unk74 & 6) == 0) {
			x += actor->mPosition.x;
			y += actor->mPosition.y;
			z += actor->mPosition.z;
			count += 1;
		}
	}

	if (count != 0) {
		f32 inv        = 1.0f / count;
		mBeeSoundPos.x = x * inv;
		mBeeSoundPos.y = y * inv;
		mBeeSoundPos.z = z * inv;
		gpMSound->startBeeSe((Vec*)&mBeeSoundPos, count);
	}
}

void TBeeHive::controlCollision()
{
	int index             = mBreakTimer;
	TRealoidActor* actor = mActors[index];
	int count             = mWaitTimer;

	actor->checkHitActors();
	actor->onHitFlag(2);
	if (count <= ++mBreakTimer)
		mBreakTimer = 0;

	index = mBreakTimer;
	if (count <= index)
		index = 0;

	actor = mActors[index];
	if ((actor->unk74 & 6) == 0)
		actor->offHitFlag(2);
}

void TBeeHive::bind()
{
	if (checkLiveFlag(0x10))
		return;

	JGeometry::TVec3<f32> next = mPosition;
	next += mLinearVelocity;
	next += mVelocity;

	mVelocity.y -= getGravityY();
	if (mVelocity.y < TLiveActor::mVelocityMinY)
		mVelocity.y = TLiveActor::mVelocityMinY;

	f32 swingOffset = -60.0f * (-1.0f + JMACos(mAngularVelocity.x));
	mGroundHeight   = gpMap->checkGround(next.x, next.y + mHeadHeight + swingOffset,
	                                    next.z, &mGroundPlane);
	mGroundHeight += 1.0f;

	if (next.y + swingOffset <= mGroundHeight + 0.05f) {
		if (mGroundPlane->checkFlag(0x10))
			kill();

		offLiveFlag(LIVE_FLAG_AIRBORNE);
		mVelocity.set(0.0f, 0.0f, 0.0f);
		next.y = mGroundHeight - swingOffset;
	} else {
		onLiveFlag(LIVE_FLAG_AIRBORNE);
	}

	gpMap->isTouchedOneWallAndMoveXZ(&next.x, next.y + mHeadHeight, &next.z,
	                                 mBodyRadius);
	mLinearVelocity = next;
	mLinearVelocity -= mPosition;
}

#pragma dont_inline on
void TBeeHive::control()
{
	controlCollision();
	TLiveActor::control();
	controlSound();
}
#pragma dont_inline off

void TBeeHive::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TRealoid::perform(flags, graphics);
	TSpineEnemy::perform(flags, graphics);
}

BOOL TBeeHive::receiveMessage(THitActor* sender, u32 message)
{
	if (message == HIT_MESSAGE_SPRAYED_BY_WATER) {
		JGeometry::TVec3<f32> diff = mPosition;
		diff -= *gpMarioPos;

		const TNerveBase<TLiveActor>* current = mSpine->getLatestNerve();
		if (current != &TNerveBeeHiveFall::theNerve()) {
			JGeometry::TVec3<f32> dir = diff;
			dir.y = 0.0f;
			if (dir.squared() <= 0.0000038146973f) {
				dir.set(0.0f, 0.0f, 0.0f);
			} else {
				dir.normalize();
			}

			f32 sign;
			if (fabsf(mAngularVelocity.y) < 0.0001f) {
				sign = 1.0f;
			} else if (mAngularVelocity.y > 0.0f) {
				sign = 1.0f;
			} else if (mAngularVelocity.y < 0.0f) {
				sign = -1.0f;
			} else {
				sign = 0.0f;
			}

			JGeometry::TVec3<f32> up(0.0f, 0.0f, 1.0f);
			mInitialQuat.setRotate(up, dir, sign);
			mAngularVelocity.y
			    += sign * getBeeParams()->mShakePower.get();
		}

		TRotation3f mtx;
		mtx.identity33();
		mtx.mMtx[0][3] = sender->mPosition.x;
		mtx.mMtx[1][3] = sender->mPosition.y;
		mtx.mMtx[2][3] = sender->mPosition.z;
		gpMarioParticleManager->emitAndBindToMtx(0xe7, mtx, 0, nullptr);
		gpMSound->startSoundSet(0x6802, &sender->mPosition, 0, 0.0f, 0, 0,
		                         4);
		return TRUE;
	}

	if (message == HIT_MESSAGE_TRAMPLE || message == HIT_MESSAGE_HIP_DROP
	    || message == HIT_MESSAGE_PUNCH) {
		const TNerveBase<TLiveActor>* current = mSpine->getLatestNerve();
		if (current == &TNerveBeeHiveWait::theNerve()) {
			mSpine->reset();
			mSpine->setNext(&TNerveBeeHiveFall::theNerve());
		}
		return TRUE;
	}

	return FALSE;
}

TRealoidActor* TBeeHive::createRealoidActor(MActor* actor)
{
	return new TBee(actor, this);
}

void TBeeHive::load(JSUMemoryInputStream& stream)
{
	loadDefault(stream, "bee_body.bmd", 2);

	u32 firstEventID;
	u32 lastEventID;
	stream.read(&firstEventID, 4);
	stream.read(&lastEventID, 4);

	mBreakObj = TMapObjBaseManager::newAndRegisterObjByEventID(firstEventID, "");

	int count = mBoidLeader->mNumActors;
	mCoinObjs = new TMapObjBase*[count];
	TMapObjBase** coinObj = mCoinObjs;
	TMapObjBase** endObj  = coinObj + count - 1;
	while (coinObj != endObj) {
		JGeometry::TVec3<f32> pos(0.0f, 0.0f, 0.0f);
		JGeometry::TVec3<f32> rot(0.0f, 0.0f, 0.0f);
		JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
		*coinObj = TMapObjBaseManager::newAndRegisterObj("coin", pos, rot,
		                                                 scale);
		coinObj += 1;
	}

	*coinObj = TMapObjBaseManager::newAndRegisterObjByEventID(lastEventID, "");
	if (!*coinObj) {
		JGeometry::TVec3<f32> pos(0.0f, 0.0f, 0.0f);
		JGeometry::TVec3<f32> rot(0.0f, 0.0f, 0.0f);
		JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
		*coinObj = TMapObjBaseManager::newAndRegisterObj("coin", pos, rot,
		                                                 scale);
	}

	mMActorKeeper->createMActor("bee_nest_break.bmd", 3);
	mMActor = mMActorKeeper->createMActor("bee_nest.bmd", 3);

	for (int i = 0; i < count; ++i) {
		TRealoidActor* actor = mActors[i];
		actor->init();
		actor->unk74 |= 2;
	}

	reset();
}

#pragma dont_inline on
void TBeeHive::receiveMessageFromChild(TBee* bee)
{
	if (bee->unk74 & 4)
		return;

	bee->unk74 |= 4;
	bee->unk64 |= HIT_FLAG_NO_COLLISION;

	int coinIndex    = mCoinIndex;
	TMapObjBase* obj = mCoinObjs[coinIndex];
	if (coinIndex != mBoidLeader->mNumActors - 1)
		obj = gpItemManager->makeObjAppear(0x2000000e);

	if (obj) {
		obj->appear();
		*(Vec*)&obj->mPosition = *(Vec*)&bee->mPosition;
		obj->mVelocity.x       = 0.0f;
		obj->mVelocity.y       = 15.0f;
		obj->mVelocity.z       = 0.0f;
		obj->mLiveFlag &= ~0x10;
	}

	mCoinIndex += 1;
}
#pragma dont_inline off

void TBeeHive::reset()
{
	mPosition = mInitialPosition;

	TRotation3f mtx;
	MsMtxSetRotRPH(mtx, mRotation.x, 0.0f, mRotation.z);
	mtx.getQuat(mCenterQuat);

	f32 half = mRotation.y * 0.5f;
	f32 sinHalf = sinf(half);
	f32 cosHalf = cosf(half);
	mCurrentQuat.set(0.0f, sinHalf, 0.0f, cosHalf);
	mInitialQuat = mCurrentQuat;

	mAngularVelocity.set(0.0f, 0.0f, 0.015707964f);
	mBreakTimer = 0;
	onLiveFlag(0x90);
	mMActor = mMActorKeeper->getMActor("bee_nest.bmd");
	offHitFlag(HIT_FLAG_NO_COLLISION);
}

void TBeeHive::init(TLiveManager* manager)
{
	mManager = manager;
	mManager->manageActor(this);
	onLiveFlag(0x18);
	mSpine->initWith(&TNerveBeeHiveWait::theNerve());

	mInitialPosition = mPosition;
	mWaitTimer       = 0;
	mCoinIndex       = 0;
	initHitActor(0x10000031, 0, 0x80000000, 50.0f, 50.0f, 100.0f,
	             100.0f);
	onHitFlag(2);
}

TBeeHive::TBeeHive(const char* name)
    : TRealoid(name)
    , mBreakObj(nullptr)
{
}

BOOL TBee::receiveMessage(THitActor* sender, u32 message)
{
	switch (message) {
	case 4:
		if (!mHolder) {
			mHolder = (TTakeActor*)sender;
			SMS_EasyEmitParticle<E_SMS_EFFECT_ONETIME_NORMAL>(
			    (E_SMS_EFFECT_ONETIME_NORMAL)0xe7, &mPosition, nullptr,
			    JGeometry::TVec3<f32>(1.0f, 1.0f, 1.0f));
			return TRUE;
		}
		break;
	case 8:
		if (mHolder) {
			mHolder = nullptr;
			return TRUE;
		}
		break;
	case 11:
		mOwner->receiveMessageFromChild(this);
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

void TBee::init()
{
	initHitActor(0x1000002f, 1, 0x80000000, 20.0f, 20.0f, 50.0f,
	             50.0f);

	JDrama::TNameRef* root = JDrama::TNameRefGen::instance->mRootNameRef;
	const char* groupName  = "\x93\x47\x83\x4F\x83\x8B\x81\x5B\x83\x76";
	JDrama::TNameRef* group
	    = root->searchF(JDrama::TNameRef::calcKeyCode(groupName), groupName);
	JGadget::TList_pointer_void* list
	    = (JGadget::TList_pointer_void*)((u8*)group + 0x10);
	JGadget::TList_pointer_void::iterator iter = list->end();
	list->insert(iter, (void*)this);

	onHitFlag(HIT_FLAG_NO_COLLISION);
}
