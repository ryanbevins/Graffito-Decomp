#ifndef ENEMY_BOSS_HANACHAN_PARTS_BASE_HPP
#define ENEMY_BOSS_HANACHAN_PARTS_BASE_HPP

#include <Strategic/LiveActor.hpp>

class TBossHanachan;

class TBossHanachanPartsBase : public TLiveActor {
public:
	TBossHanachanPartsBase(TBossHanachan*, u32, int, const char*);

	virtual const char** getBasNameTable() const;
};

#endif
