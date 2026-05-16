#include <Animal/Bird.hpp>
#include <Animal/AnimalSave.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>
#include <MSound/MSoundSE.hpp>
#include <Player/MarioAccess.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <Enemy/WireBinder.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <MSound/MSound.hpp>
#include <System/Particles.hpp>

// ---- TAnimalBirdParams ----

TAnimalBirdParams::TAnimalBirdParams(const char* prm)
    : TSpineEnemyParams(prm)
    , PARAM_INIT(mMarchSpeed, 5.0f)
    , PARAM_INIT(mTurnSpeed, 0.1f)
    , PARAM_INIT(mReturnTimer, 1800)
    , PARAM_INIT(mSearchLength, 800.0f)
    , PARAM_INIT(mSearchHeight, 600.0f)
    , PARAM_INIT(mSearchAware, 400.0f)
    , PARAM_INIT(mSearchAngle, 90.0f)
    , PARAM_INIT(mActionTimer, 100)
    , PARAM_INIT(mWaterproofTimerMax, 45)
    , PARAM_INIT(mFloatingTimerMax, 30)
    , PARAM_INIT(mLandingGravityY, 1.0f)
    , PARAM_INIT(mLandingTorqueY, 2.0f)
    , PARAM_INIT(mWalkingTorqueY, 2.0f)
    , PARAM_INIT(mWalkingSpeed, 0.5f)
    , PARAM_INIT(mWalkTimer, 100)
    , PARAM_INIT(mLandingFric, 0.95f)
    , PARAM_INIT(mActionTimerAdd, 300)
    , PARAM_INIT(mWaterPowerY, 15.0f)
{
	load(mPrmPath);
}

// ---- TAnimalBirdManager ----

TAnimalBirdManager::TAnimalBirdManager(const char* name)
    : TEnemyManager(name)
{
}

void TAnimalBirdManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "bird_man.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TAnimalBirdManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TAnimalBirdParams("/Animal/bird.prm");
	TEnemyManager::load(stream);
}

void TAnimalBirdManager::loadAfter()
{
	JDrama::TNameRef::loadAfter();
	MSoundSESystem::MSRandPlay::createRandPlayVec(0x3869, (u16)mObjNum);
	MSoundSESystem::MSRandPlay::createRandPlayVec(0x3870, (u16)mObjNum);
}

// ---- TAnimalBird ----

TAnimalBird::TAnimalBird(const char* name)
    : TSpineEnemy(name)
{
	unk150   = 0;
	mBinder2 = (TBinder*)NULL;
}

void TAnimalBird::load(JSUMemoryInputStream& stream)
{
	TSpineEnemy::load(stream);
}

void TAnimalBird::loadAfter()
{
	JDrama::TNameRef::loadAfter();
	MSoundSESystem::MSRandPlay::registerTrans(0x3869, &mPosition);
	MSoundSESystem::MSRandPlay::registerTrans(0x3870, &mPosition);
}

BOOL TAnimalBird::receiveMessage(THitActor* sender, u32 msg)
{
	if (mLiveFlag & 1)
		return FALSE;

	if (msg == 0xF) {
		JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
		SMS_EasyEmitParticle((E_SMS_EFFECT_ONETIME_NORMAL)0xE7,
		                     &sender->mPosition, (const void*)NULL, scale);
		gpMSound->startSoundSet(0x6802, &sender->mPosition, 0, 0.0f, 0, 0, 4);

		if (unk178 <= 0) {
			TAnimalBirdParams* p = (TAnimalBirdParams*)getSaveParam();
			unk178               = p->mWaterproofTimerMax.value;
			if (mLiveFlag & 0x80 && mHitPoints > 0) {
				mHitPoints--;
			}
		}
		return TRUE;
	}

	if (msg == 4) {
		if (*(THitActor**)((char*)this + 0x68) == NULL) {
			unk64 |= 1;
			*(THitActor**)((char*)this + 0x68) = sender;
			JGeometry::TVec3<f32> scale2(1.0f, 1.0f, 1.0f);
			SMS_EasyEmitParticle((E_SMS_EFFECT_ONETIME_NORMAL)0xE7,
			                     &sender->mPosition, (const void*)NULL, scale2);
		}
		return TRUE;
	}

	if (msg == 6 || msg == 7) {
		if (*(THitActor**)((char*)this + 0x68) == sender) {
			*(THitActor**)((char*)this + 0x68) = (THitActor*)NULL;
			unk64 &= ~1;
		}
		return TRUE;
	}

	if (msg == 0xB) {
		*(THitActor**)((char*)this + 0x68) = (THitActor*)NULL;
		if (mSpine->getLatestNerve() != &TNerveAnimalBirdChangeToCoin::theNerve()) {
			mSpine->setNext(&TNerveAnimalBirdChangeToCoin::theNerve());
		} else {
			// vt+0xE4 (kill?)
			// no need to actually call
		}
		return TRUE;
	}

	if (msg == 0) {
		u32 t = sender->mActorType - 0x10000000;
		if (t == 0xD) {
			if (mSpine->getLatestNerve()
			    != &TNerveAnimalBirdChangeToCoin::theNerve()) {
				mSpine->setNext(&TNerveAnimalBirdChangeToCoin::theNerve());
			} else {
				receiveMessage(this, 0xF);
			}
			return TRUE;
		}
	}

	return TSpineEnemy::receiveMessage(sender, msg);
}

void TAnimalBird::init(TLiveManager* mgr)
{
	mManager = mgr;
	mgr->manageActor(this);

	mMActorKeeper = new TMActorKeeper(mgr, (u16)1);
	mMActor       = mMActorKeeper->createMActor("bird_man.bmd", 0);

	mSpine->initWith(&TNerveAnimalBirdWaitOnGround::theNerve());

	initParams();
	initHitActor(0x10000032, 0, 0, 50.0f, 50.0f, 70.0f, 80.0f);

	onHitFlag(2);
	offHitFlag(1);

	mScaledBodyRadius = 35.0f;

	initAnmSound();
}

// Class with virtual MtxPtr-returning method at vt+0xA4 (mount object)
class TBirdMount {
public:
	virtual MtxPtr getRiderMtx() = 0;
};

void TAnimalBird::calcRootMatrix()
{
	TBirdMount* mount = *(TBirdMount**)((char*)this + 0x68);
	if (mount != NULL) {
		// vt+0xA4 = getRiderMtx
		typedef MtxPtr (*MtxFn)(TBirdMount*);
		MtxFn fn = *(MtxFn*)(*(char**)mount + 0xA4);
		MtxPtr m = fn(mount);
		PSMTXCopy(m, getModel()->unk20);
	} else {
		TSpineEnemy::calcRootMatrix();
	}
	getModel()->unk20[1][3] += 35.0f;
}

void TAnimalBird::bind()
{
	BOOL useWire = FALSE;
	if (mBinder2 != NULL) {
		BOOL cond1 = TRUE;
		BOOL cond2 = TRUE;
		BOOL cond3 = TRUE;
		TSpineBase<TLiveActor>* sp = mSpine;
		if (sp->getLatestNerve() != &TNerveAnimalBirdWaitOnGround::theNerve()) {
			if (sp->getLatestNerve()
			    != &TNerveAnimalBirdActionOnGround::theNerve())
				cond3 = FALSE;
		}
		if (!cond3) {
			if (sp->getLatestNerve()
			    != &TNerveAnimalBirdWalkOnGround::theNerve())
				cond2 = FALSE;
		}
		if (!cond2) {
			if (mSpine->getLatestNerve()
			    != &TNerveAnimalBirdPreLanding::theNerve())
				cond1 = FALSE;
		}
		if (cond1)
			useWire = TRUE;
	}

	if (!useWire) {
		TLiveActor::bind();
	} else {
		mBinder2->bind(this);
	}
}

void TAnimalBird::moveObject() { }

const char** TAnimalBird::getBasNameTable() const
{
	static const char* bastable[] = {
		"/scene/bird/bas/bird_fly.bas",
		"/scene/bird/bas/bird_open.bas",
		"/scene/bird/bas/bird_start.bas",
		"/scene/bird/bas/bird_stop.bas",
		nullptr,
	};
	return bastable;
}

void TAnimalBird::initParams()
{
	unk158 = mPosition;
	unk158.y += 90.0f;
	unk164 = mRotation;

	TSpineEnemyParams* p = getSaveParam();
	u8 v;
	if (p != NULL) {
		v = (u8)getSaveParam()->mSLHitPointMax.value;
	} else {
		v = 1;
	}
	mHitPoints = v;

	unk178 = 0;
	unk17C = 0;
	unk170 = 1.0f;

	mLiveFlag &= ~0x180;

	unk174 = 1.0f - 0.1f * (MsRandF() - 0.5f);

	if (TWireBinder::isOnWire(mPosition)) {
		TWireBinder* wb = new TWireBinder();
		mBinder2        = wb;
		wb->init(mPosition);
	}
}

BOOL TAnimalBird::isFindMario() const
{
	f32 diffY = mPosition.y - gpMarioPos->y;
	if (diffY < 0)
		diffY = -diffY;
	TAnimalBirdParams* p = (TAnimalBirdParams*)getSaveParam();
	if (p->mSearchHeight.value < diffY)
		return FALSE;

	f32 scale = unk174;
	return isInSight(*gpMarioPos, scale * p->mSearchLength.value,
	                 scale * p->mSearchAngle.value,
	                 scale * p->mSearchAware.value)
	    ? 1
	    : 0;
}

void TAnimalBird::doFlyToCurPathNode() { }

void TAnimalBird::doLanding(bool) { }

DEFINE_NERVE(TNerveAnimalBirdLanding, TLiveActor) { return FALSE; }
DEFINE_NERVE(TNerveAnimalBirdPreLanding, TLiveActor) { return FALSE; }
DEFINE_NERVE(TNerveAnimalBirdComeback, TLiveActor) { return FALSE; }
DEFINE_NERVE(TNerveAnimalBirdChangeToCoin, TLiveActor) { return FALSE; }
DEFINE_NERVE(TNerveAnimalBirdGraphWander, TLiveActor) { return FALSE; }
DEFINE_NERVE(TNerveAnimalBirdTakeoff, TLiveActor) { return FALSE; }
DEFINE_NERVE(TNerveAnimalBirdWalkOnGround, TLiveActor) { return FALSE; }
DEFINE_NERVE(TNerveAnimalBirdActionOnGround, TLiveActor) { return FALSE; }
DEFINE_NERVE(TNerveAnimalBirdWaitOnGround, TLiveActor) { return FALSE; }
