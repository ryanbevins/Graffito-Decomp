#ifndef ENEMY_FLY_ENEMY_HPP
#define ENEMY_FLY_ENEMY_HPP

#include <Enemy/WalkerEnemy.hpp>
#include <Strategic/Nerve.hpp>

class TFlyEnemyParams : public TWalkerEnemyParams {
public:
	TFlyEnemyParams(const char*);

	/* 0x32C */ TParamRT<f32> mSLNormalFlyGravityY;
	/* 0x340 */ TParamRT<f32> mSLNormalFlySpeed;
	/* 0x354 */ TParamRT<f32> mSLChaseFlyGravityY;
	/* 0x368 */ TParamRT<f32> mSLChaseDist;
	/* 0x37C */ TParamRT<f32> mSLForceGravityY;
};

class TFlyEnemy : public TWalkerEnemy {
public:
	TFlyEnemy(const char* name);

	// overrides of inherited slots
	virtual void init(TLiveManager*);
	virtual void bind();
	virtual f32 getGravityY() const;
	virtual void reset();
	virtual void setAfterDeadEffect();

	// new slots (appended after TWalkerEnemy::initialGraphNode)
	virtual void flyBehavior();
	virtual void setChaseFlyAnm();
	virtual void setNormalFlyAnm();

	// non-virtual
	void fly();
	void calcChaseParam();

	// fabricated
	TFlyEnemyParams* getSaveParam2() const
	{
		return (TFlyEnemyParams*)getSaveParam();
	}

	static f32 mTestSp;
	static s32 mInvalidTime;
	static f32 mTestMarioSpMax;

	/* 0x194 */ f32 mCurGravityY;
	/* 0x198 */ s32 mFlyState;
	/* 0x19C */ TFlyEnemyParams* mFlyParams;
	/* 0x1A0 */ s32 mFlyTimer;
	/* 0x1A4 */ u8 mChaseFinished;
	/* 0x1A5 */ u8 mIsChaseMode;
	/* 0x1A6 */ u8 mColorVariant;
};

class TLiveActor;

DECLARE_NERVE(TNerveFlyEnemyChaseFly, TLiveActor);
DECLARE_NERVE(TNerveFlyEnemyNormalFly, TLiveActor);

#endif
