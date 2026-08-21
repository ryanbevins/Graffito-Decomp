#ifndef MOVE_BG_MAP_OBJ_RICCO_HPP
#define MOVE_BG_MAP_OBJ_RICCO_HPP

#include <MoveBG/MapObjBase.hpp>
#include <MoveBG/MapObjBlock.hpp>
#include <MoveBG/Item.hpp>
#include <dolphin/gx/GXStruct.h>

class TCraneRotY : public TMapObjBase {
public:
	TCraneRotY()
	    : TMapObjBase("Ｙ軸回転クレーン")
	    , unk138(0.0f)
	    , unk13C(0.0f)
	    , unk140(0.0f)
	    , unk144(0.0f)
	    , unk148(0)
	{
	}

	virtual void load(JSUMemoryInputStream&);
	virtual void control();
	virtual void calc();

	static s32 mWaitTime;

public:
	/* 0x138 */ f32 unk138;
	/* 0x13C */ f32 unk13C;
	/* 0x140 */ f32 unk140;
	/* 0x144 */ f32 unk144;
	/* 0x148 */ u32 unk148;
};

class TCraneUpDown : public TMapObjBase {
public:
	TCraneUpDown()
	    : TMapObjBase("上下クレーン")
	    , unk138(0)
	    , unk13C(0)
	    , unk140(0.0f)
	    , unk144(0.0f)
	{
	}

	virtual void initMapObj();
	virtual void control();

	static f32 mRotSpeed;
	static s32 mWaitTime;

public:
	/* 0x138 */ TMapObjBase* unk138;
	/* 0x13C */ u32 unk13C;
	/* 0x140 */ f32 unk140;
	/* 0x144 */ f32 unk144;
};

class TCraneCargo : public TLeanBlock {
public:
	TCraneCargo()
	    : TLeanBlock("クレーン積み荷")
	{
	}

	virtual void control();
	virtual void calc();
};

class TRiccoWatermill : public TMapObjBase {
public:
	TRiccoWatermill(const char*);

	virtual void loadAfter();
	virtual void control();
	virtual void calc();
	virtual u32 touchWater(THitActor*);

	static f32 mRotAccel;
	static f32 mRotSpeedMaxUp;
	static f32 mRotSpeedMaxDown;
	static f32 mRotDown;
	static f32 mSubmarineMoveRate;
	static f32 mSubmarineMaxTransY;
	static f32 mSubmarineBottomTransY;
	static f32 mSubmarineSurfaceTransY;
	static s32 mWaitTime;

public:
	/* 0x138 */ f32 unk138;
	/* 0x13C */ TMapObjBase* unk13C;
	/* 0x140 */ s32 unk140;
	/* 0x144 */ u8 unk144;
	/* 0x148 */ TMapObjBase* unk148;
	/* 0x14C */ void* unk14C;
	/* 0x150 */ void* unk150;
	/* 0x154 */ void* unk154;
};

class TSurfGesoObj : public TItem {
public:
	TSurfGesoObj()
	    : TItem("イカサーフィン")
	{
	}

	virtual void initMapObj();

public:
	/* 0x154 */ GXColorS10 unk154;
};

class TFruitSwitch : public TMapObjBase {
public:
	TFruitSwitch()
	    : TMapObjBase("フルーツスイッチ")
	    , unk138(0)
	{
	}

	virtual BOOL receiveMessage(THitActor* sender, u32 message);

public:
	/* 0x138 */ TMapObjBase* unk138;
};

class TFruitLauncher : public TMapObjBase {
public:
	TFruitLauncher()
	    : TMapObjBase("フルーツ発射口")
	    , unk140(0)
	{
	}

	virtual void loadAfter();
	void fireObj();

	static f32 mObjSpeedXZ;
	static f32 mObjSpeedY;
	static s32 mFruitLiveTime;

public:
	/* 0x138 */ TFruitSwitch* unk138;
	/* 0x13C */ TFruitSwitch* unk13C;
	/* 0x140 */ u32 unk140;
};

#endif
