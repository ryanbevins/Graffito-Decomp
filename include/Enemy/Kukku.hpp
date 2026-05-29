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

	virtual void perform(u32, JDrama::TGraphics*);
	virtual const char** getBasNameTable() const;
	virtual void setDeadAnm();

	// fabricated
	TKukku** getKukkuBalls() { return (TKukku**)&unk194; }

public:
	/* 0x194 */ void* unk194[3];
	/* 0x1A0 */ void* unk1A0;
	/* 0x1A4 */ u32 unk1A4;
	/* 0x1A8 */ s32 unk1A8;
	/* 0x1AC */ s32 unk1AC;
};

DECLARE_NERVE(TNerveKukkuFall, TLiveActor);
DECLARE_NERVE(TNerveKukkuPostFall, TLiveActor);
DECLARE_NERVE(TNerveKukkuRecoverGraph, TLiveActor);
DECLARE_NERVE(TNerveKukkuGraphWander, TLiveActor);

#endif
