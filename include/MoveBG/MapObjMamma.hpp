#ifndef MOVE_BG_MAP_OBJ_MAMMA_HPP
#define MOVE_BG_MAP_OBJ_MAMMA_HPP

#include <MoveBG/MapObjBase.hpp>
#include <MoveBG/MapObjEx.hpp>

class TMapObjFlag;
class TShiningStone;
class TMapObjBall;
class TJointObj;
class TJointModel;
class TSandBase;

class TSandLeaf : public TMapObjBase {
public:
	virtual void control();
	virtual u32 touchWater(THitActor*);
	TSandLeaf(const char* name = "すなやまの芽")
	    : TMapObjBase(name)
	    , unk138(0)
	{
	}

public:
	/* 0x138 */ TSandBase* unk138;
};

class TSandBase : public TMapObjBase {
public:
	virtual BOOL grow() = 0;
	virtual BOOL withering();
	TSandBase(const char*);

	static u32 mWitherTime;
	static f32 mScaleMin;

public:
	/* 0x138 */ f32 unk138;
	/* 0x13C */ f32 unk13C;
	/* 0x140 */ u32 unk140;
	/* 0x144 */ TMapObjBase* unk144;
};

class TSandLeafBase : public TSandBase {
public:
	virtual void control();
	virtual void initMapObj();
	virtual BOOL grow();
	TSandLeafBase();
	TSandLeafBase(const char* name)
	    : TSandBase(name)
	{
	}
};

class TSandBomb : public TSandLeaf {
public:
	virtual void makeObjAppeared();
	virtual void initMapObj();
	virtual u32 getSDLModelFlag() const;
	virtual u32 touchWater(THitActor*);

	TSandBomb()
	    : TSandLeaf("すなやま爆弾")
	    , unk13C(0)
	    , unk140(0)
	{
	}

public:
	/* 0x13C */ u32 unk13C;
	/* 0x140 */ u8 unk140;
};

class TSandBombBase : public TSandBase {
public:
	virtual void control();
	virtual void initMapObj();
	virtual void loadAfter();
	virtual BOOL grow();
	virtual void waitBeforeExplode();
	virtual void explode();
	virtual void exploding();
	virtual void expanded();
	virtual void withered();
	virtual TMapObjBase* findTriggerActor();
	TSandBombBase(const char*);

	static f32 mFiringFrameSpeed;
	static f32 mFiringFrameDownSpeed;
	static f32 mExplodeFrameSpeed;
	static f32 mMarioJumpRate;
	static u32 mExlodingRumbleTime;

public:
	/* 0x148 */ s32 unk148;
	/* 0x14C */ f32 unk14C;
	/* 0x150 */ f32 unk150;
	/* 0x154 */ f32 unk154;
};

class TSandCastle : public TSandBombBase {
public:
	virtual void calcRootMatrix();
	virtual void initMapObj();
	virtual void loadAfter();
	virtual void waitBeforeExplode();
	virtual void explode();
	virtual void expanded();
	virtual BOOL withering();
	virtual TMapObjBase* findTriggerActor();
	TSandCastle(const char*);

	static f32 mCollisionRate;

public:
	/* 0x158 */ TMapObjBase* unk158;
	/* 0x15C */ u8 unk15C;
};

class TLeanMirror : public TMapObjBase {
public:
	virtual void load(JSUMemoryInputStream&);
	virtual BOOL receiveMessage(THitActor* sender, u32 message);
	virtual void control();
	virtual void initMapObj();
	virtual u32 getSDLModelFlag() const;
	virtual void draw() const;
	virtual void touchPlayer(THitActor*);
	virtual void touchEnemy(THitActor*);

	void enemyIsOn() const;
	void updateSpeedVec(const JGeometry::TVec3<f32>&, f32);
	void calcCurrentMtx(MtxPtr);
	void release();
	void controlGoTarget();
	void controlShake();
	void loadAfter();
	TLeanMirror(const char*);

	static u32 mGoTargetTime;
	static u32 mDemoWaitTime;
	static u32 mDemoLightTime;

public:
	/* 0x138 */ f32 unk138;
	/* 0x13C */ f32 unk13C;
	/* 0x140 */ JGeometry::TVec3<f32> unk140;
	/* 0x14C */ JGeometry::TVec3<f32> unk14C;
	/* 0x158 */ JGeometry::TVec3<f32> unk158;
	/* 0x164 */ JGeometry::TVec3<f32> unk164;
	/* 0x170 */ f32 unk170;
	/* 0x174 */ f32 unk174;
	/* 0x178 */ f32 unk178;
	/* 0x17C */ TShiningStone* unk17C;
	/* 0x180 */ JGeometry::TVec3<f32> unk180;
	/* 0x18C */ JGeometry::TVec3<f32> unk18C;
	/* 0x198 */ f32 unk198;
	/* 0x19C */ s32 unk19C;
	/* 0x1A0 */ JGeometry::TVec3<f32> unk1A0;
	/* 0x1AC */ u8 unk1AC;
	/* 0x1AE */ u16 unk1AE;
};

class TShiningStone : public THitActor {
public:
	void endDemo();
	void putOnLight(TLiveActor*);
	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*);
	TShiningStone(const char*);

public:
	/* 0x68 */ MActor** unk68;
	/* 0x6C */ MActor* unk6C;
	/* 0x70 */ u8 unk70;
	/* 0x71 */ u8 unk71;
	/* 0x72 */ u8 unk72;
	/* 0x73 */ u8 unk73;
	/* 0x74 */ s32 unk74;
	/* 0x78 */ JPABaseEmitter* unk78;
	/* 0x7C */ f32 unk7C;
};

class TMammaBlockRotate : public TMapObjBase {
public:
	virtual void load(JSUMemoryInputStream&);
	virtual void control();
	virtual void initMapObj();
	virtual u32 touchWater(THitActor*);
	TMammaBlockRotate(const char*);

	static f32 mRotSpeed;
	static f32 mRotReturnSpeed;
	static f32 mRotEnd;
	static f32 mMapGoSpeed;
	static f32 mMapBackSpeed;
	static u32 mWaitTime;

public:
	/* 0x138 */ TJointModel* unk138;
	/* 0x13C */ TJointObj* unk13C;
	/* 0x140 */ TJointObj* unk140;
	/* 0x144 */ TMapCollisionMove* unk144;
	/* 0x148 */ TMapCollisionMove* unk148;
};

class TMammaYacht : public TMapObjBase {
public:
	virtual void control();
	virtual void initMapObj();
	TMammaYacht()
	    : TMapObjBase("砂の城")
	{
	}

public:
	/* 0x138 */ TMapObjFlag* unk138;
};

class TSandBird : public TJointCoin {
public:
	virtual void control();
	virtual void initMapObj();
	virtual TMapObjBase* makeObjFromJointName(const char*, unsigned short);
	virtual bool nameIsObj(const char*);

	TSandBird(const char*);

public:
	/* 0x148 */ u8 unk148[0x8];
	/* 0x150 */ u8 unk150;
	/* 0x151 */ u8 unk151;
};

class TGoalWatermelon : public TMapObjBase {
public:
	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual void control();
	virtual void touchActor(THitActor*);
	TGoalWatermelon(const char*);

public:
	/* 0x138 */ TMapObjBase* unk138;
	/* 0x13C */ TMapObjBall* unk13C;
	/* 0x140 */ JGeometry::TVec3<f32> unk140;
};

class TMammaMirrorMapOperator : public JDrama::TViewObj {
public:
	void show(int);
	void hide(int);
	virtual void loadAfter();
	virtual void perform(u32, JDrama::TGraphics*);
	TMammaMirrorMapOperator(const char*);

public:
	/* 0x10 */ TMapObjBase* unk10[8];
	/* 0x30 */ JGeometry::TVec3<f32> unk30[8];
	/* 0x90 */ f32 unk90[8];
	/* 0xB0 */ u8 unkB0[8];
	/* 0xB8 */ JGeometry::TVec3<f32> unkB8[3];
};

class TSandEgg : public TMapObjBase {
public:
	virtual u32 getSDLModelFlag() const;
	TSandEgg()
	    : TMapObjBase("すなのたまご")
	{
	}
};

#endif
