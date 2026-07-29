#ifndef MOVE_BG_MAP_OBJ_ITEM_2_HPP
#define MOVE_BG_MAP_OBJ_ITEM_2_HPP

#include <MoveBG/MapObjBase.hpp>

class TJumpBase : public TMapObjBase {
public:
	TJumpBase(const char*);

	virtual void control();
	virtual void calcRootMatrix();
	virtual Mtx* getRootJointMtx() const;
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void ensureTakeSituation();
	virtual void initMapObj();

public:
	/* 0x138 */ s8 mState;
	/* 0x139 */ u8 unk139;
	/* 0x13A */ u8 unk13A;
	/* 0x13B */ u8 unk13B;
	/* 0x13C */ u32 mTimer;
};

class TMushroom1up : public TMapObjBase {
public:
	TMushroom1up(int, const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*);
	virtual void control();
	virtual void makeObjAppeared();
	virtual void initMapObj();
	virtual void touchPlayer(THitActor*);

public:
	/* 0x138 */ s8 mState;
	/* 0x139 */ s8 mType;
	/* 0x13A */ s8 mTaken;
	/* 0x13B */ u8 unk13B;
	/* 0x13C */ int mTimer;
};

#endif
