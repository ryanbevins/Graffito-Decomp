#ifndef ENEMY_BOSS_WANWAN_HPP
#define ENEMY_BOSS_WANWAN_HPP

#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <JSystem/JDrama/JDRViewObj.hpp>
#include <M3DUtil/M3UJoint.hpp>
#include <Strategic/Binder.hpp>
#include <Strategic/Nerve.hpp>
#include <dolphin/mtx.h>

class TLiveActor;
class MActor;
class TBossWanwan;
class TBWLeash;
class TRope;

class TBWParams : public TSpineEnemyParams {
public:
	TBWParams(const char*);

	/* 0x0A8 */ TParamRT<f32> mSLMarchSpeed;
	/* 0x0BC */ TParamRT<f32> mSLTurnSpeed;
	/* 0x0D0 */ TParamRT<f32> mSLLeashNodeLen;
	/* 0x0E4 */ TParamRT<f32> mSLPicketHeight;
	/* 0x0F8 */ TParamRT<f32> mSLPicketRadius;
	/* 0x10C */ TParamRT<f32> mSLChainHitHeight;
	/* 0x120 */ TParamRT<f32> mSLChainHitRadius;
	/* 0x134 */ TParamRT<f32> mSLChainGroundRadius;
	/* 0x148 */ TParamRT<f32> mSLPullLimit;
	/* 0x15C */ TParamRT<f32> mSLAttackSpeed;
	/* 0x170 */ TParamRT<s32> mSLStunTimer;
	/* 0x184 */ TParamRT<f32> mSLSearchLength;
	/* 0x198 */ TParamRT<f32> mSLSearchAngle;
	/* 0x1AC */ TParamRT<u8> mSLBWHitPointMax;
	/* 0x1C0 */ TParamRT<f32> mSLHeadGap;
	/* 0x1D4 */ TParamRT<f32> mSLShakeLengthMax;
	/* 0x1E8 */ TParamRT<f32> mSLShakeLengthMaxHP0;
};

class TBossWanwanMtxCalc : public M3UMtxCalcSIAnmBlendQuat {
public:
	TBossWanwanMtxCalc(TBossWanwan* owner)
	    : M3UMtxCalcSIAnmBlendQuat(false)
	    , mOwner(owner)
	{
	}

	virtual void calc(u16);

	/* 0x64 */ TBossWanwan* mOwner;
};

class TBWBinder : public TBinder {
public:
	TBWBinder() { }
	virtual void bind(TLiveActor*);
};

class TBWHit : public THitActor {
public:
	TBWHit(TBossWanwan* owner, int joint_index, const char* name)
	    : THitActor(name)
	    , mOwner(owner)
	    , mJointIndex(joint_index)
	{
	}

	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);

	/* 0x68 */ TBossWanwan* mOwner;
	/* 0x6C */ s32 mJointIndex;
};

class TBWPicket : public TTakeActor {
public:
	TBWPicket(TBossWanwan* owner, const char* name)
	    : TTakeActor(name)
	    , mOwner(owner)
	    , unk74()
	    , mMActor(nullptr)
	{
		PSMTXIdentity(unk74);
	}

	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual MtxPtr getTakingMtx();
	virtual BOOL moveRequest(const JGeometry::TVec3<f32>&);

	/* 0x70 */ TBossWanwan* mOwner;
	/* 0x74 */ TPosition3f unk74;
	/* 0xA4 */ MActor* mMActor;
};

class TBWLeashNode : public THitActor {
public:
	TBWLeashNode(TBWLeash* leash, int index, const char* name)
	    : THitActor(name)
	    , mLeash(leash)
	    , mMActor(nullptr)
	    , unk74(0.0f)
	    , mIndex(index)
	{
	}

	virtual void perform(u32, JDrama::TGraphics*);

	void calcMatrix();
	void calcTemperature();

	/* 0x68 */ TBWLeash* mLeash;
	/* 0x6C */ MActor* mMActor;
	/* 0x70 */ f32 unk74;
	/* 0x74 */ s32 mIndex;
};

class TBWLeash : public JDrama::TViewObj {
public:
	TBWLeash(TBossWanwan*, int, const char*);

	virtual void perform(u32, JDrama::TGraphics*);

	/* 0x10 */ TBossWanwan* mOwner;
	/* 0x14 */ TRope* mRope;
	/* 0x18 */ TBWLeashNode** mNodes;
};

class TBossWanwan : public TSpineEnemy {
public:
	TBossWanwan(const char*);

	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void control();
	virtual void kill();

	void emitEffects();
	void slideToCurPathNode(f32, f32);
	void shakeCamera(int);

	/* 0x150 */ TBossWanwanMtxCalc* mMtxCalc;
	/* 0x154 */ TBWLeash* mLeash;
	/* 0x158 */ TBWPicket* mPicket;
	/* 0x15C */ JGeometry::TVec3<f32> unk15C;
	/* 0x168 */ f32 unk168;
	/* 0x16C */ s32 unk16C;
	/* 0x170 */ TBWHit* mHeadHit;
	/* 0x174 */ TBWHit* mBodyHit;
	/* 0x178 */ f32 unk178;
	/* 0x17C */ s32 unk17C;
	/* 0x180 */ s32 unk180;
	/* 0x184 */ s32 unk184;
	/* 0x188 */ s32 unk188;
	/* 0x18C */ s8 unk18C;
	/* 0x18D */ u8 unk18D;
	/* 0x18E */ u8 unk18E[2];
	/* 0x190 */ s32 unk190;
	/* 0x194 */ u8 unk194;
	/* 0x195 */ u8 unk195;
	/* 0x196 */ u8 unk196[2];
	/* 0x198 */ s32 unk198;
	/* 0x19C */ s32 unk19C;
	/* 0x1A0 */ s8 unk1A0;
	/* 0x1A1 */ u8 unk1A1[3];
	/* 0x1A4 */ JGeometry::TVec3<f32> unk1A4;
	/* 0x1B0 */ s32 unk1B0;
	/* 0x1B4 */ u16 unk1B4;
	/* 0x1B6 */ u8 unk1B6[2];
};

class TBossWanwanManager : public TEnemyManager {
public:
	TBossWanwanManager(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void createModelData();
	virtual TSpineEnemy* createEnemyInstance();
};

DECLARE_NERVE(TNerveBWGraphWander, TLiveActor);
DECLARE_NERVE(TNerveBWRoll, TLiveActor);
DECLARE_NERVE(TNerveBWBark, TLiveActor);
DECLARE_NERVE(TNerveBWJump, TLiveActor);
DECLARE_NERVE(TNerveBWStun, TLiveActor);
DECLARE_NERVE(TNerveBWWakeup, TLiveActor);
DECLARE_NERVE(TNerveBWJumpToBath, TLiveActor);
DECLARE_NERVE(TNerveBWDie, TLiveActor);
DECLARE_NERVE(TNerveBWJumpAway, TLiveActor);
DECLARE_NERVE(TNerveBWShake, TLiveActor);
DECLARE_NERVE(TNerveBWFall, TLiveActor);

#endif
