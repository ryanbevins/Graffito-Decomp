#ifndef ENEMY_GATE_KEEPER_HPP
#define ENEMY_GATE_KEEPER_HPP

#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <M3DUtil/M3UJoint.hpp>
#include <MarioUtil/ModelUtil.hpp>
#include <Strategic/HitActor.hpp>
#include <Strategic/Nerve.hpp>

class MActor;
class TGKHitObj;
class TBGKObstacle;
class TBGKMtxCalc;

class TGateKeeperBase : public TSpineEnemy {
public:
	TGateKeeperBase(const char* name)
	    : TSpineEnemy(name)
	    , unk150(0)
	    , unk154(0)
	    , unk158(0.0f)
	    , unk15C(0)
	    , unk160(1)
	    , unk161(0)
	    , unk164(1.0f, 1.0f, 1.0f)
	{
		onLiveFlag(LIVE_FLAG_UNK10);
	}

	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void kill();

public:
	/* 0x150 */ MActor* unk150;
	/* 0x154 */ s32 unk154;
	/* 0x158 */ f32 unk158;
	/* 0x15C */ TMultiBtk* unk15C;
	/* 0x160 */ s8 unk160;
	/* 0x161 */ s8 unk161;
	/* 0x162 */ u8 unk162[2];
	/* 0x164 */ JGeometry::TVec3<f32> unk164;
};

class TBiancoGateKeeper : public TGateKeeperBase {
public:
	TBiancoGateKeeper(const char*);

	virtual void perform(u32, JDrama::TGraphics*);
	virtual void init(TLiveManager*);
	virtual void kill();
	virtual const char** getBasNameTable() const;

	void controlCollision();
	void emitParticles();
	BOOL isDamageFogSituation() const;
	BOOL isHeadHitActive() const;
	f32 getRumblePow();
	void launchNamekuri();
	void changeBck(int);

public:
	/* 0x170 */ void* unk170;
	/* 0x174 */ TGKHitObj* unk174;
	/* 0x178 */ TBGKMtxCalc* unk178;
	/* 0x17C */ s16 unk17C;
	/* 0x17E */ s16 unk17E;
	/* 0x180 */ f32 unk180;
	/* 0x184 */ void* unk184;
	/* 0x188 */ u8 unk188[0x100];
	/* 0x288 */ s16 unk288;
	/* 0x28A */ s16 unk28A;
	/* 0x28C */ TBGKObstacle* unk28C;
	/* 0x290 */ s16 unk290;
	/* 0x292 */ s8 unk292;
	/* 0x293 */ u8 unk293;
	/* 0x294 */ s16 unk294;
	/* 0x296 */ u8 unk296;
	/* 0x297 */ u8 unk297;
	/* 0x298 */ s16 unk298;
	/* 0x29C */ f32 unk29C;
};

class TGKHitObj : public THitActor {
public:
	TGKHitObj(TGateKeeperBase* owner, const char* name)
	    : THitActor(name)
	    , unk68(owner)
	    , unk6C(10)
	    , unk70(0)
	{
	}

	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);

public:
	/* 0x68 */ TGateKeeperBase* unk68;
	/* 0x6C */ s32 unk6C;
	/* 0x70 */ s32 unk70;
};

class TBGKObstacle : public THitActor {
public:
	TBGKObstacle(const char* name)
	    : THitActor(name)
	{
	}
};

class TBGKMtxCalc : public M3UMtxCalcSIAnmBlendQuat {
public:
	TBGKMtxCalc(TBiancoGateKeeper* owner)
	    : M3UMtxCalcSIAnmBlendQuat(true)
	    , unk64(owner)
	{
	}

	virtual void calc(u16);

public:
	/* 0x64 */ TBiancoGateKeeper* unk64;
};

class TBiancoGateKeeperParams : public TSpineEnemyParams {
public:
	TBiancoGateKeeperParams(const char* path)
	    : TSpineEnemyParams(path)
	    , PARAM_INIT(mSLDiveTimer, 480)
	    , PARAM_INIT(mSLLoop2Dive, 5)
	    , PARAM_INIT(mSLLaunchTimerNormal, 600)
	    , PARAM_INIT(mSLLaunchTimerDamage, 600)
	{
		TParams::load(mPrmPath);
	}

	/* 0xA8 */ TParamRT<s32> mSLDiveTimer;
	/* 0xBC */ TParamRT<s32> mSLLoop2Dive;
	/* 0xD0 */ TParamRT<s32> mSLLaunchTimerNormal;
	/* 0xE4 */ TParamRT<s32> mSLLaunchTimerDamage;
};

class TBiancoGateKeeperManager : public TEnemyManager {
public:
	TBiancoGateKeeperManager(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void createModelData();
	virtual void initJParticle();
};

class TLiveActor;

DECLARE_NERVE(TNerveBGKSleep, TLiveActor);
DECLARE_NERVE(TNerveBGKAppear, TLiveActor);
DECLARE_NERVE(TNerveBGKWait, TLiveActor);
DECLARE_NERVE(TNerveBGKWait2, TLiveActor);
DECLARE_NERVE(TNerveBGKSleepDamage, TLiveActor);
DECLARE_NERVE(TNerveBGKAwakeDamage, TLiveActor);
DECLARE_NERVE(TNerveBGKDie, TLiveActor);
DECLARE_NERVE(TNerveBGKDive, TLiveActor);
DECLARE_NERVE(TNerveBGKLaunchGoro, TLiveActor);
DECLARE_NERVE(TNerveBGKLaunchName, TLiveActor);

#endif
