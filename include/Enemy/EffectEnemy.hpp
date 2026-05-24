#ifndef ENEMY_EFFECT_ENEMY_HPP
#define ENEMY_EFFECT_ENEMY_HPP

#include <Enemy/WalkerEnemy.hpp>

class TEffectEnemy : public TWalkerEnemy {
public:
	TEffectEnemy(const char*);

	virtual void perform(u32, JDrama::TGraphics*);
	virtual void init(TLiveManager*);
	virtual void kill();
	virtual void reset();
	virtual void behaveToWater(THitActor*);
	virtual void setDeadAnm();
	virtual void forceKill();
	virtual void setMActorAndKeeper();
	virtual void sendAttackMsgToMario();

public:
	/* 0x194 */ int mUnk194;
};

class TEffectEnemyManager : public TSmallEnemyManager {
public:
	TEffectEnemyManager(const char* name)
	    : TSmallEnemyManager(name)
	{
	}

	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual TSpineEnemy* createEnemyInstance();
	virtual void initSetEnemies();
};

#endif
