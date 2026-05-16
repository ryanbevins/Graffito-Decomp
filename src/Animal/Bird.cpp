#include <Animal/Bird.hpp>
#include <Animal/AnimalSave.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>

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

void TAnimalBirdManager::loadAfter() { JDrama::TNameRef::loadAfter(); }

// ---- TAnimalBird ----

TAnimalBird::TAnimalBird(const char* name)
    : TSpineEnemy(name)
{
	unk150 = 0;
	unk154 = 0;
}

void TAnimalBird::load(JSUMemoryInputStream& stream)
{
	TSpineEnemy::load(stream);
}

void TAnimalBird::loadAfter() { JDrama::TNameRef::loadAfter(); }

BOOL TAnimalBird::receiveMessage(THitActor*, u32) { return FALSE; }

void TAnimalBird::init(TLiveManager* mgr) { TSpineEnemy::init(mgr); }

void TAnimalBird::calcRootMatrix() { }

void TAnimalBird::bind() { }

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

void TAnimalBird::initParams() { }

BOOL TAnimalBird::isFindMario() const { return FALSE; }

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
