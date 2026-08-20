#ifndef ENEMY_ENEMY_MARIO_HPP
#define ENEMY_ENEMY_MARIO_HPP

#include <Player/MarioMain.hpp>

class TEMario;

class TEnemyMario : public TMario {
public:
	virtual void perform(u32, JDrama::TGraphics*);

	void drawHPMeter(float (*)[4]);
	void damageExec(THitActor*, int, int, int, f32, int, f32, s16);
	void playerControl(JDrama::TGraphics*);
	void checkController(JDrama::TGraphics*);
	void checkReturn();
	void reachGoal();
	u8 thinkTrample();
	void hitWater(THitActor*);
	void consider();
	void startGateDrawing();
	void emWaitingToInviteMario();
	void decideDoingAfterCarry();
	void emRunAwayToNearestNode();
	void findRunAwayNearestNode();
	void startRunAway();
	void emDownAnimation();
	void emReplayJumpToNearestNode();
	void emReplay();
	void emDisappearToGate();
	void startDisappear(u16);
	void emWalkAround();
	void emJumping();
	virtual void emWaiting();
	BOOL tryTake();
	void changeEMDoing(u16);
	void startMonteReplay(u32);
	void initEnemyValues();
	void initModel();
	void initValues();

	// Field accessors — TEnemyMario overlays fields into TMario's
	// trailing unk4290[0x80] buffer; the offsets are TEnemyMario-specific.
	u16& doingRef() { return *(u16*)(unk4290 + 2); }
	u16 doing() const { return *(const u16*)(unk4290 + 2); }
	u16 goalFlag() const { return *(const u16*)(unk4290 + 0); }
	f32 distToMario() const { return *(const f32*)(unk4290 + 0xc); }
	TEMario*& ownerRef() { return *(TEMario**)(unk4290 + 0x10); }
	TEMario* owner() const { return *(TEMario* const*)(unk4290 + 0x10); }
	f32 waterRange() const { return *(const f32*)(unk4290 + 0x20); }
	u32 initFlag() const { return *(const u32*)(unk4290 + 0x4c); }
	u32& stateFlagRef() { return *(u32*)(unk4290 + 0x88); } // unused; see below
};

#endif
