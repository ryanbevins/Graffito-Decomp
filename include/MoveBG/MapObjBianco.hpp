#ifndef MOVE_BG_MAP_OBJ_BIANCO_HPP
#define MOVE_BG_MAP_OBJ_BIANCO_HPP

#include <MoveBG/MapObjFloat.hpp>
#include <MoveBG/MapObjMessenger.hpp>
#include <MoveBG/MapObjTurn.hpp>

struct TBGWallCheckRecord;
class TTrembleModelEffect;
class JAISound;
class TBiancoBell;

class TWoodLog : public TMapObjFloatOnSea {
public:
	TWoodLog(const char* name)
	    : TMapObjFloatOnSea(name)
	{
	}
	virtual ~TWoodLog() { }
	virtual void control();
};

class TBellWatermill : public TMapObjTurn {
public:
	TBellWatermill(const char*);
	virtual ~TBellWatermill() { }
	virtual void loadAfter();
	virtual void control();
	virtual u32 touchWater(THitActor*);

public:
	/* 0x16C */ f32 unk16C;
	/* 0x170 */ f32 unk170;
	/* 0x174 */ f32 unk174;
	/* 0x178 */ f32 unk178;
	/* 0x17C */ f32 unk17C;
	/* 0x180 */ f32 unk180;
	/* 0x184 */ f32 unk184;
	/* 0x188 */ f32 unk188;
	/* 0x18C */ f32 unk18C;
	/* 0x190 */ u8 unk190;
	/* 0x191 */ char unk191[3];
	/* 0x194 */ TBiancoBell* unk194;
	/* 0x198 */ TBiancoBell* unk198;
	/* 0x19C */ TBiancoBell* unk19C;
	/* 0x1A0 */ u8 unk1A0;
	/* 0x1A4 */ JAISound* unk1A4;
};

class TBiancoBell : public TMapObjBase {
public:
	TBiancoBell(const char*);
	virtual ~TBiancoBell() { }
	virtual void initMapObj();
	virtual void touchPlayer(THitActor*);
	virtual u32 touchWater(THitActor*);

public:
	/* 0x138 */ s16 unk138;
	/* 0x13A */ u8 unk13A;
	/* 0x13B */ u8 unk13B;
};

class TLampSeesaw : public TMapObjBase {
public:
	TLampSeesaw(const char*);
	virtual ~TLampSeesaw() { }
	virtual void load(JSUMemoryInputStream&);
	virtual void touchPlayer(THitActor*);
	virtual void pushDown(f32);

public:
	/* 0x138 */ TLampSeesaw* unk138;
	/* 0x13C */ f32 unk13C;
	/* 0x140 */ f32 unk140;
};

class TLampSeesawMain : public TLampSeesaw {
public:
	TLampSeesawMain(const char*);
	virtual ~TLampSeesawMain() { }
	virtual void loadAfter();
	virtual void control();
	virtual void touchPlayer(THitActor*);
	virtual void pushDown(f32);

public:
	/* 0x144 */ f32 unk144;
	/* 0x148 */ f32 unk148;
	/* 0x14C */ f32 unk14C;
	/* 0x150 */ f32 unk150;
};

class TLeafBoat : public TMapObjBase {
public:
	TLeafBoat(const char*);
	virtual ~TLeafBoat() { }
	virtual void initMapObj();
	virtual void calc();
	virtual void control();
	virtual void bind();
	void touchWall(JGeometry::TVec3<f32>*, TBGWallCheckRecord*);
	virtual void touchActor(THitActor*);

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
	/* 0x160 */ s32 unk160;
	/* 0x164 */ f32 unk164;
	/* 0x168 */ f32 unk168;
	/* 0x16C */ f32 unk16C;
};

class TLeafBoatRotten : public TLeafBoat {
public:
	TLeafBoatRotten(const char*);
	virtual ~TLeafBoatRotten() { }
	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*);
	virtual void control();

public:
	/* 0x170 */ s32 unk170;
	/* 0x174 */ f32 unk174;
	/* 0x178 */ s16 unk178;
	/* 0x17A */ s16 unk17A;
	/* 0x17C */ s16 unk17C;
	/* 0x17E */ s16 unk17E;

	static f32 mAlphaDownSpeed;
	static f32 mCollisionRemoveAlpha;
	static s16 mRottenColor[4];
};

class TBiancoMiniWindmill : public THideObjBase {
public:
	TBiancoMiniWindmill(const char*);
	virtual ~TBiancoMiniWindmill() { }
	virtual void initMapObj();
	virtual void control();
	virtual void calc();
	virtual u32 touchWater(THitActor*);

public:
	/* 0x150 */ f32 unk150;
	/* 0x154 */ f32 unk154;
	/* 0x158 */ f32 unk158;
	/* 0x15C */ TMapObjMessenger* unk15C;
	/* 0x160 */ JAISound* unk160;

	static f32 mRotWaterAccel;
	static f32 mFriction;
	static f32 mRotSpeedMax;
};

class TBiancoWatermillVertical : public TMapObjBase {
public:
	TBiancoWatermillVertical(const char*);
	virtual ~TBiancoWatermillVertical() { }
	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual void control();
	virtual void setGroundCollision();
	virtual u32 touchWater(THitActor*);

public:
	/* 0x138 */ f32 unk138;
	/* 0x13C */ f32 unk13C;
	/* 0x140 */ TMapObjBase* unk140;
	/* 0x144 */ u8 unk144;
	/* 0x145 */ char unk145[3];
	/* 0x148 */ JAISound* unk148;
	/* 0x14C */ JAISound* unk14C;

	static f32 mRotAccel;
	static f32 mRotSpeedDownRate;
	static f32 mRotSpeedMax;
	static f32 mBridgeRotRate;
};

class TBiancoWatermill : public TMapObjBase {
public:
	TBiancoWatermill(const char*);
	virtual ~TBiancoWatermill() { }
	virtual void initMapObj();
	virtual void control();
	virtual u32 touchWater(THitActor*);
	void turnByEnemy(THitActor*, const TBGCheckData*);

public:
	/* 0x138 */ f32 unk138;
	/* 0x13C */ JAISound* unk13C;
};

class TMapObjRootPakkun : public TMapObjBase {
public:
	TMapObjRootPakkun(const char* name)
	    : TMapObjBase(name)
	{
	}
	virtual ~TMapObjRootPakkun() { }
	virtual void initMapObj();
	virtual void drawObject(JDrama::TGraphics*);

	static f32 mTremblePower;
	static f32 mTrembleAccel;
	static f32 mTrembleBrake;
	static s32 mTrembleTime;

public:
	/* 0x138 */ TTrembleModelEffect* unk138;
};

class TBigWindmill : public TMapObjBase {
public:
	TBigWindmill(const char* name)
	    : TMapObjBase(name)
	{
	}
	virtual ~TBigWindmill() { }
	virtual void load(JSUMemoryInputStream&);
	virtual void control();

public:
	/* 0x138 */ TMapObjBase* unk138[4];
	/* 0x148 */ JAISound* unk148;
};

#endif
