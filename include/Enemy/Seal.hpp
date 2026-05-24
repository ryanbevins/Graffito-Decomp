#ifndef ENEMY_SEAL_HPP
#define ENEMY_SEAL_HPP

#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Strategic/Nerve.hpp>

class TSeal : public TSpineEnemy {
public:
	TSeal(const char* name);

	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();

public:
	/* 0x150 */ int mDamageCount;
};

class TSealManager : public TEnemyManager {
public:
	TSealManager(const char* name);

	virtual void load(JSUMemoryInputStream&);
	virtual void createModelData();
};

DECLARE_NERVE(TNerveSealDie, TLiveActor);
DECLARE_NERVE(TNerveSealWait, TLiveActor);
DECLARE_NERVE(TNerveSealSleep, TLiveActor);

#endif
