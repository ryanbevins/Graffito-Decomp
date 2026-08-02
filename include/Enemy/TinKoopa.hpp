#ifndef ENEMY_TIN_KOOPA_HPP
#define ENEMY_TIN_KOOPA_HPP

#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <M3DUtil/M3UJoint.hpp>
#include <Strategic/Nerve.hpp>

class TLiveManager;
class TMapCollisionMove;
class TTinKoopaPartsBase;
class TTinKoopaFlame;
class TTinKoopa;
class TGraphWeb;

DECLARE_NERVE(TNerveTinKoopaWait, TLiveActor);
DECLARE_NERVE(TNerveTinKoopaDamage, TLiveActor);
DECLARE_NERVE(TNerveTinKoopaBreak, TLiveActor);

class TTinKoopaLaunchOrder {
public:
	TTinKoopaLaunchOrder(TTinKoopa*, s8, s32, s8, s8);

	void checkOrder();

	/* 0x0 */ TTinKoopa* unk0;
	/* 0x4 */ s8 unk4;
	/* 0x5 */ u8 unk5[3];
	/* 0x8 */ s32 unk8;
	/* 0xC */ s8 unkC;
	/* 0xD */ s8 unkD;
	/* 0xE */ u8 unkE[2];
};

class TTinKoopaLaunchOrderTable {
public:
	/* 0x0 */ u8 unk0;
	/* 0x1 */ u8 unk1[3];
	/* 0x4 */ TTinKoopa* unk4;
	/* 0x8 */ TTinKoopaLaunchOrder** unk8;
};

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
	TTinKoopaPartsBase(const char*, s32, TTinKoopa*);

	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void reset();

	void emitPartsDisappearEffects(const char**, int, f32);
	void emitPartsDisappearEffects();
	void emitPartsTrackEffects(const char**, int);
	void startBreaking();
	void initTinKoopaPartsBase();

	/* 0xF4 */ TMapCollisionMove* unkF4;
	/* 0xF8 */ u8 unkF8;
	/* 0xF9 */ u8 unkF9[3];
	/* 0xFC */ s32 unkFC;
	/* 0x100 */ TTinKoopa* unk100;
	/* 0x104 */ MActor* unk104;
	/* 0x108 */ JGeometry::TVec3<f32> unk108[6];
};

class TTinKoopaFlame : public THitActor {
public:
	TTinKoopaFlame(const char*);

	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);

	void emitFlameEffects();

	/* 0x68 */ TTinKoopa* unk68;
	/* 0x6C */ f32 unk6C;
	/* 0x70 */ s16 unk70;
	/* 0x72 */ u8 unk72;
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

	/* 0x150 */ s32 unk150;
	/* 0x154 */ s32 unk154;
	/* 0x158 */ s32 unk158;
	/* 0x15C */ s32 unk15C;
	/* 0x160 */ TTinKoopaFlame* unk160;
	/* 0x164 */ MActor* unk164;
	/* 0x168 */ u8 unk168;
	/* 0x169 */ s8 unk169[4];
	/* 0x16D */ u8 unk16D[3];
	/* 0x170 */ s32 unk170;
	/* 0x174 */ s32 unk174;
	/* 0x178 */ s32 unk178;
	/* 0x17C */ s32 unk17C;
	/* 0x180 */ s32 unk180;
	/* 0x184 */ u8 unk184[0x30];
	/* 0x1B4 */ f32 unk1B4;
	/* 0x1B8 */ f32 unk1B8;
	/* 0x1BC */ f32 unk1BC;
	/* 0x1C0 */ f32 unk1C0;
	/* 0x1C4 */ u8 unk1C4[4];
	/* 0x1C8 */ s32 unk1C8;
	/* 0x1CC */ TTinKoopaPartsBase* unk1CC[6];
	/* 0x1E4 */ TTinKoopaPartsBase* unk1E4;
	/* 0x1E8 */ f32* unk1E8;
	/* 0x1EC */ TGraphWeb* unk1EC;
	/* 0x1F0 */ TEnemyManager* unk1F0;
	/* 0x1F4 */ TTinKoopaLaunchOrderTable* unk1F4;
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
