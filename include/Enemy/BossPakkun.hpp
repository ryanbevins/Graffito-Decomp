#ifndef ENEMY_BOSS_PAKKUN_HPP
#define ENEMY_BOSS_PAKKUN_HPP

#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <JSystem/JDrama/JDRViewObj.hpp>
#include <M3DUtil/M3UJoint.hpp>
#include <Strategic/Nerve.hpp>

class TLiveActor;
class MActor;
class TWaterEmitInfo;
class TAreaCylinderManager;

class TBossPakkunParams : public TSpineEnemyParams {
public:
	TBossPakkunParams(const char*);

	/* 0x0A8 */ TParamRT<s32> mSLWaitFrameStg0;
	/* 0x0BC */ TParamRT<s32> mSLWaterMarkLimit;
	/* 0x0D0 */ TParamRT<f32> mSLSwingLength;
	/* 0x0E4 */ TParamRT<f32> mSLPollBallStampScale;
	/* 0x0F8 */ TParamRT<s32> mSLTumbleTime;
	/* 0x10C */ TParamRT<s32> mSLAnmBlendTime0;
	/* 0x120 */ TParamRT<f32> mSLFlySpeed;
	/* 0x134 */ TParamRT<f32> mSLPivotSpeed;
	/* 0x148 */ TParamRT<f32> mSLPivotSpeedAware;
	/* 0x15C */ TParamRT<f32> mSLVomitAnmRate;
	/* 0x170 */ TParamRT<f32> mSLHeadHomingLimit;
	/* 0x184 */ TParamRT<f32> mSLDamageAngle;
	/* 0x198 */ TParamRT<f32> mSLTornadoProp;
	/* 0x1AC */ TParamRT<f32> mSLTornadoSpeed;
	/* 0x1C0 */ TParamRT<f32> mSLTornadoRollSpeed;
	/* 0x1D4 */ TParamRT<f32> mSLTornadoMoveInit;
	/* 0x1E8 */ TParamRT<f32> mSLTornadoMoveInc;
	/* 0x1FC */ TParamRT<f32> mSLTornadoMoveLimit;
	/* 0x210 */ TParamRT<s32> mSLWaterHitTimer;
	/* 0x224 */ TParamRT<s32> mSLHoverTimer;
	/* 0x238 */ TParamRT<f32> mSLPollBallRange;
	/* 0x24C */ TParamRT<f32> mSLPollBallSpeed;
	/* 0x260 */ TParamRT<f32> mSLPollBallFront;
};

class TBossPakkun;

class TBossPakkunMtxCalc : public M3UMtxCalcSIAnmBlendQuat {
public:
	TBossPakkunMtxCalc(TBossPakkun* owner)
	    : M3UMtxCalcSIAnmBlendQuat(true)
	    , mOwner(owner)
	{
	}

	virtual void calc(u16);

	void calcHeadDir(u16);
	void calcBellyScale(u16);

	/* 0x64 */ TBossPakkun* mOwner;
};

class TBPNavel : public THitActor {
public:
	TBPNavel(TBossPakkun* owner, const char* name)
	    : THitActor(name)
	    , mOwner(owner)
	{
	}

	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);

	/* 0x68 */ TBossPakkun* mOwner;
};

class TBPHeadHit : public THitActor {
public:
	TBPHeadHit(TBossPakkun* owner, const char* name)
	    : THitActor(name)
	    , mOwner(owner)
	{
	}

	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);

	void throwActor(THitActor*);

	/* 0x68 */ TBossPakkun* mOwner;
};

class TBPTornado : public THitActor {
public:
	TBPTornado(TBossPakkun*, const char*);

	virtual void perform(u32, JDrama::TGraphics*);

	/* 0x68 */ TBossPakkun* mOwner;
	/* 0x6C */ MActor* mActor;
	/* 0x70 */ JGeometry::TVec3<f32> unk70;
	/* 0x7C */ JGeometry::TVec3<f32> unk7C;
	/* 0x88 */ JGeometry::TVec3<f32> unk88;
	/* 0x94 */ f32 unk94;
	/* 0x98 */ s32 unk98;
};

class TBPVomit : public JDrama::TViewObj {
public:
	TBPVomit(TBossPakkun* owner, const char* name)
	    : JDrama::TViewObj(name)
	    , mOwner(owner)
	    , unk14(nullptr)
	    , unk18(nullptr)
	{
	}

	virtual void perform(u32, JDrama::TGraphics*);

	/* 0x10 */ TBossPakkun* mOwner;
	/* 0x14 */ MActor* unk14;
	/* 0x18 */ MActor* unk18;
};

class TBPPolDrop : public THitActor {
public:
	TBPPolDrop(TBossPakkun*, const char*);

	virtual void perform(u32, JDrama::TGraphics*);

	void move();

	/* 0x68 */ TBossPakkun* mOwner;
	/* 0x6C */ JGeometry::TVec3<f32> mVelocity;
	/* 0x78 */ MActor* unk78;
	/* 0x7C */ MActor* unk7C;
	/* 0x80 */ s32 unk80;
	/* 0x84 */ s32 unk84;
	/* 0x88 */ f32 unk88;
};

class TBossPakkun : public TSpineEnemy {
public:
	TBossPakkun(const char*);

	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void init(TLiveManager*);
	virtual void setGroundCollision();
	virtual void kill();
	virtual const char** getBasNameTable() const;

	void changeBck(int);
	void launchPolDrop();
	void gotHipDropDamage();
	void showMessage(u32);
	void rumblePad(int, const JGeometry::TVec3<f32>&);
	BOOL checkMarioRiding();

	TBossPakkunParams* getBossPakkunSaveParam() const
	{
		return (TBossPakkunParams*)getSaveParam();
	}

	/* 0x150 */ TBossPakkunMtxCalc* mMtxCalc;
	/* 0x154 */ f32 unk154;
	/* 0x158 */ TBPPolDrop* mPolDrop;
	/* 0x15C */ TBPVomit* mVomit;
	/* 0x160 */ TBPTornado* mTornado;
	/* 0x164 */ TBPHeadHit* mHeadHit;
	/* 0x168 */ TBPNavel* mNavel;
	/* 0x16C */ u8 unk16C;
	/* 0x16D */ u8 unk16D[0x3];
	/* 0x170 */ s32 unk170;
	/* 0x174 */ s32 unk174;
	/* 0x178 */ s32 unk178;
	/* 0x17C */ u8 unk17C;
	/* 0x17D */ u8 unk17D[0x3];
	/* 0x180 */ MActor* unk180;
	/* 0x184 */ f32 unk184;
	/* 0x188 */ TAreaCylinderManager* unk188;
	/* 0x18C */ TWaterEmitInfo* unk18C;
	/* 0x190 */ u8 unk190;
	/* 0x191 */ u8 unk191[0x3];
	/* 0x194 */ JGeometry::TVec3<f32> unk194;
	/* 0x1A0 */ JGeometry::TVec3<f32> unk1A0;
	/* 0x1AC */ JGeometry::TVec3<f32> unk1AC;
	/* 0x1B8 */ s32 unk1B8;
	/* 0x1BC */ u8 unk1BC;
	/* 0x1BD */ u8 unk1BD[0x3];
	/* 0x1C0 */ s32 unk1C0;
	/* 0x1C4 */ u8 unk1C4;
	/* 0x1C5 */ u8 unk1C5[0x3];
	/* 0x1C8 */ f32 unk1C8;
	/* 0x1CC */ u8 unk1CC;
};

class TBossPakkunManager : public TEnemyManager {
public:
	TBossPakkunManager(const char*, int);

	virtual void load(JSUMemoryInputStream&);
	virtual void createModelData();

	/* 0x54 */ s32 mIsLight;
};

DECLARE_NERVE(TNerveBPWait, TLiveActor);
DECLARE_NERVE(TNerveBPCannon, TLiveActor);
DECLARE_NERVE(TNerveBPVomit, TLiveActor);
DECLARE_NERVE(TNerveBPTornado, TLiveActor);
DECLARE_NERVE(TNerveBPPivot, TLiveActor);
DECLARE_NERVE(TNerveBPSwallow, TLiveActor);
DECLARE_NERVE(TNerveBPTumbleIn, TLiveActor);
DECLARE_NERVE(TNerveBPTumble, TLiveActor);
DECLARE_NERVE(TNerveBPTumbleOut, TLiveActor);
DECLARE_NERVE(TNerveBPGetUp, TLiveActor);
DECLARE_NERVE(TNerveBPSwing, TLiveActor);
DECLARE_NERVE(TNerveBPStompReact, TLiveActor);
DECLARE_NERVE(TNerveBPJumpReact, TLiveActor);
DECLARE_NERVE(TNerveBPPreDie, TLiveActor);
DECLARE_NERVE(TNerveBPDie, TLiveActor);
DECLARE_NERVE(TNerveBPTakeOff, TLiveActor);
DECLARE_NERVE(TNerveBPFly, TLiveActor);
DECLARE_NERVE(TNerveBPTouchDown, TLiveActor);
DECLARE_NERVE(TNerveBPFlyCannon, TLiveActor);
DECLARE_NERVE(TNerveBPFlyPivot, TLiveActor);
DECLARE_NERVE(TNerveBPHover, TLiveActor);
DECLARE_NERVE(TNerveBPFall, TLiveActor);
DECLARE_NERVE(TNerveBPSleep, TLiveActor);
DECLARE_NERVE(TNerveBPBreakSleep, TLiveActor);
DECLARE_NERVE(TNerveBPWaitL, TLiveActor);
DECLARE_NERVE(TNerveBPCannonL, TLiveActor);

#endif
