#ifndef ENEMY_EMARIO_HPP
#define ENEMY_EMARIO_HPP

#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>

class TEnemyMario;

class TEMarioManager : public TEnemyManager {
public:
	TEMarioManager(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual TSpineEnemy* createEnemyInstance();
};

class TEMario : public TSpineEnemy {
public:
	TEMario(const char* = "マリオモドキ");

	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void init(TLiveManager*);
	virtual void kill();
	virtual void loadAfter();

	void forceDisappear();
	void startGateDrawing();
	void startMonteReplay(u32);
	void startRunAway();
	bool isDownWaitingToTalk() const;
	bool isReachedToGate() const;
	BOOL isGoal();

public:
	/* 0x150 */ TEnemyMario* mEnemyMario;
	/* 0x154 */ u32 unk154;
	/* 0x158 */ u32 unk158;
	/* 0x15C */ u32 unk15C;
	/* 0x160 */ u32 unk160;
};

#endif
