#ifndef MOVE_BG_MAP_OBJ_TOWN_HPP
#define MOVE_BG_MAP_OBJ_TOWN_HPP

#include <JSystem/JDrama/JDRActor.hpp>
#include <MoveBG/MapObjBase.hpp>
#include <MoveBG/MapObjGeneral.hpp>
#include <MoveBG/MapObjHide.hpp>

class TMapCollisionWarp;
class JAISound;

class TDoor : public TMapObjBase {
public:
	virtual void load(JSUMemoryInputStream&);
	virtual void touchPlayer(THitActor*);

	TDoor(const char*);

public:
	/* 0x138 */ u8 unk138;
};

class TManhole : public TMapObjGeneral {
public:
	virtual void setGroundCollision();
	virtual void calc();
	virtual void touchPlayer(THitActor*);
	virtual void initMapObj();
	virtual void loadAfter();
	virtual void appeared();

	BOOL animationFinished();
	void makeManholeUnuseful(const TMapObjBase*);

	TManhole(const char*);

	static f32 mDownHeight;
	static f32 mDownSpeed;
	static f32 mVibrationSpeed;
	static f32 mVibrationEndHeight;
	static f32 mVibrationDecreaseRate;

public:
	/* 0x148 */ f32 unk148;
	/* 0x14C */ f32 unk14C;
	/* 0x150 */ u8 unk150;
	/* 0x151 */ u8 unk151;
	/* 0x152 */ u8 unk152;
	/* 0x153 */ u8 unk153;
	/* 0x154 */ const TMapObjBase* unk154;
	/* 0x158 */ TMapCollisionWarp* unk158;
};

class TMapObjBillboard : public THideObjBase {
public:
	void swing(THitActor*);
	virtual void touchActor(THitActor*);
	virtual u32 touchWater(THitActor*);
	TMapObjBillboard()
	    : THideObjBase("看板")
	    , unk150(nullptr)
	{
	}

public:
	/* 0x150 */ JAISound* unk150;
};

class TMapObjChangeStage : public TMapObjBase {
public:
	virtual void load(JSUMemoryInputStream&);
	virtual void touchPlayer(THitActor*);
	TMapObjChangeStage(const char* name = "ステージ切り替え")
	    : TMapObjBase(name)
	    , unk138(0)
	{
	}

public:
	/* 0x138 */ u16 unk138;
};

class TMapObjChangeStageHipDrop : public TMapObjChangeStage {
public:
	virtual void touchPlayer(THitActor*);
	virtual void initMapObj();
	TMapObjChangeStageHipDrop(const char* name)
	    : TMapObjChangeStage(name)
	{
	}
};

class TMapObjStartDemo : public TMapObjBase {
public:
	virtual void load(JSUMemoryInputStream&);
	virtual void touchPlayer(THitActor*);
	TMapObjStartDemo()
	    : TMapObjBase("デモ開始オブジェ")
	    , unk138(0)
	{
	}

public:
	/* 0x138 */ u32 unk138;
};

class TDamageObj : public THitActor {
public:
	virtual void load(JSUMemoryInputStream&);
	virtual void perform(unsigned long, JDrama::TGraphics*);
	void init(unsigned long);
	TDamageObj();
	TDamageObj(const char* name)
	    : THitActor(name)
	{
	}
};

class TShadowObj {
public:
	void load(JSUMemoryInputStream&);
};

class TMapObjWaterSpray : public TMapObjBase {
public:
	virtual void load(JSUMemoryInputStream&);
	virtual void calc();
	TMapObjWaterSpray(const char*);

public:
	/* 0x138 */ s32 unk138;
	/* 0x13C */ f32 unk13C;
	/* 0x140 */ JGeometry::TVec3<f32> unk140;
	/* 0x14C */ JUtility::TColor unk14C;
};

class THideObjInfo : public JDrama::TActor {
public:
	virtual void load(JSUMemoryInputStream&);
	virtual void action(long);
	THideObjInfo(const char*);

public:
	/* 0x44 */ u32 unk44;
	/* 0x48 */ f32 unk48;
	/* 0x4C */ f32 unk4C;
};

class TMapObjSwitch : public TMapObjBase {
public:
	virtual void load(JSUMemoryInputStream&);
	virtual BOOL receiveMessage(THitActor*, unsigned long);
	virtual void control();
	void registerObjInfo(THideObjInfo* info)
	{
		unk144[unk13C] = info;
		unk13C += 1;
	}
	TMapObjSwitch(const char*);

public:
	/* 0x138 */ s32 unk138;
	/* 0x13C */ s32 unk13C;
	/* 0x140 */ s32 unk140;
	/* 0x144 */ THideObjInfo** unk144;
	/* 0x148 */ u16 unk148;
	/* 0x14A */ u16 unk14A;
	/* 0x14C */ u16 unk14C;
	/* 0x14E */ u16 unk14E;
};

class TRedCoinSwitch : public TMapObjBase {
public:
	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual BOOL receiveMessage(THitActor*, unsigned long);
	virtual void control();
	TRedCoinSwitch(const char*);

public:
	/* 0x138 */ s32 unk138;
	/* 0x13C */ u16 unk13C;
	/* 0x13E */ u16 unk13E;
	/* 0x140 */ u16 unk140;
	/* 0x142 */ u16 unk142;
};

class TBasketReverse : public TMapObjBase {
public:
	virtual void kill();
	virtual void initMapObj();
	TBasketReverse()
	    : TMapObjBase("さかさバスケット")
	{
	}
};

#endif
