#ifndef ENEMY_KUKKU_HPP
#define ENEMY_KUKKU_HPP

#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Enemy/SmallEnemy.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/HitActor.hpp>
#include <Strategic/Nerve.hpp>

class TKukkuManager : public TSmallEnemyManager {
public:
	TKukkuManager(const char*);
	virtual void createModelData();
};

class TKukku : public TSmallEnemy {
public:
	TKukku(const char* = "\x83\x68\x83\x68\x83\x8A\x83\x67\x83\x8A");

	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void perform(u32, JDrama::TGraphics*);
	virtual void control();
	virtual void bind();
	virtual const char** getBasNameTable() const;
	virtual void reset();
	virtual void setDeadAnm();
	virtual void setAfterDeadEffect();

	// fabricated
	TKukku** getKukkuBalls() { return (TKukku**)&unk194; }
	s32 getUnk1A4() const { return unk1A4; }

public:
	/* 0x194 */ void* unk194[3];
	/* 0x1A0 */ void* unk1A0;
	/* 0x1A4 */ s32 unk1A4;
	/* 0x1A8 */ s32 unk1A8;
	/* 0x1AC */ s32 unk1AC;
	/* 0x1B0 */ s32 unk1B0;
};

DECLARE_NERVE(TNerveKukkuFall, TLiveActor);
DECLARE_NERVE(TNerveKukkuPostFall, TLiveActor);
DECLARE_NERVE(TNerveKukkuRecoverGraph, TLiveActor);
DECLARE_NERVE(TNerveKukkuGraphWander, TLiveActor);

#endif
