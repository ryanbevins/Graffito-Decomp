#ifndef ENEMY_LIMIT_KOOPA_HPP
#define ENEMY_LIMIT_KOOPA_HPP

#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Strategic/Nerve.hpp>

class TLimitKoopaParams : public TSpineEnemyParams {
public:
	TLimitKoopaParams(const char*);

	/* 0xA8 */ TParamRT<f32> mRotationSpeed;
	/* 0xBC */ TParamRT<f32> mBodyScale;
	/* 0xD0 */ TParamRT<f32> mHipDropInitialSpeedY;
	/* 0xE4 */ TParamRT<f32> mHipDropGravityY;
	/* 0xF8 */ TParamRT<f32> mTurnSpeed;
	/* 0x10C */ TParamRT<f32> mTurnAnim;
	/* 0x120 */ TParamRT<f32> mWaitStep;
	/* 0x134 */ TParamRT<f32> mAttackRadius;
	/* 0x148 */ TParamRT<f32> mAttackHeight;
	/* 0x15C */ TParamRT<f32> mFocusRange;
	/* 0x170 */ TParamRT<f32> mWaitRange;
	/* 0x184 */ TParamRT<f32> mFireSpeed;
	/* 0x198 */ TParamRT<f32> mTumbleSpeed;
	/* 0x1AC */ TParamRT<f32> mWaitSpeed;
	/* 0x1C0 */ TParamRT<f32> mStaggerSpeed;
	/* 0x1D4 */ TParamRT<f32> mDownSpeed;
	/* 0x1E8 */ TParamRT<f32> mTumbleWeight;
	/* 0x1FC */ TParamRT<f32> mFlameScale;
	/* 0x210 */ TParamRT<s32> mFlameCount;
	/* 0x224 */ TParamRT<s32> mFlameFocusStartStep;
	/* 0x238 */ TParamRT<s32> mFlameFocusEndStep;
	/* 0x24C */ TParamRT<f32> mFlameRadius;
	/* 0x260 */ TParamRT<f32> mFlameHeight;
	/* 0x274 */ TParamRT<f32> mHeadRadius;
	/* 0x288 */ TParamRT<f32> mWaterhitSpeed;
	/* 0x29C */ TParamRT<f32> mFlameOverStart;
	/* 0x2B0 */ TParamRT<f32> mFlameNeckRange;
	/* 0x2C4 */ TParamRT<f32> mFlameNeckDownRate;
	/* 0x2D8 */ TParamRT<f32> mMarioEstimationFire;
	/* 0x2EC */ TParamRT<f32> mMarioEstimationWait;
};

class TLimitKoopaManager : public TEnemyManager {
public:
	TLimitKoopaManager(const char* = "\x83\x4A\x83\x5A\x83\x4E\x83\x93\x83\x7D\x83\x6C\x81\x5B\x83\x57\x83\x83\x81\x5B");

	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual void createModelData();
	virtual TSpineEnemy* createEnemyInstance();
};

#endif
