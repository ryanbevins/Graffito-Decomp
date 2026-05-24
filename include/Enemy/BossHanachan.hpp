#ifndef ENEMY_BOSS_HANACHAN_HPP
#define ENEMY_BOSS_HANACHAN_HPP

#include <Strategic/LiveActor.hpp>
#include <Strategic/Nerve.hpp>

class TBossHanachanCommonSaveParams;

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

public:
	/* 0xF4  */ u8 _F4[0x174 - 0xF4];
	/* 0x174 */ int unk174;
	/* 0x178 */ u8 _178[0x1BC - 0x178];
	/* 0x1BC */ TBossHanachanCommonSaveParams* mParams;
};

#endif
