#ifndef ENEMY_BOSS_EEL_HPP
#define ENEMY_BOSS_EEL_HPP

#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Strategic/Nerve.hpp>

class TLiveActor;

class TBossEel : public TSpineEnemy {
public:
	TBossEel(const char*);
	virtual ~TBossEel();

	/* 0x150 */ u8 unk150[0xD0];
};

class TBossEelSaveParams {
public:
	TBossEelSaveParams();

	/* 0x000 */ u8 unk0[0x2C4];
};

class TBossEelManager : public TEnemyManager {
public:
	TBossEelManager(const char* name)
	    : TEnemyManager(name)
	    , mSaveParams()
	{
	}

	virtual ~TBossEelManager();

	/* 0x054 */ TBossEelSaveParams mSaveParams;
};

class TBEelTears : public TSpineEnemy {
public:
	TBEelTears(const char*);

	/* 0x150 */ u8 unk150[0x20];
};

class TBEelTearsManager : public TEnemyManager {
public:
	TBEelTearsManager(const char*);
	virtual ~TBEelTearsManager();

	/* 0x054 */ u8 unk54[0x78];
};

class TOilBall : public TBEelTears {
public:
	TOilBall(const char* name)
	    : TBEelTears(name)
	{
	}

	virtual ~TOilBall();
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
