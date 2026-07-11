#ifndef ANIMAL_FISHOID_HPP
#define ANIMAL_FISHOID_HPP

#include <Animal/Realoid.hpp>
#include <Enemy/EnemyManager.hpp>

class TMapObjBase;

class TFish : public TRealoidActor {
public:
	TFish(MActor* actor)
	    : TRealoidActor(actor)
	{
	}

	virtual void init();
};

class TFishoid : public TRealoid {
public:
	TFishoid(int, const char*);
	virtual TRealoidActor* createRealoidActor(MActor*);
	virtual void load(JSUMemoryInputStream&);
	virtual void init(TLiveManager*);
	virtual void perform(u32, JDrama::TGraphics*);

	/* 0x158 */ int mModelType;
	/* 0x15C */ TMapObjBase* mCoinObj;
};

class TFishoidManager : public TEnemyManager {
public:
	TFishoidManager(const char*);
	virtual void createModelData();
};

#endif
