#ifndef ENEMY_KUKKU_HPP
#define ENEMY_KUKKU_HPP

#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Enemy/SmallEnemy.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/HitActor.hpp>
#include <Strategic/Nerve.hpp>

class MActor;

class TKukkuBall : public THitActor {
public:
	TKukkuBall(MActor*, const char* = "\x83\x4e\x83\x62\x83\x4e\x8b\xca");

	virtual void perform(u32, JDrama::TGraphics*);
	virtual void init();

public:
	/* 0x68 */ MActor* mMActor;
	/* 0x6C */ u32 mFlags;
	/* 0x70 */ JGeometry::TVec3<f32> mVelocity;
	/* 0x7C */ u32 mState;
};

class TKukkuParams : public TSmallEnemyParams {
public:
	TKukkuParams(const char*);

	/* 0x2D4 */ TParamRT<f32> mMarchSpeed;
	/* 0x2E8 */ TParamRT<f32> mTurnSpeed;
	/* 0x2FC */ TParamRT<f32> mWaterPowerY;
	/* 0x310 */ TParamRT<f32> mShootSpeed;
	/* 0x324 */ TParamRT<s32> mShootInterval;
	/* 0x338 */ TParamRT<f32> mSearchRange;
	/* 0x34C */ TParamRT<s32> mHabatakiTimer;
	/* 0x360 */ TParamRT<f32> mAirFric;
	/* 0x374 */ TParamRT<f32> mUpperVelocityY;
	/* 0x388 */ TParamRT<f32> mDropSpeed;
	/* 0x39C */ TParamRT<f32> mDropAngleX;
};

class TKukkuManager : public TSmallEnemyManager {
public:
	TKukkuManager(const char*);
	virtual void load(JSUMemoryInputStream&);
	virtual void createModelData();
};

class TKukku : public TSmallEnemy {
public:
	TKukku(const char* = "\x83\x68\x83\x68\x83\x8A\x83\x67\x83\x8A");

	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void perform(u32, JDrama::TGraphics*);
	virtual void control();
	virtual void bind();
	virtual const char** getBasNameTable() const;
	virtual void reset();
	virtual void setDeadAnm();
	virtual void setAfterDeadEffect();
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void behaveToWater(THitActor*);

	bool isFalling() const;
	void updateRotation();
	JGeometry::TVec3<f32> calcMomentum(f32);
	void dropCoins();

	// fabricated
	TKukkuBall** getKukkuBalls() { return mKukkuBalls; }
	s32 getUnk1A4() const { return unk1A4; }
	TKukkuParams* getSaveParam2() const { return (TKukkuParams*)getSaveParam(); }

public:
	/* 0x194 */ TKukkuBall* mKukkuBalls[3];
	/* 0x1A0 */ void* unk1A0;
	/* 0x1A4 */ s32 unk1A4;
	/* 0x1A8 */ s32 unk1A8;
	/* 0x1AC */ s32 unk1AC;
	/* 0x1B0 */ s32 unk1B0;
};

DECLARE_NERVE(TNerveKukkuFall, TLiveActor);
DECLARE_NERVE(TNerveKukkuPostFall, TLiveActor);
DECLARE_NERVE(TNerveKukkuRecoverGraph, TLiveActor);
DECLARE_NERVE(TNerveKukkuGraphWander, TLiveActor);

#endif
