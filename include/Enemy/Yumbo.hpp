#ifndef ENEMY_YUMBO_HPP
#define ENEMY_YUMBO_HPP

#include <Enemy/SmallEnemy.hpp>
#include <Strategic/HitActor.hpp>
#include <Strategic/Nerve.hpp>

class MActor;
class TYumbo;

class TYumboParams : public TSmallEnemyParams {
public:
	TYumboParams(const char*);

	/* 0x2D4 */ TParamRT<s32> mRecoverTimer;
	/* 0x2E8 */ TParamRT<f32> mShootSpeed;
	/* 0x2FC */ TParamRT<f32> mShootAngleX;
	/* 0x310 */ TParamRT<s32> mSeedLife;
	/* 0x324 */ TParamRT<f32> mSeedAirFric;
	/* 0x338 */ TParamRT<f32> mSeedGravityY;
};

class TYumboManager : public TSmallEnemyManager {
public:
	TYumboManager(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void createModelData();

public:
	/* 0x60 */ void* mFlowerMatTable;
};

class TYumboSeed : public THitActor {
public:
	TYumboSeed(TYumbo* owner, MActor* mActor)
	    : THitActor("\x83\x86\x83\x93\x83\x7B\x8E\xED")
	    , mOwner(owner)
	    , mMActor(mActor)
	    , mState(1)
	{
	}

	virtual void perform(u32, JDrama::TGraphics*);
	virtual void init();

public:
	/* 0x68 */ TYumbo* mOwner;
	/* 0x6C */ MActor* mMActor;
	/* 0x70 */ u32 mState;
	/* 0x74 */ s32 mLife;
	/* 0x78 */ JGeometry::TVec3<f32> mVelocity;
};

class TYumbo : public TSmallEnemy {
public:
	TYumbo(const char* = "\x83\x86\x83\x93\x83\x7B");

	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void init(TLiveManager*);
	virtual void moveObject();
	virtual void perform(u32, JDrama::TGraphics*);
	virtual void reset();
	virtual const char** getBasNameTable() const;
	virtual void behaveToWater(THitActor*);
	virtual void setDeadAnm();
	virtual void attackToMario();
	virtual bool doKeepDistance();

	void shotSeeds();

	// fabricated
	TYumboParams* getSaveParam2() const { return (TYumboParams*)getSaveParam(); }

public:
	/* 0x194 */ TYumboSeed* mSeeds[16];
	/* 0x1D4 */ int mDanceJointIndex;
	/* 0x1D8 */ u8 mSpawnedHideParticles;
};

DECLARE_NERVE(TNerveYumboFreeze, TLiveActor);
DECLARE_NERVE(TNerveYumboDancing, TLiveActor);
DECLARE_NERVE(TNerveYumboAttack, TLiveActor);
DECLARE_NERVE(TNerveYumboAppearing, TLiveActor);
DECLARE_NERVE(TNerveYumboHiding, TLiveActor);

#endif
