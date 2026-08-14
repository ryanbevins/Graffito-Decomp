#ifndef ENEMY_LIMIT_KOOPA_HPP
#define ENEMY_LIMIT_KOOPA_HPP

#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Enemy/DirectionCalc.hpp>
#include <Strategic/Nerve.hpp>

class TLimitKoopaParams : public TSpineEnemyParams {
public:
	TLimitKoopaParams(const char*);

	/* 0xA8 */ TParamRT<f32> rotationSpeed;
	/* 0xBC */ TParamRT<f32> bodyScale;
	/* 0xD0 */ TParamRT<f32> hipDropInitialSpeedY;
	/* 0xE4 */ TParamRT<f32> hipDropGravityY;
	/* 0xF8 */ TParamRT<f32> turnSpeed;
	/* 0x10C */ TParamRT<f32> turnAnim;
	/* 0x120 */ TParamRT<f32> waitStep;
	/* 0x134 */ TParamRT<f32> attackRadius;
	/* 0x148 */ TParamRT<f32> attackHeight;
	/* 0x15C */ TParamRT<f32> focusRange;
	/* 0x170 */ TParamRT<f32> waitRange;
	/* 0x184 */ TParamRT<f32> fireSpeed;
	/* 0x198 */ TParamRT<f32> tumbleSpeed;
	/* 0x1AC */ TParamRT<f32> waitSpeed;
	/* 0x1C0 */ TParamRT<f32> staggerSpeed;
	/* 0x1D4 */ TParamRT<f32> downSpeed;
	/* 0x1E8 */ TParamRT<f32> tumbleWeight;
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
	/* 0x2D8 */ TParamRT<f32> marioEstimationFire;
	/* 0x2EC */ TParamRT<f32> marioEstimationWait;
};

class TLimitKoopa;

// TLimitKoopaParts : TLiveActor, size 0xF8. Adds two virtuals at the end of the
// vtable: perform (overriding TLiveActor::perform position) and attack_ (new).
class TLimitKoopaParts : public TLiveActor {
public:
	TLimitKoopaParts(const char*);

	virtual void perform(u32, JDrama::TGraphics*);
	virtual void attack_(THitActor*) = 0;

	/* 0xF4 */ TLimitKoopa* mOwner;
};

class TLimitKoopaBody : public TLimitKoopaParts {
public:
	TLimitKoopaBody(const char*);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void attack_(THitActor*);
};

class TLimitKoopaHead : public TLimitKoopaParts {
public:
	TLimitKoopaHead(const char*);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void attack_(THitActor*);
};

class TLimitKoopaHand : public TLimitKoopaParts {
public:
	TLimitKoopaHand(const char*);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void attack_(THitActor*);
};

class TLimitKoopaFlame : public TLimitKoopaParts {
public:
	TLimitKoopaFlame(const char*);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void attack_(THitActor*);
};

class TLimitKoopa : public TSpineEnemy {
public:
	TLimitKoopa(const char* = "\x83\x4A\x83\x5A\x83\x4E\x83\x93\x83\x7D\x83\x6C\x81\x5B\x83\x57\x83\x83\x81\x5B");

	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual void perform(u32, JDrama::TGraphics*);
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void bind();
	virtual f32 getGravityY() const;
	virtual void reset();

	void setUpHitActors();
	void startHipDrop();
	void registerToGroup(THitActor*);

	TLimitKoopaParams* getSaveParam2() const
	{
		return (TLimitKoopaParams*)((TEnemyManager*)mManager)->getSaveParam();
	}

	/* 0x150 */ s32 unk150;
	/* 0x154 */ s32 unk154;
	/* 0x158 */ s32 unk158;
	/* 0x15C */ JGeometry::TVec3<f32> mFallVelocity;
	/* 0x168 */ u8 unk168;
	/* 0x16C */ TDirectionCalc mDirection;
	/* 0x170 */ f32 unk170;
	/* 0x174 */ s32 unk174;
	/* 0x178 */ THitActor* mFlameHitActors[10];
	/* 0x1A0 */ THitActor* unk1A0;
	/* 0x1A4 */ THitActor* unk1A4;
	/* 0x1A8 */ THitActor* mHeadHitActor;
	/* 0x1AC */ THitActor* unk1AC;
	/* 0x1B0 */ s32 mNeckJointIndex;
	/* 0x1B4 */ s32 mJointIndex2;
	/* 0x1B8 */ s32 mHeadJointIndex;
	/* 0x1BC */ JGeometry::TVec3<f32> unk1BC;
};

class TLimitKoopaManager : public TEnemyManager {
public:
	TLimitKoopaManager(const char* = "\x83\x4A\x83\x5A\x83\x4E\x83\x93\x83\x7D\x83\x6C\x81\x5B\x83\x57\x83\x83\x81\x5B");

	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual void createModelData();
	virtual TSpineEnemy* createEnemyInstance();
};

// Abstract intermediate nerve: execute stays pure so its vtable execute slot is
// NULL; the vtable + dtor are emitted (weak) only because Wait/Tumble derive
// from it. This reproduces __vt__20TNerveLimitKoopaTurn (NULL execute) and the
// double vtable-write seen in the derived nerves' ctor chains.
class TNerveLimitKoopaTurn : public TNerveBase<TLiveActor> {
public:
	virtual BOOL execute(TSpineBase<TLiveActor>*) const = 0;
};

class TNerveLimitKoopaWait : public TNerveLimitKoopaTurn {
public:
	virtual BOOL execute(TSpineBase<TLiveActor>*) const;
	static const TNerveLimitKoopaWait& theNerve();
};

class TNerveLimitKoopaTumble : public TNerveLimitKoopaTurn {
public:
	virtual BOOL execute(TSpineBase<TLiveActor>*) const;
	static const TNerveLimitKoopaTumble& theNerve();
};

DECLARE_NERVE(TNerveLimitKoopaStagger, TLiveActor);
DECLARE_NERVE(TNerveLimitKoopaGetShowered, TLiveActor);
DECLARE_NERVE(TNerveLimitKoopaGetDown, TLiveActor);
DECLARE_NERVE(TNerveLimitKoopaHipDropStart, TLiveActor);
DECLARE_NERVE(TNerveLimitKoopaHipDropJump, TLiveActor);

#endif
