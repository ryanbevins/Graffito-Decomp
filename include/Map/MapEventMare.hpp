#ifndef MAP_MAP_EVENT_MARE_HPP
#define MAP_MAP_EVENT_MARE_HPP

#include <JSystem/JDrama/JDRViewObj.hpp>
#include <JSystem/JGeometry.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <MoveBG/MapObjBase.hpp>

class J3DJoint;
class TJointObj;

class TMareEventBumpyWall : public TMapObjBase {
public:
	TMareEventBumpyWall(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void control();
	virtual u32 touchWater(THitActor*);

	void bumpUpX();
	void bumpDownX();
	void bumpUpZ();
	void bumpDownZ();

public:
	/* 0x138 */ int mBuildingIndex;
	/* 0x13C */ J3DJoint* mJoint;
	/* 0x140 */ f32 mBumpSpeed;
	/* 0x144 */ f32 mBumpLimit;
	/* 0x148 */ TMapCollisionWarp* mWarpCollision;
	/* 0x14C */ TMapCollisionMove* mMoveCollision;
	/* 0x150 */ int mBumpDirection;
};

class TMareEventDepressWall : public JDrama::TViewObj {
public:
	TMareEventDepressWall(const char*);

	virtual void perform(u32, JDrama::TGraphics*);

	void init1stEvent();
	void init2ndEvent();
	void init3rdEvent();
	void initCommon();
	bool startEvent();
	void depressing();
	void rising();

	static f32 mDepressSpeed;
	static f32 mRiseSpeed;
	static int mWaitTimeToWatch;

public:
	/* 0x10 */ int mWallNum;
	/* 0x14 */ int mStartBuildingIndex;
	/* 0x18 */ int* mWaitTimes;
	/* 0x1C */ bool* mDirections;
	/* 0x20 */ f32* mTargets;
	/* 0x24 */ TMapCollisionWarp* mWarpCollisions;
	/* 0x28 */ TMapCollisionMove* mMoveCollisions;
	/* 0x2C */ u32 unk2C;
	/* 0x30 */ J3DJoint** mJoints;
	/* 0x34 */ JGeometry::TVec3<f32>* mPositions;
	/* 0x38 */ JGeometry::TVec3<f32>* mEffectDirs;
	/* 0x3C */ f32* mParticleScales;
	/* 0x40 */ f32* mParticleChildRates;
	/* 0x44 */ int mState;
	/* 0x48 */ int mCurrentIndex;
	/* 0x4C */ int mWaitTimer;
};

class TMareWallRock : public TLiveActor {
public:
	TMareWallRock();

	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual void perform(u32, JDrama::TGraphics*);

	void initEffect();
	void movement();
	void appear();

	static f32 mAppearSpeed;
	static f32 mDepressSpeed;
	static u32 mCleanedDegree;
	static int mWaitTimeToAppear;
	static int mWaitTimeToDepress;

public:
	/* 0x0F4 */ int mState;
	/* 0x0F8 */ int mIndex;
	/* 0x0FC */ f32 mSinkDepth;
	/* 0x100 */ int mTimer;
	/* 0x104 */ TJointObj* mJointObj;
	/* 0x108 */ int mLayerIndex;
	/* 0x10C */ TMapCollisionBase** mCollisions;
	/* 0x110 */ JGeometry::TVec3<f32> mEffectScale;
	/* 0x11C */ JGeometry::TVec3<f32> mEffectPos;
	/* 0x128 */ f32 mEffectRotY;
};

class TMareEventWallRock : public JDrama::TViewObj {
public:
	TMareEventWallRock(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*) { }

public:
	/* 0x10 */ int mRockNum;
	/* 0x14 */ TMareWallRock* mRocks;
};

#endif
