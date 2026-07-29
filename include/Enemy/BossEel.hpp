#ifndef ENEMY_BOSS_EEL_HPP
#define ENEMY_BOSS_EEL_HPP

#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <Strategic/SharedParts.hpp>
#include <Strategic/Nerve.hpp>
#include <dolphin/gx/GXStruct.h>

class TLiveActor;
class TBEelTearsDrop;
class TBossEelEye;
class TBossEelTooth;
class TBossEelVortex;
class SDLModel;
class SDLModelData;
class TSharedParts;
class TBossEelSaveParams;
class TCoin;
class TCubeManagerBase;
class TMapCollisionMove;

struct TBossEelUnk1EC {
	s32 unk0;
	s32 unk4;
};

class TBossEel : public TSpineEnemy {
public:
	TBossEel(const char*);
	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual MtxPtr getTakingMtx();
	virtual void init(TLiveManager*);
	virtual BOOL hasMapCollision() const;
	virtual const char** getBasNameTable() const;

	bool isInBossEelMoguDemo();
	void collideToMario();
	void forceShedTears(bool);
	void shedTears(MtxPtr);
	void updateTearsCnt();

	static f32 mOpenRollSpeed;
	static u8 mUseObjCollision;
	static f32 mForcePow;
	static u8 mUseMapCollision;

	/* 0x150 */ JGeometry::TVec3<f32> unk150;
	/* 0x15C */ TBossEelEye* unk15C[4];
	/* 0x16C */ TBossEelTooth* unk16C[8];
	/* 0x18C */ TBossEelVortex* unk18C;
	/* 0x190 */ TMapCollisionMove* unk190[4];
	/* 0x1A0 */ u16 unk1A0[4];
	/* 0x1A8 */ void* unk1A8;
	/* 0x1AC */ TCubeManagerBase* unk1AC;
	/* 0x1B0 */ void* unk1B0;
	/* 0x1B4 */ s32 unk1B4;
	/* 0x1B8 */ s32 unk1B8;
	/* 0x1BC */ f32 unk1BC;
	/* 0x1C0 */ s32 unk1C0;
	/* 0x1C4 */ s32 unk1C4;
	/* 0x1C8 */ u8 unk1C8;
	/* 0x1C9 */ u8 unk1C9[0x3];
	/* 0x1CC */ f32 unk1CC;
	/* 0x1D0 */ u8 unk1D0;
	/* 0x1D1 */ u8 unk1D1[0x3];
	/* 0x1D4 */ f32 unk1D4;
	/* 0x1D8 */ f32 unk1D8;
	/* 0x1DC */ JGeometry::TVec3<f32> unk1DC;
	/* 0x1E8 */ TBossEelSaveParams* unk1E8;
	/* 0x1EC */ TBossEelUnk1EC* unk1EC;
	/* 0x1F0 */ u8 unk1F0;
	/* 0x1F1 */ u8 unk1F1[0x3];
	/* 0x1F4 */ f32 unk1F4;
	/* 0x1F8 */ f32 unk1F8;
	/* 0x1FC */ u8 unk1FC;
	/* 0x1FD */ u8 unk1FD;
	/* 0x1FE */ u8 unk1FE;
	/* 0x1FF */ u8 unk1FF;
	/* 0x200 */ s32 unk200;
	/* 0x204 */ JGeometry::TVec3<f32> unk204;
	/* 0x210 */ void* unk210;
	/* 0x214 */ void* unk214;
	/* 0x218 */ void* unk218;
	/* 0x21C */ u8 unk21C;
	/* 0x21D */ u8 unk21D;
	/* 0x21E */ u8 unk21E[0x2];
};

class TBossEelSaveParams : public TParams {
public:
	TBossEelSaveParams();

	/* 0x008 */ TParamRT<f32> mSLInitTransYOffset;
	/* 0x01C */ TParamRT<f32> mSLAppearMoveDistY;
	/* 0x030 */ TParamRT<f32> mSLBodyScale;
	/* 0x044 */ TParamRT<f32> mSLViewClipFar;
	/* 0x058 */ TParamRT<f32> mSLViewClipRadius;
	/* 0x06C */ TParamRT<f32> mSLBodyToHeadDistance;
	/* 0x080 */ TParamRT<f32> mSLBodyAttackRadius;
	/* 0x094 */ TParamRT<f32> mSLBodyAttackHeight;
	/* 0x0A8 */ TParamRT<f32> mSLBodyDamageRadius;
	/* 0x0BC */ TParamRT<f32> mSLBodyDamageHeight;
	/* 0x0D0 */ TParamRT<f32> mSLHeadAttackRadius;
	/* 0x0E4 */ TParamRT<f32> mSLHeadAttackHeight;
	/* 0x0F8 */ TParamRT<f32> mSLHeadDamageRadius;
	/* 0x10C */ TParamRT<f32> mSLHeadDamageHeight;
	/* 0x120 */ TParamRT<f32> mSLToothAttackRadius;
	/* 0x134 */ TParamRT<f32> mSLToothAttackHeight;
	/* 0x148 */ TParamRT<f32> mSLToothDamageRadius;
	/* 0x15C */ TParamRT<f32> mSLToothDamageHeight;
	/* 0x170 */ TParamRT<f32> mSLSpinAccel;
	/* 0x184 */ TParamRT<f32> mSLSpinMaxSpeed;
	/* 0x198 */ TParamRT<f32> mSLToothUpSpeed;
	/* 0x1AC */ TParamRT<f32> mSLToothLiveHeight;
	/* 0x1C0 */ TParamRT<s32> mSLToothMaxHitPoint;
	/* 0x1D4 */ TParamRT<s32> mSLGenTearsTime;
	/* 0x1E8 */ TParamRT<f32> mSLVortexAttackRadius;
	/* 0x1FC */ TParamRT<f32> mSLVortexAttackHeight;
	/* 0x210 */ TParamRT<f32> mSLVortexDamageRadius;
	/* 0x224 */ TParamRT<f32> mSLVortexDamageHeight;
	/* 0x238 */ TParamRT<s32> mSLVortexLiveTimer;
	/* 0x24C */ TParamRT<f32> mSLVortexScaleXZ;
	/* 0x260 */ TParamRT<f32> mSLVortexScaleY;
	/* 0x274 */ TParamRT<s32> mSLMouthOpenFrame;
	/* 0x288 */ TParamRT<s32> mSLMouthOpenInterval;
	/* 0x29C */ TParamRT<s32> mSLCanEatFrame;
	/* 0x2B0 */ TParamRT<f32> mSLBreathInPower;
};

class TBossEelManager : public TEnemyManager {
public:
	TBossEelManager(const char* name)
	    : TEnemyManager(name)
	    , mSaveParams()
	{
	}

	virtual void loadAfter();
	virtual void createModelData();
	virtual void clipEnemies(JDrama::TGraphics*);

	/* 0x054 */ TBossEelSaveParams mSaveParams;
};

class TBossEelCollision : public THitActor {
public:
	TBossEelCollision(MtxPtr mtx, const char* name)
	    : THitActor(name)
	    , unk68(mtx)
	    , unk6C(0.0f)
	    , unk70(0.0f)
	    , unk74(0.0f)
	    , unk78(0.0f)
	    , unk7C(nullptr)
	{
	}

	virtual void perform(u32, JDrama::TGraphics*);
	virtual void initCollision();
	virtual void behaveToMario();

	/* 0x68 */ MtxPtr unk68;
	/* 0x6C */ f32 unk6C;
	/* 0x70 */ f32 unk70;
	/* 0x74 */ f32 unk74;
	/* 0x78 */ f32 unk78;
	/* 0x7C */ TBossEel* unk7C;
};

class TBossEelTearsRecoverCollision : public TBossEelCollision {
public:
	TBossEelTearsRecoverCollision(MtxPtr mtx, const char* name)
	    : TBossEelCollision(mtx, name)
	    , unk80(FALSE)
	    , unk81(FALSE)
	{
	}

	virtual void perform(u32, JDrama::TGraphics*);
	virtual void initCollision();
	virtual void behaveToMario();

	/* 0x80 */ u8 unk80;
	/* 0x81 */ u8 unk81;
};

class TBossEelBarrierCollision : public TBossEelCollision {
public:
	TBossEelBarrierCollision(MtxPtr mtx, const char* name)
	    : TBossEelCollision(mtx, name)
	{
	}

	virtual void initCollision();
	virtual void behaveToMario();
};

class TBossEelAwaCollision : public TBossEelCollision {
public:
	TBossEelAwaCollision(MtxPtr mtx, const char* name)
	    : TBossEelCollision(mtx, name)
	    , unk80(0)
	    , unk84(0)
	{
	}

	virtual void perform(u32, JDrama::TGraphics*);
	virtual void initCollision();
	virtual void behaveToMario();

	/* 0x80 */ u32 unk80;
	/* 0x84 */ u32 unk84;
};

class TBossEelBodyCollision : public TBossEelCollision {
public:
	TBossEelBodyCollision(MtxPtr mtx, const char* name)
	    : TBossEelCollision(mtx, name)
	{
	}

	virtual void initCollision();
};

class TBossEelVortex : public THitActor {
public:
	TBossEelVortex(TBossEel* eel, const char* name)
	    : THitActor(name)
	    , unk68(eel)
	    , unk6C(FALSE)
	    , unk70(0)
	{
	}

	virtual void perform(u32, JDrama::TGraphics*);
	virtual void reset();

	/* 0x68 */ TBossEel* unk68;
	/* 0x6C */ u8 unk6C;
	/* 0x6D */ u8 unk6D[0x3];
	/* 0x70 */ s32 unk70;
};

class TBossEelHeartCoin : public TSharedParts {
public:
	TBossEelHeartCoin(TBossEel*, int, SDLModelData*, u32, const char*);
	virtual void perform(u32, JDrama::TGraphics*);
	void generate(JGeometry::TVec3<f32>&);

	/* 0x1C */ u8 unk1C;
	/* 0x1D */ u8 unk1D[0x3];
	/* 0x20 */ TCoin* mCoins[20];
	/* 0x70 */ JGeometry::TVec3<f32> unk70;
	/* 0x7C */ TBossEel* unk7C;
};

class TBossEelEye : public TSharedParts {
public:
	TBossEelEye(const TLiveActor*, int, SDLModelData*, u32, const char*);
	virtual void perform(u32, JDrama::TGraphics*);

	/* 0x1C */ Mtx unk1C;
	/* 0x4C */ SDLModel* unk4C;
	/* 0x50 */ s32 unk50;
	/* 0x54 */ u16 unk54;
	/* 0x56 */ u16 unk56;
	/* 0x58 */ u16 unk58;
	/* 0x5A */ u16 unk5A;
	/* 0x5C */ s32 unk5C;
	/* 0x60 */ s32 unk60;
	/* 0x64 */ f32 unk64;
	/* 0x68 */ TBossEelEye* unk68;
	/* 0x6C */ s32 unk6C;
	/* 0x70 */ JGeometry::TVec3<f32> unk70;
};

class TBossEelTooth : public THitActor {
public:
	TBossEelTooth(u8, TBossEel*, const char*, SDLModelData*, const char*);
	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);

	/* 0x68 */ TSharedParts* unk68;
	/* 0x6C */ TBossEel* unk6C;
	/* 0x70 */ s32 unk70;
	/* 0x74 */ u8 unk74;
	/* 0x75 */ u8 unk75[0x3];
	/* 0x78 */ f32 unk78;
	/* 0x7C */ f32 unk7C;
	/* 0x80 */ f32 unk80;
	/* 0x84 */ s32 unk84;
	/* 0x88 */ Mtx unk88;
	/* 0xB8 */ GXColor unkB8;
	/* 0xBC */ u8 unkBC;
	/* 0xBD */ u8 unkBD[0x3];
};

class TBEelTearsSaveLoadParams : public TSpineEnemyParams {
public:
	TBEelTearsSaveLoadParams(const char*);

	/* 0x0A8 */ TParamRT<f32> mSLTearsUpSpeed;
	/* 0x0BC */ TParamRT<f32> mSLTearsDamageUpSpeed;
	/* 0x0D0 */ TParamRT<f32> mSLTearsLiveHeight;
	/* 0x0E4 */ TParamRT<s32> mSLTearsSplitNum;
	/* 0x0F8 */ TParamRT<s32> mSLTearsDamageRadius;
	/* 0x10C */ TParamRT<s32> mSLTearsDamageHeight;
	/* 0x120 */ TParamRT<s32> mSLTearsAttackRadius;
	/* 0x134 */ TParamRT<s32> mSLTearsAttackHeight;
	/* 0x148 */ TParamRT<s32> mSLTearsDropDamageRadius;
	/* 0x15C */ TParamRT<s32> mSLTearsDropDamageHeight;
	/* 0x170 */ TParamRT<s32> mSLTearsDropAttackRadius;
	/* 0x184 */ TParamRT<s32> mSLTearsDropAttackHeight;
	/* 0x198 */ TParamRT<f32> mSLHighPolyDistY;
	/* 0x1AC */ TParamRT<f32> mSLHitAnmFrameRate;
	/* 0x1C0 */ TParamRT<f32> mSLBodyScaleLow;
	/* 0x1D4 */ TParamRT<f32> mSLBodyScaleHigh;
	/* 0x1E8 */ TParamRT<f32> mSLTearsDropScaleLow;
	/* 0x1FC */ TParamRT<f32> mSLTearsDropScaleHigh;
	/* 0x210 */ TMsRange<f32> mBodyScaleRange;
	/* 0x218 */ TMsRange<f32> mTearsDropScaleRange;
};

class TBEelTears : public TSpineEnemy {
public:
	TBEelTears(const char*);
	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void moveObject();
	virtual void kill();
	virtual const char** getBasNameTable() const;
	virtual void reset();
	virtual void setMActorAndKeeper();

	/* 0x150 */ JGeometry::TVec3<f32> unk150;
	/* 0x15C */ TBEelTearsSaveLoadParams* unk15C;
	/* 0x160 */ u8 unk160;
	/* 0x161 */ u8 unk161[0x3];
	/* 0x164 */ s32 unk164;
	/* 0x168 */ MtxPtr unk168;
	/* 0x16C */ TBossEelTearsRecoverCollision* unk16C;
};

class TBEelTearsManager : public TEnemyManager {
public:
	TBEelTearsManager(const char*);
	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*);
	virtual TSpineEnemy* createEnemyInstance();
	virtual void createEnemies(int);
	virtual void createModelData();
	virtual void loadAfter();
	void splitTears(JGeometry::TVec3<f32>&);

	/* 0x054 */ TBEelTearsDrop* mDrops[30];
};

class TBEelTearsDrop : public THitActor {
public:
	TBEelTearsDrop(TBEelTears*, int, SDLModelData*, const char*);
	virtual void perform(u32, JDrama::TGraphics*);

	/* 0x068 */ TSharedParts* unk68;
	/* 0x06C */ u8 unk6C;
	/* 0x06D */ u8 unk6D[0x3];
	/* 0x070 */ f32 unk70;
	/* 0x074 */ TBEelTears* unk74;
};

class TOilBall : public TBEelTears {
public:
	TOilBall(const char* name)
	    : TBEelTears(name)
	{
	}

	virtual void load(JSUMemoryInputStream&);
	virtual void calcRootMatrix();
	virtual void moveObject();
	virtual void reset();
};

DECLARE_NERVE(TNerveBEelTearsGenerate, TLiveActor);
DECLARE_NERVE(TNerveBEelTearsMoveUp, TLiveActor);
DECLARE_NERVE(TNerveBEelTearsWaterHit, TLiveActor);
DECLARE_NERVE(TNerveBEelTearsMarioRecover, TLiveActor);
DECLARE_NERVE(TNerveBEelTearsSplit, TLiveActor);
DECLARE_NERVE(TNerveOilBallStay, TLiveActor);
DECLARE_NERVE(TNerveBossEelWaitAppear, TLiveActor);
DECLARE_NERVE(TNerveBossEelFirstSpin, TLiveActor);
DECLARE_NERVE(TNerveBossEelSecondSpin, TLiveActor);
DECLARE_NERVE(TNerveBossEelAppear, TLiveActor);
DECLARE_NERVE(TNerveBossEelOutWait, TLiveActor);
DECLARE_NERVE(TNerveBossEelSlowBack, TLiveActor);
DECLARE_NERVE(TNerveBossEelQuickBack, TLiveActor);
DECLARE_NERVE(TNerveBossEelEat, TLiveActor);
DECLARE_NERVE(TNerveBossEelDie, TLiveActor);
DECLARE_NERVE(TNerveBossEelMouthOpenWait, TLiveActor);
DECLARE_NERVE(TNerveBossEelSleepOnBottom, TLiveActor);

#endif
