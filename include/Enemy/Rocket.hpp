#ifndef ENEMY_ROCKET_HPP
#define ENEMY_ROCKET_HPP

#include <Enemy/SmallEnemy.hpp>
#include <Strategic/Nerve.hpp>

class TRocketParams : public TSmallEnemyParams {
public:
	TRocketParams(const char*);

	/* 0x2D4 */ TParamRT<f32> mSLReleaseSpeed;
	/* 0x2E8 */ TParamRT<f32> mSLFlyGravity;
	/* 0x2FC */ TParamRT<s32> mSLFlyLimitTime;
};

class TWaterEmitInfo;

class TRocket : public TSmallEnemy {
public:
	TRocket(const char* name);

	virtual void load(JSUMemoryInputStream&);
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void bind();
	virtual f32 getGravityY() const;
	virtual const char** getBasNameTable() const;
	virtual void reset();
	virtual void behaveToWater(THitActor*);
	virtual void setDeadAnm();
	virtual void attackToMario();
	virtual void setMActorAndKeeper();
	virtual bool isCollidMove(THitActor*);

	bool isAttack();

	static f32 mTestAng_y;
	static f32 mNozzleOffsetZ;
	static f32 mColOffsetY;
	static f32 mTestAng_x;
	static f32 mTestAng_z;

public:
	/* 0x194 */ JGeometry::TVec3<f32> mInitialPos;
	/* 0x1A0 */ u8 mUnk1A0;
	/* 0x1A1 */ u8 mInitialPosSaved;
	/* 0x1A2 */ u8 unk1A2[2];
	/* 0x1A4 */ TRocketParams* mParams;
};

class TRocketManager : public TSmallEnemyManager {
public:
	TRocketManager(const char* name);

	virtual void perform(u32, JDrama::TGraphics*);
	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual void createModelData();
	virtual TSmallEnemy* createEnemyInstance();
	virtual void initSetEnemies();
	virtual void clipEnemies(JDrama::TGraphics*);

public:
	/* 0x60 */ u8 mActiveFlag;
	/* 0x61 */ u8 pad61[3];
	/* 0x64 */ u32 unk64;
	/* 0x68 */ TWaterEmitInfo* mWaterEmitInfo;
};

DECLARE_NERVE(TNerveRocketWait, TLiveActor);
DECLARE_NERVE(TNerveRocketFly, TLiveActor);
DECLARE_NERVE(TNerveRocketPossessedNozzle, TLiveActor);

#endif
