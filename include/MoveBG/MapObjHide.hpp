#ifndef MOVE_BG_MAP_OBJ_HIDE_HPP
#define MOVE_BG_MAP_OBJ_HIDE_HPP

#include <MoveBG/MapObjBase.hpp>

class THideObjBase : public TMapObjBase {
public:
	THideObjBase(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual BOOL receiveMessage(THitActor* sender, u32 message);
	virtual void appearObj(f32);
	virtual void appearObjFromPoint(const JGeometry::TVec3<f32>&);
	virtual void emitEffect();

public:
	/* 0x138 */ TMapObjBase* unk138;
	/* 0x13C */ f32 unk13C;
	/* 0x140 */ f32 unk140;
	/* 0x144 */ u32 unk144;
	/* 0x148 */ u32 unk148;
	/* 0x14C */ u8 unk14C;
};

class THideObj : public THideObjBase {
public:
	THideObj(const char* name)
	    : THideObjBase(name)
	{
	}

	virtual void touchPlayer(THitActor*);
};

class THipDropHideObj : public THideObjBase {
public:
	THipDropHideObj(const char* name)
	    : THideObjBase(name)
	{
	}

	virtual void touchPlayer(THitActor*);
};

class TWaterHitHideObj : public THideObjBase {
public:
	TWaterHitHideObj(const char* name)
	    : THideObjBase(name)
	{
	}

	virtual void load(JSUMemoryInputStream&);
	virtual u32 touchWater(THitActor*);
};

class TFruitHitHideObj : public THideObjBase {
public:
	TFruitHitHideObj(const char* name)
	    : THideObjBase(name)
	{
	}

	virtual void load(JSUMemoryInputStream&);
	virtual void touchActor(THitActor*);
	virtual void touchFruit(THitActor*);
};

class TFruitBasket : public TFruitHitHideObj {
public:
	TFruitBasket(const char* name)
	    : TFruitHitHideObj(name)
	{
	}

	virtual void loadAfter();
	virtual void touchFruit(THitActor*);
	virtual void countFruit(THitActor*);

public:
	/* 0x150 */ u32 unk150;
};

class TFruitBasketEvent : public TFruitBasket {
public:
	TFruitBasketEvent(const char*);

	virtual void countFruit(THitActor*);

	void reset();
	int getFruitNum(int) const;

public:
	/* 0x154 */ int mFruitCounts[5];
};

class TBreakHideObj : public THideObjBase {
public:
	TBreakHideObj(const char* name)
	    : THideObjBase(name)
	{
	}

	virtual BOOL receiveMessage(THitActor* sender, u32 message);
	virtual void control();
	virtual void kill();
	virtual void initMapObj();
};

class TWoodBox : public TBreakHideObj {
public:
	TWoodBox(const char*);

	virtual void loadAfter();
	virtual void kill();
};

class TWaterHitPictureHideObj : public THideObjBase {
public:
	TWaterHitPictureHideObj(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual BOOL receiveMessage(THitActor* sender, u32 message);
	virtual void control();
	virtual void touchActor(THitActor*);
	virtual u32 touchWater(THitActor*);
	virtual Vec* getObjAppearPos() const;
	virtual void afterFinishedAnim();
	virtual void forward(f32);

public:
	/* 0x150 */ u8 unk150;
	/* 0x154 */ f32 unk154;
	/* 0x158 */ f32 unk158;
	/* 0x15C */ f32 unk15C;
	/* 0x160 */ f32 unk160;
	/* 0x164 */ f32 unk164;
	/* 0x168 */ f32 unk168;
	/* 0x16C */ u16 unk16C;
	/* 0x16E */ u16 unk16E;
	/* 0x170 */ u16 unk170;
	/* 0x172 */ u16 unk172;
};

class THideObjPictureTwin : public TWaterHitPictureHideObj {
public:
	THideObjPictureTwin(const char*);

	virtual void loadAfter();
	virtual void afterFinishedAnim();
	virtual void initMapObj();
	virtual Vec* getObjAppearPos() const;

public:
	/* 0x174 */ TMapObjBase* unk174;
	/* 0x178 */ char unk178[0x19];
};

#endif
