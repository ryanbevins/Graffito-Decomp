#ifndef ENEMY_TOBIPUKU_HPP
#define ENEMY_TOBIPUKU_HPP

#include <Enemy/WalkerEnemy.hpp>
#include <Strategic/Nerve.hpp>

class TTobiPuku;
class TTobiPukuLaunchPad;
class J3DNode;

class TTobiPukuParams : public TWalkerEnemyParams {
public:
	TTobiPukuParams(const char* prm)
	    : TWalkerEnemyParams(prm)
	    , PARAM_INIT(mSLBoundNum, 3)
	    , PARAM_INIT(mSLBoundVal, 0.8f)
	    , PARAM_INIT(mSLLifeTimer, 200)
	    , PARAM_INIT(mSLFlyGravityY, 0.2f)
	    , PARAM_INIT(mSLPowerFromWater, 1.0f)
	{
		TParams::load(mPrmPath);
	}

	/* 0x32C */ TParamRT<s32> mSLBoundNum;
	/* 0x340 */ TParamRT<f32> mSLBoundVal;
	/* 0x354 */ TParamRT<s32> mSLLifeTimer;
	/* 0x368 */ TParamRT<f32> mSLFlyGravityY;
	/* 0x37C */ TParamRT<f32> mSLPowerFromWater;
};

class TTobiPukuLaunchPadParams : public TSmallEnemyParams {
public:
	TTobiPukuLaunchPadParams(const char* prm)
	    : TSmallEnemyParams(prm)
	    , PARAM_INIT(mSLLaunchInterval, 300)
	    , PARAM_INIT(mSLLaunchVelocityY, 12.0f)
	    , PARAM_INIT(mSLFlyDist, 1000.0f)
	    , PARAM_INIT(mSLFlySpeed, 30.0f)
	    , PARAM_INIT(mSLLaunchAngle, 45.0f)
	{
		TParams::load(mPrmPath);
	}

	/* 0x2D4 */ TParamRT<s32> mSLLaunchInterval;
	/* 0x2E8 */ TParamRT<f32> mSLLaunchVelocityY;
	/* 0x2FC */ TParamRT<f32> mSLFlyDist;
	/* 0x310 */ TParamRT<f32> mSLFlySpeed;
	/* 0x324 */ TParamRT<f32> mSLLaunchAngle;
};

class TTobiPuku : public TWalkerEnemy {
public:
	TTobiPuku(const char* name = "とびプク");

	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void moveObject();
	virtual void kill();
	virtual f32 getGravityY() const;
	virtual const char** getBasNameTable() const;
	virtual void reset();
	virtual void genEventCoin();
	virtual void behaveToWater(THitActor*);
	virtual void scalingChangeActor();
	virtual void changeOut();
	virtual void setDeadAnm();
	virtual void attackToMario();
	virtual void forceKill();
	virtual void initAttacker(THitActor*);
	virtual BOOL isInhibitedForceMove();
	virtual void generateEffectColumWater();
	virtual void walkBehavior(int, float);
	virtual bool isPichiEffect();
	virtual bool isJumpBck();
	virtual bool isDeadBck();
	virtual bool isJumpStartBck();
	virtual bool isAttackBck();
	virtual bool isFallEndLandBck();
	virtual void setJumpAnm();
	virtual void setJumpStartAnm();
	virtual void setSwimAnm();
	virtual void setAttackAnm();
	virtual void setPichiAnm();
	virtual void setFallAnm();
	virtual void setDownAirAnm();
	virtual void setDownLandAnm();
	virtual void setFallEndLandAnm();
	virtual void swimEffect();
	virtual bool isReachedToGoalXZ();
	virtual void hitWater();

	void hitWall();
	TTobiPukuParams* getTobiPukuParams() const
	{
		return (TTobiPukuParams*)getSaveParam();
	}

	static f32 mLandAngle;
	static u8 mBoundSw;
	static f32 mBoundVelocityY;
	static u8 mReturnLaunchSw;

public:
	/* 0x194 */ u8 unk194;
	/* 0x195 */ u8 unk195[3];
	/* 0x198 */ s32 unk198;
	/* 0x19C */ TTobiPukuParams* mTobiPukuParams;
	/* 0x1A0 */ JGeometry::TVec3<f32> unk1A0;
	/* 0x1AC */ u8 unk1AC;
	/* 0x1AD */ u8 unk1AD;
	/* 0x1AE */ u8 unk1AE;
	/* 0x1AF */ u8 unk1AF;
	/* 0x1B0 */ f32 unk1B0;
	/* 0x1B4 */ f32 unk1B4;
	/* 0x1B8 */ JGeometry::TVec3<f32> unk1B8[2];
	/* 0x1D0 */ JGeometry::TVec3<f32> mLaunchVelocity;
	/* 0x1DC */ TTobiPukuLaunchPad* mLaunchPad;
	/* 0x1E0 */ f32 unk1E0;
	/* 0x1E4 */ f32 unk1E4;
	/* 0x1E8 */ f32 unk1E8;
	/* 0x1EC */ f32 unk1EC;
	/* 0x1F0 */ f32 unk1F0;
};

class TMoePuku : public TTobiPuku {
public:
	TMoePuku(const char* name = "モエプク")
	    : TTobiPuku(name)
	{
	}

	virtual void calcRootMatrix();
	virtual const char** getBasNameTable() const;
	virtual void generateEffectColumWater();
	virtual void setDeadAnm();
	virtual bool isPichiEffect();
	virtual bool isJumpBck();
	virtual bool isDeadBck();
	virtual bool isJumpStartBck();
	virtual bool isAttackBck();
	virtual bool isFallEndLandBck();
	virtual void setJumpAnm();
	virtual void setJumpStartAnm();
	virtual void setSwimAnm();
	virtual void setAttackAnm();
	virtual void setPichiAnm();
	virtual void setFallAnm();
	virtual void setDownAirAnm();
	virtual void setDownLandAnm();
	virtual void setFallEndLandAnm();
	virtual void swimEffect();
	virtual void hitWater();
};

class TPukuPuku : public TTobiPuku {
public:
	TPukuPuku(const char* name = "プクプク");

	virtual void load(JSUMemoryInputStream&);
	virtual void init(TLiveManager*);
	virtual void reset();
};

class TTobiPukuLaunchPad : public TSmallEnemy {
public:
	TTobiPukuLaunchPad(const char* name = "とびプク発射台");

	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*);
	virtual void init(TLiveManager*);
	virtual void reset();
	virtual void launch();

	void forceLaunch(TTobiPuku*);
	TTobiPukuLaunchPadParams* getLaunchParams() const
	{
		return mLaunchParams;
	}

public:
	/* 0x194 */ s32 mTimer;
	/* 0x198 */ TTobiPukuLaunchPadParams* mLaunchParams;
	/* 0x19C */ f32 mLaunchSpeed;
	/* 0x1A0 */ u8 unk1A0[8];
	/* 0x1A8 */ TTobiPuku* mLaunchedPuku;
};

class TMoePukuLaunchPad : public TTobiPukuLaunchPad {
public:
	TMoePukuLaunchPad(const char* name = "モエプク発射台")
	    : TTobiPukuLaunchPad(name)
	{
	}

	virtual void launch();
};

class TTobiPukuManager : public TSmallEnemyManager {
public:
	TTobiPukuManager(const char* name = "とびプクマネージャー");

	virtual void load(JSUMemoryInputStream&);
	virtual TSpineEnemy* createEnemyInstance();
};

class TMoePukuManager : public TTobiPukuManager {
public:
	TMoePukuManager(const char* name = "モエプクマネージャー")
	    : TTobiPukuManager(name)
	{
	}

	virtual TSpineEnemy* createEnemyInstance();
};

class TTobiPukuLaunchPadManager : public TSmallEnemyManager {
public:
	TTobiPukuLaunchPadManager(const char* name = "とびプク発射台マネージャー");

	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*);
	virtual TSpineEnemy* createEnemyInstance();

public:
	/* 0x60 */ u8 mForceJumpToPad;
	/* 0x61 */ u8 unk61[3];
};

class TMoePukuLaunchPadManager : public TTobiPukuLaunchPadManager {
public:
	TMoePukuLaunchPadManager(const char* name = "モエプク発射台マネージャー")
	    : TTobiPukuLaunchPadManager(name)
	{
	}

	virtual TSpineEnemy* createEnemyInstance();
};

#define DECLARE_TOBIPUKU_NERVE(Name)                                           \
	class Name : public TNerveBase<TLiveActor> {                                \
	public:                                                                     \
		virtual BOOL execute(TSpineBase<TLiveActor>*) const;                     \
		static const Name& theNerve()                                            \
		{                                                                       \
			static Name instance;                                                \
			return instance;                                                     \
		}                                                                       \
	};

DECLARE_TOBIPUKU_NERVE(TNerveTobiPukuGenerate);
DECLARE_TOBIPUKU_NERVE(TNerveTobiPukuFly);
DECLARE_TOBIPUKU_NERVE(TNerveTobiPukuAttack);
DECLARE_TOBIPUKU_NERVE(TNerveTobiPukuHitWater);
DECLARE_TOBIPUKU_NERVE(TNerveTobiPukuFall);
DECLARE_TOBIPUKU_NERVE(TNerveTobiPukuPitiPiti);
DECLARE_TOBIPUKU_NERVE(TNerveTobiPukuDie);
DECLARE_TOBIPUKU_NERVE(TNerveTobiPukuLand);
DECLARE_TOBIPUKU_NERVE(TNerveTobiPukuBound);
DECLARE_TOBIPUKU_NERVE(TNerveTobiPukuPrepareFly);
DECLARE_TOBIPUKU_NERVE(TNerveTobiPukuReturnLaunch);
DECLARE_TOBIPUKU_NERVE(TNerveTobiPukuSwimWander);

#undef DECLARE_TOBIPUKU_NERVE

int TobiPukuRollCallback(J3DNode*, int);

#endif
