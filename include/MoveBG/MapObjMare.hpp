#ifndef MOVE_BG_MAP_OBJ_MARE_HPP
#define MOVE_BG_MAP_OBJ_MARE_HPP

#include <MoveBG/MapObjBase.hpp>

struct TBGWallCheckRecord;

class TCannon;
class TMareEventDepressWall;

class TCogwheel;

class TCogwheelScale : public TMapObjBase {
public:
	TCogwheelScale(const char*);
	virtual ~TCogwheelScale() { }
	virtual BOOL receiveMessage(THitActor* sender, u32 message);
	virtual void control();
	virtual void touchPlayer(THitActor*);
	virtual u32 touchWater(THitActor*);

public:
	/* 0x138 */ f32 unk138;
	/* 0x13C */ f32 unk13C;
	/* 0x140 */ f32 unk140;
	/* 0x144 */ f32 unk144;
	/* 0x148 */ f32 unk148;
	/* 0x14C */ f32 unk14C;
	/* 0x150 */ f32 unk150;
	/* 0x154 */ u8 unk154;
	/* 0x158 */ TCogwheel* unk158;

	static f32 mWaterLeakSpeed;
};

class TCogwheel : public TMapObjBase {
public:
	TCogwheel(const char*);
	virtual ~TCogwheel() { }
	virtual void control();
	virtual void initMapObj();
	virtual void calc();
	virtual void draw() const;

	void initDraw() const;
	void rebound();

public:
	/* 0x138 */ f32 unk138;
	/* 0x13C */ f32 unk13C;
	/* 0x140 */ f32 unk140;
	/* 0x144 */ f32 unk144;
	/* 0x148 */ f32 unk148;
	/* 0x14C */ f32 unk14C;
	/* 0x150 */ TCogwheelScale* unk150;
	/* 0x154 */ JGeometry::TVec3<f32> unk154;
	/* 0x160 */ f32 unk160;
	/* 0x164 */ TCogwheelScale* unk164;
	/* 0x168 */ JGeometry::TVec3<f32> unk168;
	/* 0x174 */ f32 unk174;

	static f32 mRopeWidthX;
	static f32 mRopeWidthZ;
	static f32 mTexPosRate;
	static f32 mMinSpeed;
};

class TMapObjElasticCode : public TMapObjBase {
public:
	TMapObjElasticCode()
	    : TMapObjBase("ゴムひも")
	    , unk138(0.0f)
	    , unk13C(0.0f)
	    , unk140(0.0f)
	{
	}
	virtual ~TMapObjElasticCode() { }
	virtual void control();
	virtual void initMapObj();
	virtual void draw() const;

public:
	/* 0x138 */ f32 unk138;
	/* 0x13C */ f32 unk13C;
	/* 0x140 */ f32 unk140;
};

class TMapObjGrowTree : public TMapObjBase {
public:
	TMapObjGrowTree(const char*);
	virtual ~TMapObjGrowTree() { }
	virtual void loadAfter();
	virtual void control();
	virtual void initMapObj();
	virtual u32 touchWater(THitActor*);

public:
	/* 0x138 */ f32 unk138;
	/* 0x13C */ f32 unk13C;
	/* 0x140 */ f32 unk140;
	/* 0x144 */ u32 unk144;
	/* 0x148 */ f32 unk148;
};

class TWireBell : public TMapObjBase {
public:
	TWireBell(const char*);
	virtual ~TWireBell() { }
	virtual void loadAfter();
	virtual void control();
	virtual void draw() const;

	void initDraw() const;

public:
	/* 0x138 */ int unk138;
	/* 0x13C */ f32 unk13C;
	/* 0x140 */ f32 unk140;
	/* 0x144 */ f32 unk144;
	/* 0x148 */ f32 unk148;
	/* 0x14C */ JGeometry::TVec3<f32> unk14C;
};

class TMapObjPuncher : public TMapObjBase {
public:
	TMapObjPuncher()
	    : TMapObjBase("パンチャー")
	    , unk138(0.0f)
	{
	}
	virtual ~TMapObjPuncher() { }
	virtual void load(JSUMemoryInputStream&);
	virtual void control();
	virtual void touchPlayer(THitActor*);

public:
	/* 0x138 */ f32 unk138;
};

class TMuddyBoat : public TMapObjBase {
public:
	TMuddyBoat(const char*);
	virtual ~TMuddyBoat() { }
	virtual void calcRootMatrix();
	virtual void control();
	virtual void bind();
	virtual void kill();
	virtual void initMapObj();
	virtual u32 getSDLModelFlag() const;
	virtual void calc();

	void moveByWater();
	void touchWall(JGeometry::TVec3<f32>*, const TBGWallCheckRecord&);
	BOOL bindToWall(const JGeometry::TVec3<f32>&, f32,
	                JGeometry::TVec3<f32>*);

public:
	/* 0x138 */ f32 unk138;
	/* 0x13C */ f32 unk13C;
	/* 0x140 */ f32 unk140;
	/* 0x144 */ f32 unk144;
	/* 0x148 */ f32 unk148;
	/* 0x14C */ f32 unk14C;
	/* 0x150 */ f32 unk150;
	/* 0x154 */ f32 unk154;
	/* 0x158 */ f32 unk158;
	/* 0x15C */ f32 unk15C;
	/* 0x160 */ f32 unk160;
	/* 0x164 */ f32 unk164;
	/* 0x168 */ u32 unk168;
	/* 0x16C */ u32 unk16C;
	/* 0x170 */ JGeometry::TVec3<f32> unk170;
	/* 0x17C */ JGeometry::TVec3<f32> unk17C;
};

class TMareFall : public TMapObjBase {
public:
	TMareFall()
	    : TMapObjBase("マーレ滝")
	{
	}
	virtual ~TMareFall() { }
	virtual void load(JSUMemoryInputStream&);
	virtual void calc();
};

class TMareCork : public TMapObjBase {
public:
	TMareCork()
	    : TMapObjBase("マーレコルク")
	    , unk138(nullptr)
	    , unk154(0)
	{
	}
	virtual ~TMareCork() { }
	virtual void loadAfter();
	virtual MtxPtr getTakingMtx();
	virtual void calcRootMatrix();
	virtual void moveObject();
	virtual void drawObject(JDrama::TGraphics*);

public:
	/* 0x138 */ TCannon* unk138;
	/* 0x13C */ JGeometry::TVec3<f32> unk13C;
	/* 0x148 */ JGeometry::TVec3<f32> unk148;
	/* 0x154 */ u8 unk154;
};

class TMareEventPoint : public THitActor {
public:
	TMareEventPoint()
	    : THitActor("イベントポイント")
	    , unk68(nullptr)
	{
	}
	virtual ~TMareEventPoint();
	virtual void load(JSUMemoryInputStream&);
	virtual BOOL receiveMessage(THitActor* sender, u32 message);

public:
	/* 0x68 */ TMareEventDepressWall* unk68;
};

#endif
