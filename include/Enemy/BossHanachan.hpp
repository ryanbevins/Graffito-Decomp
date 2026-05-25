#ifndef ENEMY_BOSS_HANACHAN_HPP
#define ENEMY_BOSS_HANACHAN_HPP

#include <Enemy/BossHanachanPartsBase.hpp>
#include <Strategic/LiveActor.hpp>
#include <Strategic/Nerve.hpp>

class TBossHanachanCommonSaveParams;
class TBossHanachanChangeSaveParams;

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
	bool isAllBckAlreadyEnd(EnumBossHanachanAnmKind) const;
	bool isFinishedGetUp() const;
	void considerSetAnm(EnumBossHanachanNerveAnm);
	void setAnmTimerWhenSnort();
	void setAnmTimerWhenGetUp();
	void setTumbleAnm(EnumBossHanachanStopMotionBlendOnOff);
	void setHeadAndBodyAnm(EnumBossHanachanAnmKind,
	                       EnumBossHanachanStopMotionBlendOnOff);

public:
	/* 0xF4  */ u8 _F4[0x13C - 0xF4];
	/* 0x13C */ u8 mTempo;
	/* 0x13D */ u8 _13D[3];
	/* 0x140 */ f32 unk140;
	/* 0x144 */ u8 _144[0x174 - 0x144];
	/* 0x174 */ int unk174;
	/* 0x178 */ u8 _178[0x1BC - 0x178];
	/* 0x1BC */ TBossHanachanCommonSaveParams* mParams;
	/* 0x1C0 */ TBossHanachanChangeSaveParams* mChangeParams;
};

#endif
