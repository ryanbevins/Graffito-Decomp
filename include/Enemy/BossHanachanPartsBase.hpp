#ifndef ENEMY_BOSS_HANACHAN_PARTS_BASE_HPP
#define ENEMY_BOSS_HANACHAN_PARTS_BASE_HPP

#include <Strategic/LiveActor.hpp>
#include <JSystem/JDrama/JDRGraphics.hpp>
#include <Player/ModelWaterManager.hpp>

class TBossHanachan;
class TMapCollisionMove;
class TIdxGroupObj;

enum EnumBossHanachanAnmKind {
	BHANM_KIND_00 = 0,
	BHANM_KIND_01,
	BHANM_KIND_02,
	BHANM_KIND_03,
	BHANM_KIND_04,
	BHANM_KIND_05,
	BHANM_KIND_06,
	BHANM_KIND_07,
	BHANM_KIND_08,
	BHANM_KIND_09,
	BHANM_KIND_0A,
	BHANM_KIND_0B,
	BHANM_KIND_0C,
	BHANM_KIND_0D,
	BHANM_KIND_0E,
	BHANM_KIND_0F,
	BHANM_KIND_10,
	BHANM_KIND_11,
};

enum EnumBossHanachanStopMotionBlendOnOff {
	BHANM_STOP_OFF = 0,
	BHANM_STOP_ON  = 1,
};

enum EnumBossHanachanNerveAnm {
	BHANM_NERVE_0 = 0,
	BHANM_NERVE_1,
	BHANM_NERVE_2,
	BHANM_NERVE_3,
	BHANM_NERVE_4,
	BHANM_NERVE_5,
};

struct TBHPalFrame {
	/* 0x00 */ int unk0;
	/* 0x04 */ int mFrame;
	/* 0x08 */ int unk8;
	/* 0x0C */ f32 unkC;
	/* 0x10 */ f32 unk10;
	/* 0x14 */ f32 unk14;
	/* 0x18 */ f32 unk18;
	/* 0x1C */ f32 unk1C;
	/* 0x20 */ f32 unk20;
	/* 0x24 */ int unk24;
	/* 0x28 */ f32 unk28;
};

class TBossHanachanPartsBase : public TLiveActor {
public:
	TBossHanachanPartsBase(TBossHanachan*, u32, int, const char*);

	virtual const char** getBasNameTable() const;
	virtual BOOL setAnm_(EnumBossHanachanAnmKind anmKind,
	                     EnumBossHanachanStopMotionBlendOnOff stopMotionBlend)
	    = 0;

	void considerSetAnm_(EnumBossHanachanNerveAnm nerveAnm);
	void calcRotateZWhenGetUp_();
	const TLiveActor* getSandActor_() const;
	void copyFrameFromOldAnmToNewAnm_();
	bool isCurBckAlreadyEnd_() const;
	void setDamageFog_(JDrama::TGraphics*);
	void entryCircleShadow_();
	void moveMapCollision_();
	void changeTumbleAnmRate_();
	void initMapCollisionAndHitActor_(TIdxGroupObj*);

public:
	/* 0xF4  */ int mCurAnm;
	/* 0xF8  */ int mPrevAnm;
	/* 0xFC  */ TBossHanachan* mOwner;
	/* 0x100 */ TWaterHitActor* mWaterHit;
	/* 0x104 */ TMapCollisionMove* mMapCollision;
	/* 0x108 */ MtxPtr mCenterJointMtx;
	/* 0x10C */ int mAnmCounter;
	/* 0x110 */ TBHPalFrame* mPalFrame;
};

class TBossHanachanPartsHead : public TBossHanachanPartsBase {
public:
	TBossHanachanPartsHead(TBossHanachan*, const char*);

	virtual BOOL receiveMessage(THitActor* sender, u32 message);
	virtual BOOL setAnm_(EnumBossHanachanAnmKind anmKind,
	                     EnumBossHanachanStopMotionBlendOnOff stopMotionBlend);

public:
	/* 0x114 */ MtxPtr mLeftNoseHallJointMtx;
	/* 0x118 */ MtxPtr mRightNoseHallJointMtx;
};

class TFootHitActor : public TWaterHitActor {
public:
	TFootHitActor(const char* name)
	    : TWaterHitActor(name)
	{
	}
};

class TBossHanachanPartsBody : public TBossHanachanPartsBase {
public:
	TBossHanachanPartsBody(TBossHanachan*, const char*);

	virtual BOOL receiveMessage(THitActor* sender, u32 message);
	virtual BOOL setAnm_(EnumBossHanachanAnmKind anmKind,
	                     EnumBossHanachanStopMotionBlendOnOff stopMotionBlend);

	void initFootHitActor_(TIdxGroupObj*);

public:
	/* 0x114 */ int unk114;
	/* 0x118 */ TFootHitActor* mFeet[2];
	/* 0x120 */ f32 unk120;
	/* 0x124 */ f32 unk124;
	/* 0x128 */ f32 unk128;
	/* 0x12C */ f32 unk12C;
	/* 0x130 */ f32 unk130;
	/* 0x134 */ f32 unk134;
	/* 0x138 */ f32 unk138;
	/* 0x13C */ f32 unk13C;
	/* 0x140 */ f32 unk140;
	/* 0x144 */ f32 unk144;
	/* 0x148 */ f32 unk148;
	/* 0x14C */ MtxPtr mLeftLegJointMtx;
	/* 0x150 */ MtxPtr mRightLegJointMtx;
	/* 0x154 */ f32 unk154;
	/* 0x158 */ f32 unk158;
	/* 0x15C */ f32 unk15C;
};


#endif
