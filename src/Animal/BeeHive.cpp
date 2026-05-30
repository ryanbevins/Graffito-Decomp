#include <Animal/BeeHive.hpp>
#include <Animal/BoidLeader.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <M3DUtil/MActor.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <MSound/MSSetSound.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
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
}

DEFINE_NERVE(TNerveBeeHiveReset, TLiveActor)
{
	TBeeHive* hive = (TBeeHive*)spine->getBody();
	if (spine->getTime() == 0)
		hive->reset();

	if (spine->getTime() > 30) {
		spine->pushAfterCurrent(&TNerveBeeHiveWait::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveBeeHiveMarioWaterIn, TLiveActor)
{
	TBeeHive* hive = (TBeeHive*)spine->getBody();
	if (spine->getTime() == 0) {
		hive->mWaitTimer = hive->getBeeParams()->mGiveupTimer.get();
		hive->appearBee(0);
	}

	hive->doWait();
	if (hive->mWaitTimer <= 0) {
		spine->pushAfterCurrent(&TNerveBeeHiveWait::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveBeeHiveAttack, TLiveActor)
{
	TBeeHive* hive = (TBeeHive*)spine->getBody();
	if (spine->getTime() == 0)
		hive->appearBee(0);

	hive->doWait();
	if (hive->mWaitTimer <= 0) {
		spine->pushAfterCurrent(&TNerveBeeHiveWait::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveBeeHiveBreak, TLiveActor)
{
	TBeeHive* hive = (TBeeHive*)spine->getBody();
	if (spine->getTime() == 0) {
		hive->appearBee(0);
		hive->mBreakTimer = 0;
	}

	hive->doWait();
	if (hive->mBreakTimer > 60)
		return TRUE;
	hive->mBreakTimer += 1;
	return FALSE;
}

DEFINE_NERVE(TNerveBeeHiveFall, TLiveActor)
{
	TBeeHive* hive = (TBeeHive*)spine->getBody();
	hive->mAngularVelocity.y += hive->getBeeParams()->mFallAngularVel.get();
	hive->mRotation.y += hive->mAngularVelocity.y;
	hive->bind();

	if (hive->mPosition.y <= hive->mInitialPosition.y - 60.0f) {
		spine->pushAfterCurrent(&TNerveBeeHiveBreak::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveBeeHiveWait, TLiveActor)
{
	TBeeHive* hive = (TBeeHive*)spine->getBody();
	if (hive->doWait()) {
		spine->pushAfterCurrent(&TNerveBeeHiveFall::theNerve());
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
	JGeometry::TVec3<f32> center = mPosition;
	center.x += mCenterDir.x * mCenterRadius;
	center.y += mCenterDir.y * mCenterRadius;
	center.z += mCenterDir.z * mCenterRadius;
	return center;
}

void TBeeHive::appearBee(int count)
{
	int max = 0;
	if (mBoidLeader)
		max = mBoidLeader->mNumActors;

	for (int i = 0; i < max && i < count; ++i) {
		TBee* bee = (TBee*)mActors[i];
		if (bee)
			bee->offHitFlag(HIT_FLAG_NO_COLLISION);
	}
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
		mAngularVelocity.z += params->mShakePower.get();
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
		mBoidLeader->mGoalActor = (THitActor*)gpMarioAddress;
		if (gpMarioAddress)
			mBoidLeader->mGoalPos = ((THitActor*)gpMarioAddress)->mPosition;
		else
			mBoidLeader->mGoalPos.set(0.0f, 0.0f, 0.0f);
		mBoidLeader->mGoalOffset.set(0.0f, 200.0f, 0.0f);
	} else {
		mBoidLeader->mGoalActor = nullptr;
		mBoidLeader->mGoalPos   = mPosition;
		mBoidLeader->mGoalOffset.set(0.0f, 0.0f, 0.0f);
	}

	return fabsf(mAngularVelocity.x) >= cAngleLimit;
}

void TBeeHive::calcRootMatrix()
{
	TLiveActor::calcRootMatrix();
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

	mBreakTimer += 1;
	if (count <= mBreakTimer)
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
	mVelocity.y -= 1.0f;
	mPosition.x += mVelocity.x;
	mPosition.y += mVelocity.y;
	mPosition.z += mVelocity.z;
}

void TBeeHive::control()
{
	controlCollision();
	TLiveActor::control();
	controlSound();
}

void TBeeHive::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TSpineEnemy::perform(flags, graphics);
	if (flags & 2) {
		controlSound();
		controlCollision();
	}
}

BOOL TBeeHive::receiveMessage(THitActor* sender, u32 message)
{
	if (message == 0xf || message == 0x10) {
		mSpine->pushAfterCurrent(&TNerveBeeHiveFall::theNerve());
		return TRUE;
	}

	if (message == 8) {
		mSpine->pushAfterCurrent(&TNerveBeeHiveMarioWaterIn::theNerve());
		return TRUE;
	}

	return TSpineEnemy::receiveMessage(sender, message);
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

	int count = mBoidLeader ? mBoidLeader->mNumActors : 0;
	mCoinObjs = new TMapObjBase*[count];
	for (int i = 0; i < count - 1; ++i) {
		JGeometry::TVec3<f32> pos(0.0f, 0.0f, 0.0f);
		JGeometry::TVec3<f32> rot(0.0f, 0.0f, 0.0f);
		JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
		mCoinObjs[i] = TMapObjBaseManager::newAndRegisterObj("coin", pos, rot,
		                                                      scale);
	}

	if (count > 0) {
		mCoinObjs[count - 1]
		    = TMapObjBaseManager::newAndRegisterObjByEventID(lastEventID, "");
		if (!mCoinObjs[count - 1]) {
			JGeometry::TVec3<f32> pos(0.0f, 0.0f, 0.0f);
			JGeometry::TVec3<f32> rot(0.0f, 0.0f, 0.0f);
			JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
			mCoinObjs[count - 1]
			    = TMapObjBaseManager::newAndRegisterObj("coin", pos, rot,
			                                            scale);
		}
	}

	if (mMActorKeeper) {
		mMActorKeeper->createMActor("bee_nest_break.bmd", 3);
		mMActor = mMActorKeeper->createMActor("bee_nest.bmd", 3);
	}

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

	Mtx mtx;
	MsMtxSetRotRPH(mtx, mRotation.x, 0.0f, mRotation.z);

	f32 xx    = mtx[0][0];
	f32 yy    = mtx[1][1];
	f32 zz    = mtx[2][2];
	f32 trace = xx + yy + zz;
	if (trace >= 0.0f) {
		f32 scale    = JGeometry::TUtil<f32>::sqrt(trace + 1.0f);
		mCenterRadius = 0.5f * scale;
		f32 inv      = 0.5f / scale;
		mCenterDir.x = (mtx[2][1] - mtx[1][2]) * inv;
		mCenterDir.y = (mtx[0][2] - mtx[2][0]) * inv;
		mCenterDir.z = (mtx[1][0] - mtx[0][1]) * inv;
	} else {
		f32 maxDiag = xx;
		if (maxDiag < yy)
			maxDiag = yy;
		if (maxDiag < zz)
			maxDiag = zz;

		if (maxDiag == xx) {
			f32 scale = JGeometry::TUtil<f32>::sqrt(xx - (yy + zz) + 1.0f);
			mCenterDir.x = 0.5f * scale;
			f32 inv      = 0.5f / scale;
			mCenterDir.y = (mtx[0][1] + mtx[1][0]) * inv;
			mCenterDir.z = (mtx[2][0] + mtx[0][2]) * inv;
			mCenterRadius = (mtx[2][1] - mtx[1][2]) * inv;
		} else if (maxDiag == yy) {
			f32 scale = JGeometry::TUtil<f32>::sqrt(yy - (zz + xx) + 1.0f);
			mCenterDir.y = 0.5f * scale;
			f32 inv      = 0.5f / scale;
			mCenterDir.z = (mtx[1][2] + mtx[2][1]) * inv;
			mCenterDir.x = (mtx[0][1] + mtx[1][0]) * inv;
			mCenterRadius = (mtx[0][2] - mtx[2][0]) * inv;
		} else {
			f32 scale = JGeometry::TUtil<f32>::sqrt(zz - (xx + yy) + 1.0f);
			mCenterDir.z = 0.5f * scale;
			f32 inv      = 0.5f / scale;
			mCenterDir.x = (mtx[2][0] + mtx[0][2]) * inv;
			mCenterDir.y = (mtx[1][2] + mtx[2][1]) * inv;
			mCenterRadius = (mtx[1][0] - mtx[0][1]) * inv;
		}
	}

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
			{
				JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
				SMS_EasyEmitParticle<E_SMS_EFFECT_ONETIME_NORMAL>(
				    (E_SMS_EFFECT_ONETIME_NORMAL)0xe7, &mPosition, nullptr,
				    scale);
			}
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
	             100.0f);
	onHitFlag(HIT_FLAG_NO_COLLISION);
}
