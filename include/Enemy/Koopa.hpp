#ifndef ENEMY_KOOPA_HPP
#define ENEMY_KOOPA_HPP

#include <Enemy/BathtubKiller.hpp>
#include <Enemy/DirectionCalc.hpp>
#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Strategic/HitActor.hpp>
#include <Strategic/Nerve.hpp>

class TKoopaJr;
class TKoopaJrSubmarine;
class TBathtubBinder;
class TKoopa;

class TKoopaParams : public TSpineEnemyParams {
public:
	TKoopaParams(const char*);

	/* 0xA8 */ TParamRT<f32> turnSpeed;
	/* 0xBC */ TParamRT<f32> turnAnim;
	/* 0xD0 */ TParamRT<f32> waitStep;
	/* 0xE4 */ TParamRT<f32> downStep;
	/* 0xF8 */ TParamRT<f32> attackRadius;
	/* 0x10C */ TParamRT<f32> attackHeight;
	/* 0x120 */ TParamRT<f32> focusRange;
	/* 0x134 */ TParamRT<f32> waitRange;
	/* 0x148 */ TParamRT<f32> fireSpeed;
	/* 0x15C */ TParamRT<f32> tumbleWeight;
	/* 0x170 */ TParamRT<f32> tumbleSpeed;
	/* 0x184 */ TParamRT<f32> tumbleStartFrame;
	/* 0x198 */ TParamRT<f32> tumbleEndFrame;
	/* 0x1AC */ TParamRT<f32> waitSpeed;
	/* 0x1C0 */ TParamRT<f32> staggerSpeed;
	/* 0x1D4 */ TParamRT<f32> downSpeed;
	/* 0x1E8 */ TParamRT<f32> flameVelocity;
	/* 0x1FC */ TParamRT<f32> flameScale;
	/* 0x210 */ TParamRT<s32> flameCount;
	/* 0x224 */ TParamRT<s32> flameFocusStartStep;
	/* 0x238 */ TParamRT<s32> flameFocusEndStep;
	/* 0x24C */ TParamRT<f32> flameRadius;
	/* 0x260 */ TParamRT<f32> flameHeight;
	/* 0x274 */ TParamRT<f32> headRadius;
	/* 0x288 */ TParamRT<f32> waterhitSpeed;
	/* 0x29C */ TParamRT<f32> flameOverStart;
	/* 0x2B0 */ TParamRT<f32> flameNeckRange;
	/* 0x2C4 */ TParamRT<f32> flameNeckDownRate;
	/* 0x2D8 */ TParamRT<f32> flameJump;
	/* 0x2EC */ TParamRT<f32> fallSpeed;
	/* 0x300 */ TParamRT<f32> marioEstimationFire;
	/* 0x314 */ TParamRT<f32> marioEstimationWait;
};

class TKoopaParts : public THitActor {
public:
	TKoopaParts(const char*, u32, TKoopa*, f32);

	virtual void perform(u32, JDrama::TGraphics*);
	virtual void control();
	virtual void attack_(THitActor*) = 0;

	/* 0x68 */ TKoopa* mOwner;
};

class TKoopaBody : public TKoopaParts {
public:
	TKoopaBody(const char* name, u32 actor_type, TKoopa* owner, f32 radius)
	    : TKoopaParts(name, actor_type, owner, radius)
	{
	}
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void attack_(THitActor*);
};

class TKoopaHead : public TKoopaParts {
public:
	TKoopaHead(const char* name, u32 actor_type, TKoopa* owner, f32 radius)
	    : TKoopaParts(name, actor_type, owner, radius)
	{
	}
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void attack_(THitActor*);
};

class TKoopaHand : public TKoopaParts {
public:
	TKoopaHand(const char* name, u32 actor_type, TKoopa* owner, f32 radius)
	    : TKoopaParts(name, actor_type, owner, radius)
	{
	}
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void attack_(THitActor*);
};

class TKoopaFlame : public TKoopaParts {
public:
	TKoopaFlame(const char* name, u32 actor_type, TKoopa* owner, f32 radius)
	    : TKoopaParts(name, actor_type, owner, radius)
	    , unk88(0.0f)
	    , unk8C(1.0f)
	{
	}
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void control();
	virtual void attack_(THitActor*);

	f32 getCurrentTime() const { return unk8C; }

	/* 0x6C */ f32 unk6C;
	/* 0x70 */ f32 unk70;
	/* 0x74 */ f32 unk74;
	/* 0x78 */ f32 unk78;
	/* 0x7C */ f32 unk7C;
	/* 0x80 */ f32 unk80;
	/* 0x84 */ f32 unk84;
	/* 0x88 */ f32 unk88;
	/* 0x8C */ f32 unk8C;
	/* 0x90 */ f32 unk90;
	/* 0x94 */ f32 unk94;
};

class TKoopaJrParams : public TSpineEnemyParams {
public:
	TKoopaJrParams(const char*);

	/* 0xA8 */ TParamRT<f32> mSLLaunchKillerLimit;
	/* 0xBC */ TParamRT<f32> mSLDamageRadius;
	/* 0xD0 */ TParamRT<f32> mSLDamageHeight;
	/* 0xE4 */ TParamRT<f32> mSLKoopaJrScale;
	/* 0xF8 */ TParamRT<f32> mSLFastLaunchDistance;
	/* 0x10C */ TParamRT<s32> mSLDamagePeriod;
	/* 0x120 */ TParamRT<s32> mSLLaunchKillerPeriod;
	/* 0x134 */ TParamRT<s32> mSLLaunchKillerPeriodFast;
};

class TKoopaJrSubmarineParams : public TSpineEnemyParams {
public:
	TKoopaJrSubmarineParams(const char*);

	/* 0xA8 */ TParamRT<f32> killerTargetDistanceMin;
	/* 0xBC */ TParamRT<f32> killerTargetDistance;
	/* 0xD0 */ TParamRT<f32> bottomHeight;
	/* 0xE4 */ TParamRT<f32> centerZ;
	/* 0xF8 */ TParamRT<f32> aboidKoopaFlameAngle;
	/* 0x10C */ TParamRT<f32> traceMarioAngle;
	/* 0x120 */ TParamRT<f32> mSLWavePhaseVelocity;
	/* 0x134 */ TParamRT<f32> mSLWaveAmplitudeMin;
	/* 0x148 */ TParamRT<f32> mSLWaveAmplitudeMaxLaunch;
	/* 0x15C */ TParamRT<f32> mSLWaveAmplitudeMax;
	/* 0x170 */ TParamRT<f32> mSLSwingPhaseVelocity;
	/* 0x184 */ TParamRT<f32> mSLSwingAmplitudeMin;
	/* 0x198 */ TParamRT<f32> mSLSwingAmplitudeMax;
	/* 0x1AC */ TParamRT<f32> mSLRoundAngleVelocity;
	/* 0x1C0 */ TParamRT<f32> mSLRoundDistance;
	/* 0x1D4 */ TParamRT<f32> mSLAcceleration;
	/* 0x1E8 */ TParamRT<f32> mSLRotationSpeed;
	/* 0x1FC */ TParamRT<f32> mSLSpeedMax;
	/* 0x210 */ TParamRT<f32> mSLKoopaJrSubmarineScale;
	/* 0x224 */ TParamRT<f32> mSLDamageRadius;
	/* 0x238 */ TParamRT<f32> mSLDamageHeight;
	/* 0x24C */ TParamRT<f32> shineKillerProbability0;
	/* 0x260 */ TParamRT<f32> shineKillerProbability1;
	/* 0x274 */ TParamRT<s32> mSLKillerIntervalFast;
	/* 0x288 */ TParamRT<s32> mSLKillerInterval;
};

class TKoopa : public TSpineEnemy {
public:
	TKoopa(const char*);

	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual void perform(u32, JDrama::TGraphics*);
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual BOOL updateAnmSound();
	virtual const char** getBasNameTable() const;
	virtual void reset();

	bool isFlaming() const;
	f32 getFlameDirDegree() const;
	BOOL allowsLaunch() const;
	BOOL effectsTumble() const;
	f32 getFlameDirRate() const;
	void changeAnm(int, int, f32);
	f32 getNeckFocus() const;
	f32 getTargetDir(const JGeometry::TVec3<f32>&) const;
	void setUpHitActors();
	void stagger(bool);
	BOOL getShowered();
	void getDown();
	void fall();

	TKoopaParams* getSaveParam2() const
	{
		return (TKoopaParams*)((TEnemyManager*)mManager)->getSaveParam();
	}

	/* 0x150 */ f32 unk150;
	/* 0x154 */ u8 unk154;
	/* 0x155 */ u8 unk155;
	/* 0x156 */ u8 unk156[2];
	/* 0x158 */ JGeometry::TVec3<f32> unk158;
	/* 0x164 */ TKoopaFlame* mFlameHitActors[10];
	/* 0x18C */ TKoopaHand* mHandHitActors[2];
	/* 0x194 */ TKoopaHead* mHeadHitActor;
	/* 0x198 */ TKoopaBody* mBodyHitActor;
	/* 0x19C */ s32 unk19C;
	/* 0x1A0 */ s32 mNeckJointIndex;
	/* 0x1A4 */ s32 mJointIndex2;
	/* 0x1A8 */ s32 mHeadJointIndex;
	/* 0x1AC */ u8 unk1AC[0x0C];
	/* 0x1B8 */ f32 unk1B8;
};

class TKoopaManager : public TEnemyManager {
public:
	TKoopaManager(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual void createModelData();
	virtual TSpineEnemy* createEnemyInstance();
};

class TKoopaJr : public TSpineEnemy {
public:
	TKoopaJr(const char*);

	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void perform(u32, JDrama::TGraphics*);
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual const char** getBasNameTable() const;
	virtual void reset();

	void checkNerveKillerHit();
	void checkNerveKillerLaunchFast();
	void checkNerveKillerLaunchNormal();

	TKoopaJrParams* getSaveParam2() const
	{
		return (TKoopaJrParams*)getSaveParam();
	}

	/* 0x150 */ s32 unk150;
	/* 0x154 */ s32 unk154;
	/* 0x158 */ s32 unk158;
	/* 0x15C */ TBathtub* unk15C;
	/* 0x160 */ TKoopa* unk160;
	/* 0x164 */ TKoopaJrSubmarine* unk164;
	/* 0x168 */ TEnemyManager* unk168;
	/* 0x16C */ TEnemyManager* unk16C;
};

class TKoopaJrManager : public TEnemyManager {
public:
	TKoopaJrManager(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual void createModelData();
	virtual TSpineEnemy* createEnemyInstance();
};

class TKoopaJrSubmarine : public TSpineEnemy {
public:
	TKoopaJrSubmarine(const char*);

	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void perform(u32, JDrama::TGraphics*);
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void bind();
	virtual const char** getBasNameTable() const;
	virtual void reset();

	void checkNerve();
	void makeRoundVelocity();
	void makeRelativeAngle();
	void makeKillerVelocity(TBathtubKiller*, JGeometry::TVec3<f32>);
	void launchKiller();
	bool appearShineKiller(int);
	void moveSwing();
	void makeCollisionPositions();
	void resetKoopaJrSubmarine();

	TKoopaJrSubmarineParams* getSaveParam2() const
	{
		return (TKoopaJrSubmarineParams*)getSaveParam();
	}

	/* 0x150 */ s32 unk150;
	/* 0x154 */ f32 unk154;
	/* 0x158 */ f32 unk158;
	/* 0x15C */ f32 unk15C;
	/* 0x160 */ f32 unk160;
	/* 0x164 */ TDirectionCalc unk164;
	/* 0x168 */ f32 unk168;
	/* 0x16C */ TDirectionCalc unk16C;
	/* 0x170 */ u8 unk170;
	/* 0x171 */ u8 unk171[3];
	/* 0x174 */ TBathtubBinder* unk174;
	/* 0x178 */ u8 unk178[8];
	/* 0x180 */ s32 unk180;
	/* 0x184 */ s32 unk184;
	/* 0x188 */ f32 unk188;
	/* 0x18C */ u8 unk18C;
	/* 0x18D */ u8 unk18D[3];
	/* 0x190 */ f32 unk190;
	/* 0x194 */ f32 unk194;
	/* 0x198 */ f32 unk198;
	/* 0x19C */ f32 unk19C;
	/* 0x1A0 */ TKoopaJr* unk1A0;
	/* 0x1A4 */ THitActor* unk1A4;
	/* 0x1A8 */ THitActor* unk1A8;
};

class TKoopaJrSubmarineManager : public TEnemyManager {
public:
	TKoopaJrSubmarineManager(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual void createModelData();
	virtual TSpineEnemy* createEnemyInstance();
};

class TCallbackHitActor : public THitActor {
public:
	TCallbackHitActor(const char* name, TKoopaJrSubmarine* owner)
	    : THitActor(name)
	    , unk68(owner)
	{
	}

	virtual BOOL receiveMessage(THitActor*, u32);

	/* 0x68 */ TKoopaJrSubmarine* unk68;
};

DECLARE_NERVE(TNerveKoopaJrWait, TLiveActor);
DECLARE_NERVE(TNerveKoopaJrDamage, TLiveActor);
DECLARE_NERVE(TNerveKoopaJrDemo, TLiveActor);
DECLARE_NERVE(TNerveKoopaJrLaunch, TLiveActor);
DECLARE_NERVE(TNerveKoopaJrYahoo, TLiveActor);
DECLARE_NERVE(TNerveKoopaJrSubmarineWait, TLiveActor);
DECLARE_NERVE(TNerveKoopaJrSubmarineCannonOpenClose, TLiveActor);
DECLARE_NERVE(TNerveKoopaJrSubmarineLaunchKiller, TLiveActor);

class TNerveKoopaTurn : public TNerveBase<TLiveActor> {
public:
	virtual BOOL execute(TSpineBase<TLiveActor>*) const = 0;
};

class TNerveKoopaWait : public TNerveKoopaTurn {
public:
	virtual BOOL execute(TSpineBase<TLiveActor>*) const;
	static const TNerveKoopaWait& theNerve();
};

class TNerveKoopaTumble : public TNerveKoopaTurn {
public:
	virtual BOOL execute(TSpineBase<TLiveActor>*) const;
	static const TNerveKoopaTumble& theNerve();
};

class TNerveKoopaFlame : public TNerveKoopaTurn {
public:
	virtual BOOL execute(TSpineBase<TLiveActor>*) const;
	static const TNerveKoopaFlame& theNerve();
};

class TNerveKoopaTurnL : public TNerveKoopaTurn {
public:
	virtual BOOL execute(TSpineBase<TLiveActor>*) const;
	static const TNerveKoopaTurnL& theNerve();
};

class TNerveKoopaTurnR : public TNerveKoopaTurn {
public:
	virtual BOOL execute(TSpineBase<TLiveActor>*) const;
	static const TNerveKoopaTurnR& theNerve();
};

DECLARE_NERVE(TNerveKoopaProvoke, TLiveActor);
DECLARE_NERVE(TNerveKoopaFall, TLiveActor);
DECLARE_NERVE(TNerveKoopaStagger, TLiveActor);
DECLARE_NERVE(TNerveKoopaGetDown, TLiveActor);
DECLARE_NERVE(TNerveKoopaGetShowered, TLiveActor);

#endif
