#ifndef ENEMY_TIN_KOOPA_HPP
#define ENEMY_TIN_KOOPA_HPP

#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <M3DUtil/M3UJoint.hpp>

class TLiveManager;
class TTinKoopaPartsBase;
class TTinKoopaFlame;

class TTinKoopaParams : public TSpineEnemyParams {
public:
	TTinKoopaParams(const char*);

	/* 0xA8 */ TParamRT<s32> mSLPartsHP;
	/* 0xBC */ TParamRT<s32> mSLFlameHP;
	/* 0xD0 */ TParamRT<s32> mSLFlameRevivalTime;
	/* 0xE4 */ TParamRT<f32> mSLFlameDamageRadius0;
	/* 0xF8 */ TParamRT<f32> mSLFlameDamageHeight0;
	/* 0x10C */ TParamRT<f32> mSLFlameDamageRadius1;
	/* 0x120 */ TParamRT<f32> mSLFlameDamageHeight1;
	/* 0x134 */ TParamRT<f32> mSLDamageRadius;
	/* 0x148 */ TParamRT<f32> mSLDamageHeight0;
	/* 0x15C */ TParamRT<f32> mSLDamageHeight1;
	/* 0x170 */ TParamRT<s32> mSLKillerInterval;
	/* 0x184 */ TParamRT<s32> mSLDefeatWaitTime;
	/* 0x198 */ TParamRT<f32> mSLKillerApproachingDistance;
};

class TTinKoopaMtxCalc : public M3UMtxCalcSIAnmBlendQuat {
public:
	TTinKoopaMtxCalc()
	    : M3UMtxCalcSIAnmBlendQuat(true)
	{
	}

	virtual void calc(u16);

	/* 0x64 */ void* unk64;
};

class TTinKoopaPartsBase : public TLiveActor {
public:
	TTinKoopaPartsBase(const char*);

	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void reset();

	/* 0xF4 */ u8 unkF4[0x5C];
};

class TTinKoopaFlame : public THitActor {
public:
	TTinKoopaFlame(const char*);

	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);

	/* 0x68 */ u8 unk68[0x14];
};

class TTinKoopa : public TSpineEnemy {
public:
	TTinKoopa(const char*);

	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void perform(u32, JDrama::TGraphics*);
	virtual void init(TLiveManager*);
	virtual BOOL hasMapCollision() const;
	virtual const char** getBasNameTable() const;
	virtual void reset();

	void emitTinKoopaEffects();
	void checkTinKoopaFirstFlameMessage();
	void checkTinKoopaKillerApproachingMessage();
	void launchKiller(int);
	void hitParts();
	void resetTinKoopa();
	f32 calcCoasterDistance(int, int);
	void makeLaunchSchedule();

	/* 0x150 */ u8 unk150[0xA0];
	/* 0x1F0 */ TEnemyManager* unk1F0;
	/* 0x1F4 */ void* unk1F4;
	/* 0x1F8 */ void* unk1F8;
};

class TTinKoopaManager : public TEnemyManager {
public:
	TTinKoopaManager(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual void createModelData();
	virtual BOOL hasMapCollision() const;
	virtual TSpineEnemy* createEnemyInstance();
};

#endif
