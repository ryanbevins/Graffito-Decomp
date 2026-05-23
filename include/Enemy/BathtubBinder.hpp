#ifndef ENEMY_BATHTUB_BINDER_HPP
#define ENEMY_BATHTUB_BINDER_HPP

#include <Strategic/Binder.hpp>
#include <types.h>

class TLiveActor;
class TBathtub;
class TBathWaterManager;

class TBathtubBinder : public TBinder {
public:
	TBathtubBinder();
	virtual ~TBathtubBinder();
	virtual void bind(TLiveActor* actor);

	bool init(f32 a, f32 b, f32 c, f32 d, f32 e);
	void float_(TLiveActor* actor);

public:
	/* 0x04 */ TBathtub* mBathtub;
	/* 0x08 */ TBathWaterManager* mBathWaterMgr;
	/* 0x0C */ f32 mUnk0C;
	/* 0x10 */ f32 mUnk10;
	/* 0x14 */ f32 mUnk14;
	/* 0x18 */ f32 mUnk18;
	/* 0x1C */ f32 mUnk1C;
	/* 0x20 */ f32 mUnk20;
};

#endif
