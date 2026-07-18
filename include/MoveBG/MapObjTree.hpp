#ifndef MOVE_BG_MAP_OBJ_TREE_HPP
#define MOVE_BG_MAP_OBJ_TREE_HPP

#include <MoveBG/MapObjGeneral.hpp>
#include <JSystem/JGeometry.hpp>

class TMapCollisionMove;
class TMapEventSink;

class TMapObjLeaf {
public:
	TMapObjLeaf();

public:
	/* 0x00 */ f32 mAngle;
	/* 0x04 */ f32 mAngleVel;
	/* 0x08 */ TMapCollisionMove* mCollision;
	/* 0x0C */ TMtx34f mMtx;
};

class TMapObjTree : public TMapObjGeneral {
public:
	TMapObjTree(const char*);

	virtual void perform(u32, JDrama::TGraphics*);
	virtual f32 getRadiusAtY(f32) const;
	virtual void initMapObj();
	virtual void touchPlayer(THitActor*);

	int controlLeaf(int);
	void initEach();

	static f32 mBananaTreeJumpPower;

public:
	/* 0x148 */ f32 unk148;
	/* 0x14C */ f32 unk14C;
	/* 0x150 */ int mLeafCount;
	/* 0x154 */ TMapObjLeaf* mLeaves;
	/* 0x158 */ bool mIsResting;
	/* 0x15C */ f32 mMarioIsOnRotImpulse;
	/* 0x160 */ f32 mHipAttackRotImpulse;
	/* 0x164 */ f32 mSpring;
	/* 0x168 */ f32 mDamping;
	/* 0x16C */ u32 unk16C;
};

class TMapObjTreeScale : public TMapObjTree {
public:
	TMapObjTreeScale(const char*);

	virtual void loadAfter();
	virtual void control();
	virtual u32 touchWater(THitActor*);

	void startScaleUp();
	void beSmall();

	static f32 mScaleSpeedY;
	static f32 mStatusChangeScaleY;
	static f32 mScaleSpeedXZ;
	static f32 mScaleMin;

public:
	/* 0x170 */ JGeometry::TVec3<f32> mParticlePos[30];
	/* 0x2D8 */ int mParticleIndex;
	/* 0x2DC */ int mWaitTimer;
	/* 0x2E0 */ TMapEventSink* mEventSink;
};

#endif
