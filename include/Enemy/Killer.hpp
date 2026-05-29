#ifndef ENEMY_KILLER_HPP
#define ENEMY_KILLER_HPP

#include <Enemy/FlyEnemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Strategic/Nerve.hpp>

class TKillerParams : public TFlyEnemyParams {
public:
	TKillerParams(const char* path = "/enemy/killer.prm");

	/* 0x390 */ TParamRT<f32> mSLWaterAddGravityY;
	/* 0x3A4 */ TParamRT<s32> mSLChaseTimer;
	/* 0x3B8 */ TParamRT<f32> mSLBombRange;
};

class TKiller : public TFlyEnemy {
public:
	TKiller(const char* name = "\x83\x4C\x83\x89\x81\x5B");

	// overrides (declared in vtable-slot order)
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void bind();
	virtual const char** getBasNameTable() const;
	virtual void reset();
	virtual void genEventCoin();
	virtual void behaveToWater(THitActor*);
	virtual void changeOut();
	virtual void setDeadAnm();
	virtual void attackToMario();
	virtual void forceKill();
	virtual void setMActorAndKeeper();
	virtual bool isHitValid(u32);
	virtual bool isCollidMove(THitActor*);
	virtual bool isFindMario(float);

	// new-slot overrides (TFlyEnemy-introduced)
	virtual void flyBehavior();
	virtual void setChaseFlyAnm();
	virtual void setNormalFlyAnm();

	// non-virtual
	void setColorType();
	bool isRollFly();

	// fabricated
	TKillerParams* getSaveParam2() const
	{
		return (TKillerParams*)((TEnemyManager*)mManager)->getSaveParam();
	}

	static bool mTrampleDie;
	static bool mRollSw;
	static bool mSerialBomb;

	/* 0x1A8 */ JGeometry::TVec3<f32> mChaseTarget;
	/* 0x1B4 */ TKillerParams* mKillerParams;
	/* 0x1B8 */ f32 mRollAnim;
	/* 0x1BC */ TMtx34f mRollMtx;
	/* 0x1EC */ GXColorS10 mColor0;
	/* 0x1F4 */ GXColorS10 mColor1;
	/* 0x1FC */ GXColorS10 mColor2;
	/* 0x204 */ GXColorS10 mColor3;
	/* 0x20C */ f32 mExplosionTimer;
};

class TKillerManager : public TSmallEnemyManager {
public:
	TKillerManager(const char* name = "\x83\x4C\x83\x89\x81\x5B\x83\x7D\x83\x6C\x81\x5B\x83\x57\x83\x83\x81\x5B");

	virtual void load(JSUMemoryInputStream&);
	virtual void createModelData();
	virtual TSpineEnemy* createEnemyInstance();
};

class TLiveActor;

DECLARE_NERVE(TNerveKillerExplosion, TLiveActor);

#endif
