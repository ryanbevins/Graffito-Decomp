#ifndef ENEMY_BOSS_MANTA_HPP
#define ENEMY_BOSS_MANTA_HPP

#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Strategic/Nerve.hpp>
#include <Strategic/Spine.hpp>

class TBossManta;

class TBossMantaParams : public TSpineEnemyParams {
public:
	TBossMantaParams(const char*);

	/* 0x0A8 */ TParamRT<f32> mSLPolluteRadius;
	/* 0x0BC */ TParamRT<s32> mSLDamageEffectNum;
	/* 0x0D0 */ TParamRT<f32> mSLAppearDemoInitialZ;
	/* 0x0E4 */ TParamRT<f32> mSLAppearDemoWalkSpeed;
	/* 0x0F8 */ TParamRT<s32> mSLMantaRed;
	/* 0x10C */ TParamRT<s32> mSLMantaGreen;
	/* 0x120 */ TParamRT<s32> mSLMantaBlue;
	/* 0x134 */ TParamRT<s32> mSLMantaAlpha;
	/* 0x148 */ TParamRT<s32> mSLAngryMantaRed;
	/* 0x15C */ TParamRT<s32> mSLAngryMantaGreen;
	/* 0x170 */ TParamRT<s32> mSLAngryMantaBlue;
	/* 0x184 */ TParamRT<s32> mSLAngryMantaAlpha;
	/* 0x198 */ TParamRT<f32> mSLAttractorPower;
	/* 0x1AC */ TParamRT<f32> mSLPusherPower;
	/* 0x1C0 */ TParamRT<f32> mSLEscapeLookPoint;
	/* 0x1D4 */ TParamRT<f32> mSLEscapeLookedPoint;
	/* 0x1E8 */ TParamRT<f32> mSLEscapeRegion;
};

class TBossMantaAdditionalCollision : public THitActor {
public:
	TBossMantaAdditionalCollision(const char* name);

	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);

	/* 0x68 */ TBossManta* mOwner;
};

class TBossMantaAdditionalCollisionSet {
public:
	TBossMantaAdditionalCollisionSet();

	void adapt(TBossManta*);
	void update(u32, JDrama::TGraphics*);

	/* 0x00 */ TBossMantaAdditionalCollision* mCollisions[3];
	/* 0x0C */ TBossManta* mOwner;
};

class TBossMantaManager : public TEnemyManager {
public:
	class TMantaBattleState {
	public:
		TMantaBattleState(TBossMantaManager* manager)
		    : mManager(manager)
		    , mState(0)
		{
		}

		void update();

		/* 0x0 */ TBossMantaManager* mManager;
		/* 0x4 */ int mState;
	};

	class TMantaMessageState {
	public:
		TMantaMessageState(TBossMantaManager* manager)
		    : mManager(manager)
		    , mState(0)
		{
		}

		void update();

		/* 0x0 */ TBossMantaManager* mManager;
		/* 0x4 */ int mState;
	};

	TBossMantaManager(const char*);
	virtual ~TBossMantaManager() { }

	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual void perform(u32, JDrama::TGraphics*);
	virtual void createModelData();
	virtual TSpineEnemy* createEnemyInstance();
	virtual void createEnemies(int);

	void spawn(int, const JGeometry::TVec3<f32>&);
	void setupEfbAlpha(JDrama::TGraphics*);
	void updateMantaEscape();
	void drawMantaShadow(JDrama::TGraphics*);

	/* 0x54 */ TBossMantaAdditionalCollisionSet* mCollisionSets[8];
	/* 0x74 */ JGeometry::TVec3<f32>* mPalmPositions;
	/* 0x78 */ JGeometry::TVec3<f32>* mEscapePositions;
	/* 0x7C */ u8* mEfbAlpha;
	/* 0x80 */ int unk80;
	/* 0x84 */ int mShadowAlphaTimer;
	/* 0x88 */ TMantaBattleState mBattleState;
	/* 0x90 */ TMantaMessageState mMessageState;
};

class TBossManta : public TSpineEnemy {
public:
	enum { GENERATION_COUNT = 6 };

	TBossManta(const char*);
	virtual ~TBossManta() { }

	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void control();
	virtual void moveObject();
	virtual void drawObject(JDrama::TGraphics*) { }
	virtual void reset() { }

	void initNthGeneration(int);
	BOOL getIntoGraphVec(JGeometry::TVec3<f32>*);
	bool collidedWithWater();
	f32 getPolluteRadius();
	void updateAttractor();

	static f32 sFrameRate[GENERATION_COUNT];
	static f32 sScale[GENERATION_COUNT];
	static u32 sCenterJointIndex;
	static u32 sBodyJointIndex;
	static u32 sRwingJointIndex;
	static u32 sLwingJointIndex;
	static bool sEscapeFromMario;

	/* 0x150 */ f32 unk150;
	/* 0x154 */ s32 unk154;
	/* 0x158 */ JGeometry::TVec3<f32> mAttractor;
	/* 0x164 */ JGeometry::TVec3<f32> mMoveDir;
	/* 0x170 */ JGeometry::TVec3<f32> mDirection;
	/* 0x17C */ JGeometry::TVec3<f32> mCenterPos;
	/* 0x188 */ s32 mTurnTimer;
	/* 0x18C */ s32 mGeneration;
	/* 0x190 */ f32 mAppearSpeed;
	/* 0x194 */ f32 mMoveSpeed;
	/* 0x198 */ f32 unk198;
	/* 0x19C */ s32 mWaterHitCount;
	/* 0x1A0 */ s32 unk1A0;
	/* 0x1A4 */ s32 unk1A4;
};

DECLARE_NERVE(TNerveMantaAppearDemo, TLiveActor);
DECLARE_NERVE(TNerveMantaDeath, TLiveActor);
DECLARE_NERVE(TNerveMantaSpawn, TLiveActor);
DECLARE_NERVE(TNerveMantaHitWater, TLiveActor);
DECLARE_NERVE(TNerveMantaMove, TLiveActor);

#endif
