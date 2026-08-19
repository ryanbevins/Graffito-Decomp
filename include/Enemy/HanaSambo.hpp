#ifndef ENEMY_HANA_SAMBO_HPP
#define ENEMY_HANA_SAMBO_HPP

#include <Enemy/WalkerEnemy.hpp>
#include <JSystem/JDrama/JDRViewObj.hpp>

class J3DMaterialTable;
class SDLModel;
class TMBindShadowBody;
class TMapObjBase;
class TSamboFlowerCoinUnit;
class TSamboFlower;
class TSamboFlowerManager;
class THanaSambo;

class TSamboFlowerSaveLoadParams : public TSpineEnemyParams {
public:
	TSamboFlowerSaveLoadParams(const char*);

	/* 0x0A8 */ TParamRT<f32> mSLLeafVelocityXZ;
	/* 0x0BC */ TParamRT<f32> mSLLeafVelocityY;
	/* 0x0D0 */ TParamRT<f32> mSLLeafGravity;
	/* 0x0E4 */ TParamRT<f32> mSLBudDist;
	/* 0x0F8 */ TParamRT<s32> mSLBloomTimer;
	/* 0x10C */ TParamRT<f32> mSLCoinCircleR;
	/* 0x120 */ TParamRT<f32> mSLCoinVelocityXZ;
	/* 0x134 */ TParamRT<f32> mSLCoinVelocityY;
	/* 0x148 */ TParamRT<f32> mSLSeedShootRange;
	/* 0x15C */ TParamRT<s32> mSLSeedShootInterval;
	/* 0x170 */ TParamRT<f32> mSLSeedGravity;
	/* 0x184 */ TParamRT<f32> mSLSeedSpeedXZ;
	/* 0x198 */ TParamRT<f32> mSLSeedSpeedY;
};

class TSamboFlower : public TSpineEnemy {
public:
	TSamboFlower(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void init(TLiveManager*);
	virtual void control();
	virtual void moveObject();
	virtual void drawObject(JDrama::TGraphics*);
	virtual void reset();
	virtual void setMActorAndKeeper();

	TSamboFlowerSaveLoadParams* getSamboFlowerParams() const { return mParams; }

public:
	/* 0x150 */ u8 unk150;
	/* 0x151 */ u8 unk151[3];
	/* 0x154 */ s32 unk154;
	/* 0x158 */ s32 unk158;
	/* 0x15C */ s32 unk15C;
	/* 0x160 */ u8 unk160;
	/* 0x161 */ u8 unk161[3];
	/* 0x164 */ s32* unk164;
	/* 0x168 */ TMapObjBase* unk168;
	/* 0x16C */ TSamboFlowerSaveLoadParams* mParams;
};

class TSamboFlowerCoinUnit {
public:
	TSamboFlowerCoinUnit(int capacity)
	    : mFlowers(nullptr)
	    , mCenter(0.0f, 0.0f, 0.0f)
	    , mFlowerCount(0)
	    , mCapacity(capacity)
	    , mCoin(nullptr)
	    , unk1C(capacity)
	{
		mFlowers = new TSamboFlower*[capacity];
	}

	void checkGenCoin();

	/* 0x00 */ TSamboFlower** mFlowers;
	/* 0x04 */ JGeometry::TVec3<f32> mCenter;
	/* 0x10 */ s32 mFlowerCount;
	/* 0x14 */ s32 mCapacity;
	/* 0x18 */ TMapObjBase* mCoin;
	/* 0x1C */ s32 unk1C;
};

class TSamboLeaf : public JDrama::TViewObj {
public:
	TSamboLeaf(const char* name, SDLModel* model, TSamboFlowerManager* manager)
	    : JDrama::TViewObj(name)
	    , mModel(model)
	    , mActive(false)
	    , mManager(manager)
	{
	}

	virtual void perform(u32, JDrama::TGraphics*);

	/* 0x10 */ SDLModel* mModel;
	/* 0x14 */ JGeometry::TVec3<f32> mPosition;
	/* 0x20 */ JGeometry::TVec3<f32> mRotation;
	/* 0x2C */ JGeometry::TVec3<f32> mScale;
	/* 0x38 */ JGeometry::TVec3<f32> mVelocity;
	/* 0x44 */ bool mActive;
	/* 0x45 */ u8 unk45[3];
	/* 0x48 */ TSamboFlowerManager* mManager;
};

class TSamboFlowerManager : public TEnemyManager {
public:
	TSamboFlowerManager(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual void perform(u32, JDrama::TGraphics*);
	virtual void createModelData();
	virtual TSpineEnemy* createEnemyInstance();
	virtual void dropLeaf(JGeometry::TVec3<f32>&, JGeometry::TVec3<f32>&);

	/* 0x54 */ TSamboFlowerCoinUnit** mCoinUnits;
	/* 0x58 */ s32 mCoinUnitCount;
	/* 0x5C */ void* unk5C;
	/* 0x60 */ TSamboLeaf** mLeaves;
	/* 0x64 */ J3DMaterialTable* mMaterialTable;
};

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
	/* 0x1A0 */ JGeometry::TVec3<f32> unk1A0;
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
