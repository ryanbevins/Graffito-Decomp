#ifndef MOVE_BG_MAP_OBJ_MONTE_HPP
#define MOVE_BG_MAP_OBJ_MONTE_HPP

#include <MoveBG/MapObjBase.hpp>
#include <MoveBG/MapObjBlock.hpp>

// TODO: mark virtual methods as such

class JAISound;
class THangingBridge;
class TFluffManager;

class TMapObjMonteRoot : public TMapObjBase {
public:
	void initMapObj();
	TMapObjMonteRoot()
	    : TMapObjBase("根っこ")
	{
	}
};

class TJumpMushroom : public TMapObjBase {
public:
	BOOL receiveMessage(THitActor*, unsigned long);
	void load(JSUMemoryInputStream&);
	TJumpMushroom()
	    : TMapObjBase("ジャンプきのこ")
	{
	}
};

class THangingBridgeBoard : public TLeanBlock {
public:
	void drawOneRope(const JGeometry::TVec3<f32>&) const;
	void drawRopes() const;
	void push(f32);
	void pushNeighbor(f32);
	void control();
	void calcDefaultMtx();
	void setGroundCollision();
	void initMapObj();
	THangingBridgeBoard(const char*);

	static f32 mMarioAccelY;
	static f32 mMarioHipDropAccelY;
	static f32 mReturnAccelRate;
	static f32 mSpeedDownRate;
	static f32 mRopeWidthX;
	static f32 mRopeWidthZ;
	static f32 mTexPosRate;

public:
	/* 0x194 */ THangingBridgeBoard* unk194;
	/* 0x198 */ THangingBridgeBoard* unk198;
	/* 0x19C */ THangingBridgeBoard* unk19C;
	/* 0x1A0 */ THangingBridgeBoard* unk1A0;
	/* 0x1A4 */ JGeometry::TVec3<f32> unk1A4[2];
	/* 0x1BC */ THangingBridge* unk1BC;
};

class THangingBridge : public JDrama::TViewObj {
public:
	void drawLowerMinus(const JGeometry::TVec3<f32>&,
	                    const JGeometry::TVec3<f32>&,
	                    const JGeometry::TVec2<f32>&, int) const;
	void drawLowerPlus(const JGeometry::TVec3<f32>&,
	                   const JGeometry::TVec3<f32>&,
	                   const JGeometry::TVec2<f32>&, int) const;
	void drawUpper(const JGeometry::TVec3<f32>&, const JGeometry::TVec3<f32>&,
	               const JGeometry::TVec2<f32>&, int) const;
	void setDrawPos(int, f32, JGeometry::TVec3<f32>*) const;
	void drawRopeBetweenBoards(f32, int) const;
	void initDraw() const;
	void perform(unsigned long, JDrama::TGraphics*);
	void initMonte();
	void loadAfter();
	THangingBridge(const char*);

	static f32 mRopeWidthBetweenBoards;
	static f32 mRopeWidthBetweenBoardsY;
	static s32 mPointNumBetweenBoards;
	static f32 mBetweenBoardsTexPosRate;
	static f32 mRopeHeight;

public:
	/* 0x10 */ s32 unk10;
	/* 0x14 */ THangingBridgeBoard** unk14;
	/* 0x18 */ JGeometry::TVec3<f32> unk18;
	/* 0x24 */ JGeometry::TVec3<f32> unk24;
	/* 0x30 */ JGeometry::TVec2<f32> unk30;
	/* 0x38 */ f32* unk38;
	/* 0x3C */ JGeometry::TVec3<f32> unk3C;
};

class TSwingBoard : public TMapObjBase {
public:
	void drawOneRope(const JGeometry::TVec3<f32>&,
	                 const JGeometry::TVec3<f32>&) const;
	void initDraw() const;
	void draw() const;
	void swing();
	void control();
	void load(JSUMemoryInputStream&);
	TSwingBoard(const char*);

	static f32 mBoardWidth;
	static f32 mRopeWidthX;
	static f32 mRopeWidthZ;
	static f32 mTexPosRate;
	static f32 mReturnAccelRate;
	static f32 mSpeedDownRate;

public:
	/* 0x138 */ f32 unk138;
	/* 0x13C */ f32 unk13C;
	/* 0x140 */ f32 unk140;
	/* 0x144 */ f32 unk144;
	/* 0x148 */ f32 unk148;
	/* 0x14C */ Mtx unk14C;
	/* 0x17C */ JGeometry::TVec3<f32> unk17C;
	/* 0x188 */ JAISound* unk188;
};

class TGoalFlag : public TMapObjBase {
public:
	f32 getRadiusAtY(f32) const;
	void touchActor(THitActor*);
	void initMapObj();
	TGoalFlag()
	    : TMapObjBase("ゴールフラグ")
	{
	}
};

class TFluff : public TMapObjBase {
public:
	f32 getRadiusAtY(f32) const;
	u32 touchWater(THitActor*);
	void move();
	void kill();
	void control();
	void appear();
	void initMapObj();
	TFluff(const char*);

	static f32 mScaleUpSpeed;
	static f32 mScaleDownSpeed;

public:
	/* 0x138 */ f32 unk138;
	/* 0x13C */ f32 unk13C;
	/* 0x140 */ f32 unk140;
	/* 0x144 */ f32 unk144;
	/* 0x148 */ f32 unk148;
	/* 0x14C */ f32 unk14C;
	/* 0x150 */ f32 unk150;
	/* 0x154 */ JGeometry::TVec3<f32> unk154;
	/* 0x160 */ f32 unk160;
	/* 0x164 */ f32 unk164;
	/* 0x168 */ TFluffManager* unk168;
	/* 0x16C */ u8 unk16C;
};

class TFluffManager : public TMapObjBase {
public:
	void findNextFluff();
	void control();
	void registerNextFluff(TFluff*);
	void setUpNextFluff();
	void newFluff(const char*);
	f32 getRandomX() const;
	f32 getRandomZ() const;
	void loadAfter();
	void load(JSUMemoryInputStream&);
	TFluffManager(const char*);

	static f32 mWindMin;

public:
	/* 0x138 */ JGeometry::TVec3<f32> unk138;
	/* 0x144 */ s32 unk144;
	/* 0x148 */ JGeometry::TVec3<f32> unk148;
	/* 0x154 */ f32 unk154;
	/* 0x158 */ TFluff* unk158;
	/* 0x15C */ TFluff* unk15C;
	/* 0x160 */ s32 unk160;
	/* 0x164 */ s32 unk164;
	/* 0x168 */ TFluff** unk168;
};

#endif
