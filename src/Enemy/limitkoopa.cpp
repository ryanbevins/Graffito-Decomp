#include <Enemy/LimitKoopa.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Strategic/ObjManager.hpp>
#include <System/Particles.hpp>
#include <JSystem/JDrama/JDRNameRef.hpp>

// rogue includes needed for matching sinit & rodata
#include <M3DUtil/InfectiousStrings.hpp>

TLimitKoopaManager::TLimitKoopaManager(const char* name)
    : TEnemyManager(name)
{
}

TSpineEnemy* TLimitKoopaManager::createEnemyInstance() { return nullptr; }

void TLimitKoopaManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "koopa_model.bmd", 0x14240000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

inline TLimitKoopaParams::TLimitKoopaParams(const char* path)
    : TSpineEnemyParams(path)
    , PARAM_INIT(mRotationSpeed, 1.0f)
    , PARAM_INIT(mBodyScale, 1.0f)
    , PARAM_INIT(mHipDropInitialSpeedY, 1.0f)
    , PARAM_INIT(mHipDropGravityY, 1.0f)
    , PARAM_INIT(mTurnSpeed, 1.6f)
    , PARAM_INIT(mTurnAnim, 3.7f)
    , PARAM_INIT(mWaitStep, 600.0f)
    , PARAM_INIT(mAttackRadius, 800.0f)
    , PARAM_INIT(mAttackHeight, 1000.0f)
    , PARAM_INIT(mFocusRange, 2.0f)
    , PARAM_INIT(mWaitRange, 12.0f)
    , PARAM_INIT(mFireSpeed, 4.0f)
    , PARAM_INIT(mTumbleSpeed, 2.0f)
    , PARAM_INIT(mWaitSpeed, 2.0f)
    , PARAM_INIT(mStaggerSpeed, 2.0f)
    , PARAM_INIT(mDownSpeed, 1.8f)
    , PARAM_INIT(mTumbleWeight, 4.2f)
    , PARAM_INIT(mFlameScale, 1.0f)
    , PARAM_INIT(mFlameCount, 300)
    , PARAM_INIT(mFlameFocusStartStep, 100)
    , PARAM_INIT(mFlameFocusEndStep, 300)
    , PARAM_INIT(mFlameRadius, 200.0f)
    , PARAM_INIT(mFlameHeight, 600.0f)
    , PARAM_INIT(mHeadRadius, 400.0f)
    , PARAM_INIT(mWaterhitSpeed, 2.0f)
    , PARAM_INIT(mFlameOverStart, 0.9f)
    , PARAM_INIT(mFlameNeckRange, 16.0f)
    , PARAM_INIT(mFlameNeckDownRate, 0.3f)
    , PARAM_INIT(mMarioEstimationFire, 20.0f)
    , PARAM_INIT(mMarioEstimationWait, 10.0f)
{
	TParams::load(mPrmPath);

	mRotationSpeed.set(0.25f);
	mBodyScale.set(0.7f);
	mHipDropInitialSpeedY.set(40.0f);
	mHipDropGravityY.set(0.4f);
}

void TLimitKoopaManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk38 = new TLimitKoopaParams("/enemy/limitkoopa.prm");
}

void TLimitKoopaManager::loadAfter()
{
	JDrama::TNameRef::loadAfter();
	SMS_LoadParticle("/scene/koopa/jpa/ms_kp_fire_a.jpa", 0x1c0);
	SMS_LoadParticle("/scene/koopa/jpa/ms_kp_fire_b.jpa", 0x1c1);
	SMS_LoadParticle("/scene/koopa/jpa/ms_kp_fire_c.jpa", 0x1c2);
	SMS_LoadParticle("/scene/koopa/jpa/ms_kp_fire_d.jpa", 0x1c3);
	SMS_LoadParticle("/scene/koopa/jpa/ms_kp_fire_e.jpa", 0x1f3);
}
