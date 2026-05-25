#ifndef MOVE_BG_MAP_OBJ_FENCE_HPP
#define MOVE_BG_MAP_OBJ_FENCE_HPP

#include <MoveBG/MapObjBase.hpp>

class TGraphTracer;
class TMapObjMessenger;

class TFence : public TMapObjBase {
public:
	virtual BOOL receiveMessage(THitActor* sender, u32 message);
	virtual void initMapObj();
	virtual void initMapCollisionData();
	TFence(const char* name)
	    : TMapObjBase(name)
	    , unk138(0)
	{
	}

public:
	/* 0x138 */ u8 unk138;
};

class TRevolvingFenceOuter : public TFence {
public:
	virtual BOOL receiveMessage(THitActor* sender, u32 message);
	virtual void initMapCollisionData();
	TRevolvingFenceOuter(const char* name = "\x83\x74\x83\x46\x83\x93\x83\x58\x8a\x4f\x91\xa4")
	    : TFence(name)
	    , unk13C(nullptr)
	{
	}

public:
	/* 0x13C */ TMapObjBase* unk13C;
};

class TRevolvingFenceInner : public TFence {
public:
	virtual BOOL receiveMessage(THitActor* sender, u32 message);
	virtual void setGroundCollision();
	virtual void control();
	virtual void initMapObj();
	virtual void initMapCollisionData();
	void calcCurrentMtx();
	void controlWall();
	void controlGroundRoof();

	TRevolvingFenceInner(const char* name = "\x83\x74\x83\x46\x83\x93\x83\x58\x93\xe0\x91\xa4")
	    : TFence(name)
	    , unk13C(0.0f)
	    , unk140(1)
	{
	}

	static f32 mSpeed;

public:
	/* 0x13C */ f32 unk13C;
	/* 0x140 */ u8 unk140;
};

class TFenceWater : public TFence {
public:
	virtual BOOL receiveMessage(THitActor* sender, u32 message);
	virtual void control();
	virtual void initMapObj();
	virtual void initMapCollisionData();
	virtual void draw() const;
	virtual void changeStatusToGo();
	virtual void changeStatusToWait();
	void controlRotation();
	TFenceWater(const char* name)
	    : TFence(name)
	    , unk13C(0.0f)
	    , unk140(0.0f)
	    , unk144(0)
	{
	}

	static f32 mWaterAccel;
	static f32 mBackSpeed;
	static int mTurnedWaitTime;

public:
	/* 0x13C */ f32 unk13C;
	/* 0x140 */ f32 unk140;
	/* 0x144 */ TMapObjMessenger* unk144;
};

class TFenceWaterH : public TFenceWater {
public:
	virtual void control();
	virtual void changeStatusToGo();
	virtual void changeStatusToWait();
	TFenceWaterH(const char* name)
	    : TFenceWater(name)
	{
	}
};

class TRailFence : public TFence {
public:
	virtual void load(JSUMemoryInputStream&);
	virtual BOOL receiveMessage(THitActor* sender, u32 message);
	virtual void control();
	virtual void initMapCollisionData();
	void goOnRail();
	TRailFence(const char* name);

	static f32 mFallHeight;
	static int mWaitTime;

public:
	/* 0x13C */ TGraphTracer* unk13C;
	/* 0x140 */ f32 unk140;
};

#endif
