#ifndef ENEMY_BOSS_TELESA_HPP
#define ENEMY_BOSS_TELESA_HPP

#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Enemy/SmallEnemy.hpp>

class TBossTelesa : public TSpineEnemy {
public:
	TBossTelesa(const char*);
	virtual ~TBossTelesa();

	/* 0x150 */ u8 unk150[0x23C];
};

class TBossTelesaManager : public TEnemyManager {
public:
	TBossTelesaManager(const char*);
	virtual ~TBossTelesaManager();
};

class TBubbleManager : public TSmallEnemyManager {
public:
	TBubbleManager(const char*);
	virtual ~TBubbleManager();
};

#endif
