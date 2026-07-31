#ifndef MOVE_BG_MAP_OBJ_PINNA_HPP
#define MOVE_BG_MAP_OBJ_PINNA_HPP

#include <MoveBG/MapObjBase.hpp>
#include <MoveBG/MapObjGeneral.hpp>
#include <MoveBG/MapObjTown.hpp>

// TODO: mark virtual methods as such

class TCoin;

class TFerrisWheel : public TMapObjBase {
public:
	static s32 becomeCalmlyCallback(u32, u32);
	void control();
	void initMapObj();
	TFerrisWheel(const char*);

public:
	/* 0x138 */ s32 unk138;
	/* 0x13C */ TMapObjBase** unk13C;
	/* 0x140 */ f32 unk140;
};

class THorizontalViking : public TMapObjBase {
public:
	void updateTrans();
	void moveNormal();
	void control();
	virtual void reset();
	void initMapObj();
	THorizontalViking(const char* name)
	    : TMapObjBase(name)
	    , unk138(0.0f)
	    , unk13C(0.0f)
	    , unk140(0.0f)
	    , unk144(0.0f)
	    , unk148(0.0f)
	{
	}

public:
	/* 0x138 */ f32 unk138;
	/* 0x13C */ f32 unk13C;
	/* 0x140 */ f32 unk140;
	/* 0x144 */ f32 unk144;
	/* 0x148 */ f32 unk148;
};

class TViking : public THorizontalViking {
public:
	void roll();
	void control();
	virtual void reset();
	void loadAfter();
	void initMapObj();
	TViking(const char*);

public:
	/* 0x14C */ s32 unk14C;
	/* 0x150 */ f32 unk150;
	/* 0x154 */ f32 unk154;
	/* 0x158 */ f32 unk158;
};

class TPinnaShell : public THitActor {
public:
	void opened();
	BOOL receiveMessage(THitActor* sender, u32 message);
	void control();
	TPinnaShell(const char*);
	TPinnaShell();

public:
	/* 0x68 */ s32 unk68;
	/* 0x6C */ f32 unk6C;
	/* 0x70 */ f32 unk70;
	/* 0x74 */ MtxPtr unk74;
	/* 0x78 */ J3DJoint* unk78;
	/* 0x7C */ s32 unk7C;
	/* 0x80 */ THitActor* unk80;
	/* 0x84 */ TMapCollisionMove* unk84;
	/* 0x88 */ TDamageObj* unk88;
	/* 0x8C */ TMapObjBase* unk8C;
};

class TShellCup : public TMapObjBase {
public:
	void control();
	void attachCoin(TCoin*, int);
	void calcAfter();
	void perform(u32, JDrama::TGraphics*);
	void loadAfter();
	void initMapObj();
	TShellCup(const char*);

	static f32 mOpenRotMax;
	static f32 mShellDamageRot;
	static f32 mWaterOpenAccel;
	static f32 mCloseAccel;

public:
	/* 0x138 */ TPinnaShell unk138[6];
	/* 0x498 */ TCoin* unk498;
	/* 0x49C */ TCoin* unk49C;
	/* 0x4A0 */ TCoin* unk4A0;
};

class TMerrygoround : public TMapObjBase {
public:
	void control();
	void draw() const;
	void initMapObj();
	TMerrygoround(const char*);

	static f32 mRotSpeed;

public:
	/* 0x138 */ TMapObjBase* unk138[2];
	/* 0x140 */ u16 unk140[2];
	/* 0x144 */ TMapObjBase* unk144[9];
	/* 0x168 */ TMapCollisionMove* unk168[9];
	/* 0x18C */ u16 unk18C[9];
	/* 0x1A0 */ TMapObjBase* unk1A0;
	/* 0x1A4 */ u16 unk1A4;
};

class TChangeStageMerrygoround : public TMapObjChangeStage {
public:
	void touchPlayer(THitActor*);
	void calc();

	TChangeStageMerrygoround()
	    : TMapObjChangeStage("ステージ切り替え（メリーゴーランド用）")
	    , unk13C(0)
	{
	}

public:
	/* 0x13C */ u8 unk13C;
};

class TBalloonKoopaJr : public TMapObjGeneral {
public:
	void touchActor(THitActor*);
	void kill();
	void load(JSUMemoryInputStream&);
	TBalloonKoopaJr()
	    : TMapObjGeneral("風船（クッパＪｒ）")
	    , unk148(0.0f, 0.0f, 0.0f)
	{
	}

public:
	/* 0x148 */ JGeometry::TVec3<f32> unk148;
};

class TPinnaEntrance : public TMapObjBase {
public:
	void loadAfter();
	TPinnaEntrance()
	    : TMapObjBase("ピンナ入り口")
	{
	}
};

class TWaterRecoverObj : public TMapObjBase {
public:
	void touchPlayer(THitActor*);
	TWaterRecoverObj()
	    : TMapObjBase("水回復オブジェ")
	{
	}
};

class TAmiKing : public TMapObjBase {
public:
	u32 touchWater(THitActor*);
	void loadAfter();
	void initMapObj();
	void moveObject();
	void calcRootMatrix();
	void bind();
	void touchPlayer(THitActor*);
	TAmiKing()
	    : TMapObjBase("アミキング")
	    , unk138(0)
	{
	}

public:
	/* 0x138 */ u8 unk138;
	/* 0x13C */ JGeometry::TVec3<f32> unk13C;
};

class TPinnaCoaster : public TMapObjBase {
public:
	void control();
	void initMapObj();
	TPinnaCoaster(const char*);

public:
	/* 0x138 */ MActor* unk138;
	/* 0x13C */ u32 unk13C;
	/* 0x140 */ JGeometry::TVec3<f32> unk140;
};

class TMerryPole : public TMapObjBase {
public:
	virtual Mtx* getRootJointMtx() const { return (Mtx*)unk138.mMtx; }

	TMerryPole()
	    : TMapObjBase("メリーゴーランド用ポール")
	{
		unk138.mMtx[1][0] = unk138.mMtx[2][0] = unk138.mMtx[0][1]
		    = unk138.mMtx[2][1] = unk138.mMtx[0][2] = unk138.mMtx[1][2]
		    = unk138.mMtx[0][3] = unk138.mMtx[1][3] = unk138.mMtx[2][3] = 0.0f;

		unk138.mMtx[0][0] = unk138.mMtx[1][1] = unk138.mMtx[2][2] = 1.0f;
	}

public:
	/* 0x138 */ TPosition3f unk138;
};

#endif
