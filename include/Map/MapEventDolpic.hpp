#ifndef MAP_MAP_EVENT_DOLPIC_HPP
#define MAP_MAP_EVENT_DOLPIC_HPP

#include <Map/MapEvent.hpp>

class J3DJoint;
class TMapCollisionMove;
class TMapCollisionWarp;
class TBiancoGateKeeper;

class TDolpicEventRiccoMammaGate : public TMapEvent {
public:
	TDolpicEventRiccoMammaGate(const char* name = "イベント（リコゲート）");

	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual bool watch();
	virtual bool control();
	virtual bool isFinishedAll() const;

public:
	/* 0x20 */ J3DJoint* unk20;
	/* 0x24 */ TMapCollisionMove* unk24;
	/* 0x28 */ TMapCollisionWarp* unk28;
	/* 0x2C */ u32 unk2C;
	/* 0x30 */ bool unk30;
	/* 0x34 */ f32 unk34;
	/* 0x38 */ int unk38;
	/* 0x3C */ int unk3C;
	/* 0x40 */ int unk40;
	/* 0x44 */ int unk44;
	/* 0x48 */ JGeometry::TVec3<f32> mPos;
	/* 0x54 */ JGeometry::TVec3<f32> mWarpPos;
	/* 0x60 */ f32 mWarpY;
};

class TDolpicEventBiancoGate : public TMapEvent {
public:
	TDolpicEventBiancoGate(const char* name = "イベント（ビアンコゲート）");

	virtual void loadAfter();
	virtual bool watch();
	virtual bool control();
	virtual bool isFinishedAll() const;

public:
	/* 0x20 */ TBiancoGateKeeper* unk20;
	/* 0x24 */ f32 unk24;
};

#endif
