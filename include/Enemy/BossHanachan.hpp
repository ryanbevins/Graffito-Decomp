#ifndef ENEMY_BOSS_HANACHAN_HPP
#define ENEMY_BOSS_HANACHAN_HPP

#include <Enemy/BossHanachanPartsBase.hpp>
#include <Strategic/LiveActor.hpp>
#include <Strategic/Nerve.hpp>

class TBossHanachanCommonSaveParams;
class TBossHanachanChangeSaveParams;
class TBossHanachanPartsBody;
class TSphereLink;
class MActor;

DECLARE_NERVE(TNerveSBH_Fall, TLiveActor);
DECLARE_NERVE(TNerveSBH_SleepContinue, TLiveActor);
DECLARE_NERVE(TNerveBossHanachanDead, TLiveActor);
DECLARE_NERVE(TNerveBossHanachanSnort, TLiveActor);
DECLARE_NERVE(TNerveBossHanachanDamage, TLiveActor);
DECLARE_NERVE(TNerveBossHanachanGetUp, TLiveActor);
DECLARE_NERVE(TNerveBossHanachanDown, TLiveActor);
DECLARE_NERVE(TNerveBossHanachanTumble, TLiveActor);
DECLARE_NERVE(TNerveBossHanachanGraphWander, TLiveActor);

class TBossHanachan : public TLiveActor {
public:
	void execDamage();
	void execSlip();
	void execWalk(bool);
	void removeAllMapCollision();
	void goToInitialRecoverGraphNode();
	bool checkFallDecideAndSetup();
	bool isTumbleCompletelyAllBody() const;
	void setRandomWeakBodyIndex();
	void changeAnmRateAndFrameUpdate_();
	bool isAllBckAlreadyEnd(EnumBossHanachanAnmKind) const;
	bool isFinishedGetUp() const;
	void considerSetAnm(EnumBossHanachanNerveAnm);
	void setAnmTimerWhenDead();
	void setAnmTimerWhenDamage();
	void setAnmTimerWhenSnort();
	void setAnmTimerWhenGetUp();
	void setTumbleAnm(EnumBossHanachanStopMotionBlendOnOff);
	void setHeadAndBodyAnm(EnumBossHanachanAnmKind,
	                       EnumBossHanachanStopMotionBlendOnOff);
	void emitCamShake_();
	void emitOneTimeSandPillar_(TBossHanachanPartsBody*);
	void emitParticle_();
	static void staticLoadParticle();

public:
	/* 0xF4  */ u8 _F4[0x134 - 0xF4];
	/* 0x134 */ f32 unk134;
	/* 0x138 */ u8 _138[0x13C - 0x138];
	/* 0x13C */ u8 mTempo;
	/* 0x13D */ u8 _13D[3];
	/* 0x140 */ f32 unk140;
	/* 0x144 */ u8 _144[0x150 - 0x144];
	/* 0x150 */ TBossHanachanPartsBase* mBody[8];
	/* 0x170 */ TBossHanachanPartsBase* mHead;
	/* 0x174 */ int unk174;
	/* 0x178 */ TSphereLink* mSphereLink;
	/* 0x17C */ u8 _17C[0x194 - 0x17C];
	/* 0x194 */ f32 unk194;
	/* 0x198 */ f32 unk198;
	/* 0x19C */ MActor* mEffectActor;
	/* 0x1A0 */ JGeometry::TVec3<f32> mEffectPos;
	/* 0x1AC */ u8 _1AC[0x1BC - 0x1AC];
	/* 0x1BC */ TBossHanachanCommonSaveParams* mParams;
	/* 0x1C0 */ TBossHanachanChangeSaveParams* mChangeParams;
};

#endif
