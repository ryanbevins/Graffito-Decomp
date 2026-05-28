#ifndef ENEMY_LIMIT_KOOPA_JR_HPP
#define ENEMY_LIMIT_KOOPA_JR_HPP

#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Enemy/DirectionCalc.hpp>
#include <Strategic/Nerve.hpp>

class TLimitKoopaJrParams : public TSpineEnemyParams {
public:
	TLimitKoopaJrParams(const char*);

	/* 0xA8 */ TParamRT<f32> mSLAcceleration;
	/* 0xBC */ TParamRT<f32> mSLRotationSpeed;
	/* 0xD0 */ TParamRT<f32> mSLSpeedMax;
	/* 0xE4 */ TParamRT<f32> mSLRoundAngleVelocity;
	/* 0xF8 */ TParamRT<f32> mSLRoundRadius;
	/* 0x10C */ TParamRT<f32> mSLRoundHeight;
	/* 0x120 */ TParamRT<f32> mSLDamageRadius;
	/* 0x134 */ TParamRT<f32> mSLDamageHeight;
	/* 0x148 */ TParamRT<f32> mSLKoopaJrScale;
	/* 0x15C */ TParamRT<s32> mSLShotDoodlePeriod;
	/* 0x170 */ TParamRT<s32> mSLDamagePeriod;
};

class TLimitKoopaJr : public TSpineEnemy {
public:
	TLimitKoopaJr(const char* = "リミットクッパジュニア");

	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void bind();
	virtual void perform(u32, JDrama::TGraphics*);
	virtual const char** getBasNameTable() const;
	virtual void reset();

	void moveRun();

	// fabricated
	TLimitKoopaJrParams* getSaveParam2() const
	{
		return (TLimitKoopaJrParams*)getSaveParam();
	}

public:
	/* 0x150 */ TLiveActor* mTargetActor;
	/* 0x154 */ TLiveActor** mKoopaMgrRef;
	/* 0x158 */ s32 mDamageTimer;
	/* 0x15C */ s32 mShotDoodleTimer;
	/* 0x160 */ JGeometry::TVec3<f32> mShotVelocity;
	/* 0x16C */ f32 unk16C;
	/* 0x170 */ TDirectionCalc mDirection1;
	/* 0x178 */ TDirectionCalc mDirection2;
};

class TLimitKoopaJrManager : public TEnemyManager {
public:
	TLimitKoopaJrManager(const char* = "リミットクッパジュニアマネージャー");

	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual void createModelData();
	virtual TSpineEnemy* createEnemyInstance();
};

DECLARE_NERVE(TNerveLimitKoopaJrRun, TLiveActor);
DECLARE_NERVE(TNerveLimitKoopaJrWait, TLiveActor);
DECLARE_NERVE(TNerveLimitKoopaJrLaunch, TLiveActor);
DECLARE_NERVE(TNerveLimitKoopaJrYahoo, TLiveActor);

#endif
