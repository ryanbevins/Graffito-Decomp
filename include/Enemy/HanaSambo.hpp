#ifndef ENEMY_HANA_SAMBO_HPP
#define ENEMY_HANA_SAMBO_HPP

#include <Enemy/WalkerEnemy.hpp>

class TMBindShadowBody;
class TSamboFlower;
class THanaSambo;

class TSamboHeadSaveLoadParams : public TWalkerEnemyParams {
public:
	TSamboHeadSaveLoadParams(const char* path)
	    : TWalkerEnemyParams(path)
	    , PARAM_INIT(mSLAppearDist, 800.0f)
	    , PARAM_INIT(mSLHideDist, 1000.0f)
	    , PARAM_INIT(mSLMoveDist, 100.0f)
	    , PARAM_INIT(mSLMoveGravity, 0.1f)
	    , PARAM_INIT(mSLJumpSp, 10.0f)
	    , PARAM_INIT(mSLJumpPrepareTime, 20)
	    , PARAM_INIT(mSLHitJumpSpXZ, 12.0f)
	    , PARAM_INIT(mSLHitJumpSpY, 10.0f)
	    , PARAM_INIT(mSLHitJumpGravity, 1.0f)
	    , PARAM_INIT(mSLHitJumpSpRateXZ, 0.3f)
	    , PARAM_INIT(mSLHitJumpSpRateY, 0.3f)
	    , PARAM_INIT(mSLJumpAngY, 30.0f)
	{
		TParams::load(mPrmPath);
	}

	/* 0x32C */ TParamRT<f32> mSLAppearDist;
	/* 0x340 */ TParamRT<f32> mSLHideDist;
	/* 0x354 */ TParamRT<f32> mSLMoveDist;
	/* 0x368 */ TParamRT<f32> mSLMoveGravity;
	/* 0x37C */ TParamRT<f32> mSLJumpSp;
	/* 0x390 */ TParamRT<s32> mSLJumpPrepareTime;
	/* 0x3A4 */ TParamRT<f32> mSLHitJumpSpXZ;
	/* 0x3B8 */ TParamRT<f32> mSLHitJumpSpY;
	/* 0x3CC */ TParamRT<f32> mSLHitJumpGravity;
	/* 0x3E0 */ TParamRT<f32> mSLHitJumpSpRateXZ;
	/* 0x3F4 */ TParamRT<f32> mSLHitJumpSpRateY;
	/* 0x408 */ TParamRT<f32> mSLJumpAngY;
};

class TSamboHead : public TWalkerEnemy {
public:
	TSamboHead(const char* = "サンボヘッド");

	virtual void load(JSUMemoryInputStream&);
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void kill();
	virtual f32 getGravityY() const;
	virtual const char** getBasNameTable() const;
	virtual void reset();
	virtual void genEventCoin();
	virtual void behaveToWater(THitActor*);
	virtual void setDeadAnm();
	virtual void attackToMario();
	virtual void setMActorAndKeeper();
	virtual void setAfterDeadEffect();

	TSamboHeadSaveLoadParams* getSamboHeadParams() const { return mParams; }

	static u8 mBodyJntIndex;

public:
	/* 0x194 */ TSamboHeadSaveLoadParams* mParams;
	/* 0x198 */ TLiveActor* unk198;
	/* 0x19C */ s32 unk19C;
	/* 0x1A0 */ u8 unk1A0[0xC];
	/* 0x1AC */ f32 mRollAngle;
	/* 0x1B0 */ u8 unk1B0;
	/* 0x1B1 */ u8 unk1B1[3];
};

class TSamboHeadManager : public TSmallEnemyManager {
public:
	TSamboHeadManager(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void createModelData();
	virtual TSpineEnemy* createEnemyInstance();
};

class THanaSamboSaveLoadParams : public TSmallEnemyParams {
public:
	THanaSamboSaveLoadParams(const char* path)
	    : TSmallEnemyParams(path)
	    , PARAM_INIT(mSLAttackDist, 200.0f)
	    , PARAM_INIT(mSLAttackInterval, 200)
	    , PARAM_INIT(mSLAppearDist, 800.0f)
	    , PARAM_INIT(mSLHideDist, 1000.0f)
	    , PARAM_INIT(mSLAttackingTime, 30)
	    , PARAM_INIT(mSLHeadAttackRadius, 60.0f)
	    , PARAM_INIT(mSLHeadAttackHeight, 20.0f)
	    , PARAM_INIT(mSLHeadDamageRadius, 80.0f)
	    , PARAM_INIT(mSLHeadDamageHeight, 40.0f)
	{
		TParams::load(mPrmPath);
	}

	/* 0x2D4 */ TParamRT<f32> mSLAttackDist;
	/* 0x2E8 */ TParamRT<s32> mSLAttackInterval;
	/* 0x2FC */ TParamRT<f32> mSLAppearDist;
	/* 0x310 */ TParamRT<f32> mSLHideDist;
	/* 0x324 */ TParamRT<s32> mSLAttackingTime;
	/* 0x338 */ TParamRT<f32> mSLHeadAttackRadius;
	/* 0x34C */ TParamRT<f32> mSLHeadAttackHeight;
	/* 0x360 */ TParamRT<f32> mSLHeadDamageRadius;
	/* 0x374 */ TParamRT<f32> mSLHeadDamageHeight;
};

class THanaSamboHead : public THitActor {
public:
	THanaSamboHead(const char* name)
	    : THitActor(name)
	    , mOwner(nullptr)
	{
	}

	virtual BOOL receiveMessage(THitActor*, u32);

	/* 0x68 */ THanaSambo* mOwner;
};

class THanaSambo : public TSmallEnemy {
public:
	THanaSambo(const char* = "ハナサンボ");

	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*);
	virtual void init(TLiveManager*);
	virtual void moveObject();
	virtual void drawObject(JDrama::TGraphics*);
	virtual void kill();
	virtual const char** getBasNameTable() const;
	virtual void reset();
	virtual void behaveToWater(THitActor*);
	virtual void setDeadAnm();
	virtual void setWaitAnm();
	virtual void setMActorAndKeeper();
	virtual bool isHitValid(u32);
	virtual bool isCollidMove(THitActor*);

	void createPollen();

	THanaSamboSaveLoadParams* getHanaSamboParams() const { return mParams; }

	static u8 mHeadJntIndex;
	static u8 mPollenJntIndex;

public:
	/* 0x194 */ THanaSamboHead* mHead;
	/* 0x198 */ THanaSamboSaveLoadParams* mParams;
	/* 0x19C */ JGeometry::TVec3<f32> mRootPosition;
	/* 0x1A8 */ TSamboFlower* mFlower;
	/* 0x1AC */ TMBindShadowBody* mBindShadow;
	/* 0x1B0 */ bool mUseYDownAnim;
	/* 0x1B1 */ u8 unk1B1[3];
	/* 0x1B4 */ JGeometry::TVec3<f32> mPollenPositions[4];
};

class THanaSamboManager : public TSmallEnemyManager {
public:
	THanaSamboManager(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void createModelData();
	virtual TSpineEnemy* createEnemyInstance();
};

DECLARE_NERVE(TNerveHanaSamboFreeze, TLiveActor);
DECLARE_NERVE(TNerveHanaSamboDie, TLiveActor);
DECLARE_NERVE(TNerveHanaSamboHide, TLiveActor);
DECLARE_NERVE(TNerveHanaSamboAttack, TLiveActor);
DECLARE_NERVE(TNerveHanaSamboWait, TLiveActor);
DECLARE_NERVE(TNerveHanaSamboAppear, TLiveActor);

DECLARE_NERVE(TNerveSamboHeadHitWall, TLiveActor);
DECLARE_NERVE(TNerveSamboHeadRecoverWater, TLiveActor);
DECLARE_NERVE(TNerveSamboHeadHitWater, TLiveActor);
DECLARE_NERVE(TNerveSamboHeadHide, TLiveActor);
DECLARE_NERVE(TNerveSamboHeadAttack, TLiveActor);
DECLARE_NERVE(TNerveSamboHeadAppear, TLiveActor);

#endif
