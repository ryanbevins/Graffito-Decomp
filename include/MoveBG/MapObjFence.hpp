#ifndef MOVE_BG_MAP_OBJ_FENCE_HPP
#define MOVE_BG_MAP_OBJ_FENCE_HPP

#include <MoveBG/MapObjBase.hpp>

class TGraphTracer;
class TMapObjMessenger;

class TFence : public TMapObjBase {
public:
	BOOL receiveMessage(THitActor* sender, u32 message);
	void initMapCollisionData();
	void initMapObj();
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
	BOOL receiveMessage(THitActor* sender, u32 message);
	void initMapCollisionData();
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
	BOOL receiveMessage(THitActor* sender, u32 message);
	void calcCurrentMtx();
	void controlWall();
	void controlGroundRoof();
	void setGroundCollision();
	void control();
	void initMapCollisionData();
	void initMapObj();

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
	void draw() const;
	BOOL receiveMessage(THitActor* sender, u32 message);
	void changeStatusToGo();
	void changeStatusToWait();
	void controlRotation();
	void control();
	void initMapCollisionData();
	void initMapObj();
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
	void control();
	void changeStatusToGo();
	void changeStatusToWait();
	TFenceWaterH(const char* name)
	    : TFenceWater(name)
	{
	}
};

class TRailFence : public TFence {
public:
	BOOL receiveMessage(THitActor* sender, u32 message);
	void goOnRail();
	void control();
	void initMapCollisionData();
	void load(JSUMemoryInputStream&);
	TRailFence(const char* name);

	static f32 mFallHeight;
	static int mWaitTime;

public:
	/* 0x13C */ TGraphTracer* unk13C;
	/* 0x140 */ f32 unk140;
};

#endif
