#ifndef ENEMY_BOSS_TELESA_HPP
#define ENEMY_BOSS_TELESA_HPP

#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Enemy/WalkerEnemy.hpp>
#include <JSystem/JDrama/JDRViewObj.hpp>
#include <JSystem/JGeometry.hpp>
#include <MoveBG/MapObjSirena.hpp>
#include <Strategic/HitActor.hpp>

class TBossTelesaSaveLoadParams : public TSpineEnemyParams {
public:
	TBossTelesaSaveLoadParams(const char*);

	/* 0x0A8 */ TParamRT<s32> mSLDamageRadius;
	/* 0x0BC */ TParamRT<s32> mSLDamageHeight;
	/* 0x0D0 */ TParamRT<s32> mSLAttackRadius;
	/* 0x0E4 */ TParamRT<s32> mSLAttackHeight;
	/* 0x0F8 */ TParamRT<s32> mSLGenAttackerTime;
	/* 0x10C */ TParamRT<s32> mSLGenBubbleTime;
	/* 0x120 */ TParamRT<f32> mSLHitAngle;
	/* 0x134 */ TParamRT<s32> mSLNumGenBubble;
	/* 0x148 */ TParamRT<f32> mSL1stBubbleSp;
	/* 0x15C */ TParamRT<f32> mSLHideAreaRadius;
	/* 0x170 */ TParamRT<s32> mSLSlotItemNum;
	/* 0x184 */ TParamRT<s32> mSLSlotFruitNum;
	/* 0x198 */ TParamRT<f32> mSLSlotFirstHitCollectRate;
	/* 0x1AC */ TParamRT<f32> mSLSlotHitCollectRate;
	/* 0x1C0 */ TParamRT<f32> mSLTransYOffset;
	/* 0x1D4 */ TParamRT<s32> mSLStopSlotTime0;
	/* 0x1E8 */ TParamRT<s32> mSLStopSlotTime1;
	/* 0x1FC */ TParamRT<s32> mSLStopSlotTime2;
	/* 0x210 */ TParamRT<s32> mSLSpicyTime;
};

class TBossTelesa : public TSpineEnemy {
public:
	TBossTelesa(const char*);

	virtual void loadAfter();
	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual MtxPtr getTakingMtx();
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void moveObject();
	virtual void kill();
	virtual const char** getBasNameTable() const;
	virtual void reset();

	void forceHide();
	void forceAllItemKill();
	void generateSlotItem();
	void rouletteStart();
	void genAttacker();
	void flashItem(int);
	void slotFall();
	void rouletteFall();
	void damageRecover();
	void setSpicy(TLiveActor*);
	void checkHitObject(THitActor*);

	static f32 mEnemyGenRate;
	static f32 mItemGenRate;
	static u8 mNormalAlpha;
	static f32 mBaseHoseiPosY;
	static f32 mRouletteUpRate;
	static u32 mTelesaGenerateInterval;
	static f32 mCameraMoveLimit;
	static f32 mCameraMoveSp;

public:
	/* 0x150 */ u8 unk150;
	/* 0x151 */ u8 unk151[0x3];
	/* 0x154 */ TTelesaSlot* unk154;
	/* 0x158 */ void* unk158;
	/* 0x15C */ void* unk15C;
	/* 0x160 */ s32 unk160;
	/* 0x164 */ s32 unk164;
	/* 0x168 */ f32 unk168;
	/* 0x16C */ THitActor* unk16C;
	/* 0x170 */ THitActor* unk170;
	/* 0x174 */ THitActor* unk174;
	/* 0x178 */ TLiveActor* unk178;
	/* 0x17C */ void* unk17C;
	/* 0x180 */ void* unk180;
	/* 0x184 */ JDrama::TViewObj* unk184;
	/* 0x188 */ JDrama::TViewObj* unk188;
	/* 0x18C */ u8 unk18C;
	/* 0x18D */ u8 unk18D[0x1B];
	/* 0x1A8 */ s32 unk1A8;
	/* 0x1AC */ u8 unk1AC[0xC8];
	/* 0x274 */ s32 unk274;
	/* 0x278 */ JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > unk278;
	/* 0x2A8 */ u8 unk2A8[0xA8];
	/* 0x350 */ u8 unk350;
	/* 0x351 */ u8 unk351[0x3];
	/* 0x354 */ s32 unk354;
	/* 0x358 */ u16 unk358;
	/* 0x35A */ u8 unk35A;
	/* 0x35B */ u8 unk35B;
	/* 0x35C */ s32 unk35C;
	/* 0x360 */ f32 unk360;
	/* 0x364 */ f32 unk364;
	/* 0x368 */ s32 unk368;
	/* 0x36C */ s32 unk36C;
	/* 0x370 */ u8 unk370;
	/* 0x371 */ u8 unk371[0x13];
	/* 0x384 */ u8 unk384;
	/* 0x385 */ u8 unk385[0x7];
};

class TBossTelesaBody : public THitActor {
public:
	TBossTelesaBody(const char* name)
	    : THitActor(name)
	{
	}

	virtual BOOL receiveMessage(THitActor*, u32);
};

class TBossTelesaTongue : public THitActor {
public:
	TBossTelesaTongue(const char* name)
	    : THitActor(name)
	{
	}

	virtual BOOL receiveMessage(THitActor*, u32);
};

class TBossTelesaKillSmallEnemy : public THitActor {
public:
	TBossTelesaKillSmallEnemy(const char* name)
	    : THitActor(name)
	{
	}

	BOOL checkHit();
};

class TBossTelesaManager : public TEnemyManager {
public:
	TBossTelesaManager(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*);
	virtual void createModelData();
	virtual TSpineEnemy* createEnemyInstance();
	virtual void clipEnemies(JDrama::TGraphics*) { }
};

class TBubbleSaveLoadParams : public TWalkerEnemyParams {
public:
	TBubbleSaveLoadParams(const char* path)
	    : TWalkerEnemyParams(path)
	    , PARAM_INIT(mSLLiveTime, 200)
	    , PARAM_INIT(mSLNumDivision, 5)
	    , PARAM_INIT(mSLMaxScale, 1.5f)
	    , PARAM_INIT(mSLAddPosBase, 50.0f)
	    , PARAM_INIT(mSLRateExpand, 1.001f)
	    , PARAM_INIT(mSLDeadHeight, 300.0f)
	{
		TParams::load(mPrmPath);
	}

	/* 0x32C */ TParamRT<s32> mSLLiveTime;
	/* 0x340 */ TParamRT<s32> mSLNumDivision;
	/* 0x354 */ TParamRT<f32> mSLMaxScale;
	/* 0x368 */ TParamRT<f32> mSLAddPosBase;
	/* 0x37C */ TParamRT<f32> mSLRateExpand;
	/* 0x390 */ TParamRT<f32> mSLDeadHeight;
};

class TBubble : public TWalkerEnemy {
public:
	TBubble(const char* name = "パブル")
	    : TWalkerEnemy(name)
	    , unk194(nullptr)
	    , unk198(0)
	    , unk1CC(0.0f)
	    , unk1D0(0)
	    , unk1D1(0)
	    , unk1D2(0)
	    , unk1D3(0)
	{
	}

	virtual MtxPtr getTakingMtx();
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void kill();
	virtual f32 getGravityY() const;
	virtual const char** getBasNameTable() const;
	virtual void reset();
	virtual void behaveToWater(THitActor*);
	virtual void setDeadAnm();
	virtual void attackToMario();
	virtual void setAfterDeadEffect() { }

	void appendEnemy();
	void split();

public:
	/* 0x194 */ TBubbleSaveLoadParams* unk194;
	/* 0x198 */ s32 unk198;
	/* 0x19C */ JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > unk19C;
	/* 0x1CC */ f32 unk1CC;
	/* 0x1D0 */ u8 unk1D0;
	/* 0x1D1 */ u8 unk1D1;
	/* 0x1D2 */ u8 unk1D2;
	/* 0x1D3 */ u8 unk1D3;
};

class TBubbleManager : public TSmallEnemyManager {
public:
	TBubbleManager(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void createModelData();
	virtual TSmallEnemy* createEnemyInstance();
};

DECLARE_NERVE(TNerveBossTelesaFallDemo, TLiveActor);
DECLARE_NERVE(TNerveBossTelesaFreeze, TLiveActor);
DECLARE_NERVE(TNerveBossTelesaPrepareSlot, TLiveActor);
DECLARE_NERVE(TNerveBossTelesaSpitSlotItem, TLiveActor);
DECLARE_NERVE(TNerveBossTelesaSlotStart, TLiveActor);
DECLARE_NERVE(TNerveBossTelesaAppear, TLiveActor);
DECLARE_NERVE(TNerveBossTelesaHideWait, TLiveActor);
DECLARE_NERVE(TNerveBossTelesaHide, TLiveActor);
DECLARE_NERVE(TNerveBossTelesaSpit, TLiveActor);
DECLARE_NERVE(TNerveBossTelesaDie, TLiveActor);
DECLARE_NERVE(TNerveBubbleSplit, TLiveActor);
DECLARE_NERVE(TNerveBubbleLive, TLiveActor);

#endif
