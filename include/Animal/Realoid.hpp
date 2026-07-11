#ifndef ANIMAL_REALOID_HPP
#define ANIMAL_REALOID_HPP

#include <Enemy/Enemy.hpp>
#include <Strategic/TakeActor.hpp>

class MActor;
class TBoid;
class TBoidLeader;
class TRealoidActor;

class TRealoid : public TSpineEnemy {
public:
	TRealoid(const char*);

	void loadDefault(JSUMemoryInputStream&, const char*, int);
	void clipBoids(JDrama::TGraphics*);
	virtual void perform(u32, JDrama::TGraphics*);
	virtual TRealoidActor* createRealoidActor(MActor*) = 0;

	/* 0x150 */ TBoidLeader* mBoidLeader;
	/* 0x154 */ TRealoidActor** mActors;
};

class TRealoidActor : public TTakeActor {
public:
	TRealoidActor(MActor*);

	virtual MtxPtr getTakingMtx();
	virtual void init() = 0;
	virtual void perform(u32, JDrama::TGraphics*);

	void calcRootMatrix(TBoid*);
	void checkHitActors();

	/* 0x70 */ MActor* mMActor;
	/* 0x74 */ u32 unk74;
	/* 0x78 */ Mtx mTakingMtx;
};

#endif
