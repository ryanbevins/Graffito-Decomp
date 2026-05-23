#ifndef ENEMY_DEMO_BOSS_HANACHAN_HPP
#define ENEMY_DEMO_BOSS_HANACHAN_HPP

#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>

class TDemoBossHanachanSaveParams : public TParams {
public:
	TDemoBossHanachanSaveParams(const char*);

	/* 0x8  */ TParamRT<f32> mSLViewClipFar;
	/* 0x1C */ TParamRT<f32> mSLViewClipRadius;
};

class TDemoBossHanachan : public TSpineEnemy {
public:
	TDemoBossHanachan(const char*);

	virtual BOOL receiveMessage(THitActor*, u32);

	void initBase(TLiveManager*, u32);
};

class TDemoBossHanachanManager : public TEnemyManager {
public:
	TDemoBossHanachanManager(const char*);

	virtual void clipEnemies(JDrama::TGraphics*);

public:
	/* 0x54 */ TDemoBossHanachanSaveParams* mSaveParams;
};

#endif
