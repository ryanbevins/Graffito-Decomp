#ifndef MOVE_BG_MAP_OBJ_SIRENA_HPP
#define MOVE_BG_MAP_OBJ_SIRENA_HPP

#include <MoveBG/MapObjBase.hpp>
#include <MoveBG/MapObjHide.hpp>
#include <Strategic/HitActor.hpp>

class TPictureTelesa : public TWaterHitPictureHideObj {
public:
	TPictureTelesa(const char* name)
	    : TWaterHitPictureHideObj(name)
	    , unk174(0)
	{
	}

	virtual void control();
	virtual void touchActor(THitActor*);
	virtual void afterFinishedAnim();

public:
	/* 0x174 */ u8 unk174;
};

class TChestRevolve : public TMapObjBase {
public:
	TChestRevolve(const char* name)
	    : TMapObjBase(name)
	{
	}

	virtual u32 touchWater(THitActor*);
	virtual void control();
};

class TPanelRevolve : public TMapObjBase {
public:
	TPanelRevolve(const char* name)
	    : TMapObjBase(name)
	{
	}

	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void touchPlayer(THitActor*);
	virtual void control();
};

class TMultiBtk;
class TMapCollisionMove;

class TWarpAreaActor : public THitActor {
public:
	TWarpAreaActor(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*);

public:
	/* 0x68 */ s16 unk68;
	/* 0x6A */ s16 unk6A;
};

class TSirenaCasinoRoof : public TMapObjBase {
public:
	TSirenaCasinoRoof(const char* name)
	    : TMapObjBase(name)
	    , unk138(nullptr)
	{
	}

	virtual void perform(u32, JDrama::TGraphics*);
	virtual u32 getSDLModelFlag() const;
	virtual void initMapObj();

public:
	/* 0x138 */ TMultiBtk* unk138;
};

class TSirenabossWall : public TMapObjBase {
public:
	TSirenabossWall(const char* name)
	    : TMapObjBase(name)
	    , unk138(nullptr)
	{
	}

	virtual void perform(u32, JDrama::TGraphics*);
	virtual void drawObject(JDrama::TGraphics*);
	virtual u32 getSDLModelFlag() const;
	virtual void initMapObj();

public:
	/* 0x138 */ TMultiBtk* unk138;
};

class TMapCollisionWarp;

class TSakuCasino : public TMapObjBase {
public:
	TSakuCasino(const char*);

	virtual void loadAfter();
	virtual void calcRootMatrix();
	virtual void initMapObj();

public:
	/* 0x138 */ TMapCollisionWarp* unk138;
	/* 0x13C */ u8 unk13C;
	/* 0x140 */ f32 unk140;
	/* 0x144 */ THitActor* unk144;
};

class TDonchou : public TMapObjBase {
public:
	TDonchou(const char*);

	virtual void loadAfter();
	virtual void calcRootMatrix();
	virtual void initMapObj();
	virtual u32 touchWater(THitActor*);

public:
	/* 0x138 */ TMapCollisionWarp* unk138;
	/* 0x13C */ u8 unk13C;
	/* 0x140 */ f32 unk140;
	/* 0x144 */ THitActor* unk144;
	/* 0x148 */ THitActor* unk148;
	/* 0x14C */ s32 unk14C;
};

class TSirenaRollMapObj : public TMapObjBase {
public:
	TSirenaRollMapObj(const char*);

	virtual f32 getRollAngX(int) const { return 0.0f; }
	virtual f32 getRollAngY(int) const { return 0.0f; }
	virtual f32 getRollAngZ(int) const { return 0.0f; }

public:
	/* 0x138 */ f32* unk138;
	/* 0x13C */ f32* unk13C;
	/* 0x140 */ f32 unk140;
	/* 0x144 */ f32 unk144;
	/* 0x148 */ s32 unk148;
	/* 0x14C */ f32 unk14C;
	/* 0x150 */ f32 unk150;
	/* 0x154 */ f32 unk154;
	/* 0x158 */ f32 unk158;
	/* 0x15C */ f32 unk15C;
	/* 0x160 */ f32 unk160;
	/* 0x164 */ s16 unk164;
	/* 0x166 */ u16 unk166;
};

class TCloset : public TSirenaRollMapObj {
public:
	TCloset(const char*);

	virtual void calcRootMatrix();
	virtual void initMapObj();
	virtual void moveObject();
	virtual u32 touchWater(THitActor*);
	virtual f32 getRollAngY(int i) const;

public:
	/* 0x168 */ TMapCollisionWarp* unk168;
	/* 0x16C */ u8 unk16C;
	/* 0x16D */ u8 unk16D;
};

class TCasinoPanelGate : public TSirenaRollMapObj {
public:
	TCasinoPanelGate(const char*);

	virtual void calcRootMatrix();
	virtual void initMapObj();
	virtual void moveObject();
	virtual u32 touchWater(THitActor*);
	virtual f32 getRollAngX(int i) const;

public:
	/* 0x168 */ u32 unk168;
	/* 0x16C */ u8 unk16C;
	/* 0x16D */ u8 unk16D;
};

class TSlotDrum : public TSirenaRollMapObj {
public:
	TSlotDrum(const char*);

	virtual void calcRootMatrix();
	virtual void initMapObj();
	virtual void moveObject();
	virtual u32 touchWater(THitActor*);

	virtual void initNeonMatColor();
	virtual f32 getRollAngX(int i) const;

public:
	/* 0x168 */ s32 unk168;
	/* 0x16C */ u8 unk16C;
	/* 0x16D */ u8 unk16D;
	/* 0x16E */ u8 unk16E;
	/* 0x16F */ u8 unk16F;
	/* 0x170 */ GXColorS10 unk170[3];
	/* 0x188 */ f32 unk188[3];
	/* 0x194 */ u8 unk194;
};

class TTelesaSlot : public TSlotDrum {
public:
	TTelesaSlot(const char* name)
	    : TSlotDrum(name)
	    , unk1A0(nullptr)
	    , unk1A4(0)
	{
		unk198 = 0;
		unk199 = 0;
		unk19A = 0;
		unk19B = 0;
		unk19C = 0;
		unk1A8 = 0;
		unk1A9 = 0;
		unk1AA = 0;
	}

	virtual void calcRootMatrix();
	virtual void initMapObj();
	virtual void moveObject();
	virtual u32 touchWater(THitActor*);
	virtual void initNeonMatColor() { }

	void randomReset();
	void moveStart();
	void forceStopSlot(int);
	BOOL isRollDrum();
	int getSlotResult();
	int getForcastResult(int);
	int getResultFromAng(f32);
	f32* getDrumSpeeds() const { return unk138; }

public:
	/* 0x198 */ u8 unk198;
	/* 0x199 */ u8 unk199;
	/* 0x19A */ u8 unk19A;
	/* 0x19B */ u8 unk19B;
	/* 0x19C */ u8 unk19C;
	/* 0x19D */ u8 unk19D[0x3];
	/* 0x1A0 */ void* unk1A0;
	/* 0x1A4 */ s32 unk1A4;
	/* 0x1A8 */ u8 unk1A8;
	/* 0x1A9 */ u8 unk1A9;
	/* 0x1AA */ u8 unk1AA;
	/* 0x1AB */ u8 unk1AB[0x31];
	/* 0x1DC */ TMapCollisionMove* unk1DC;
	/* 0x1E0 */ u8 unk1E0;
	/* 0x1E1 */ u8 unk1E1[0x3];
	/* 0x1E4 */ f32 unk1E4[3];
};

class TRoulette;

class TRouletteSw : public THitActor {
public:
	TRouletteSw(TRoulette* roulette, const char* name)
	    : THitActor(name)
	    , unk68(roulette)
	    , unk6C(0)
	{
	}

	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor* sender, u32 message);

public:
	/* 0x68 */ TRoulette* unk68;
	/* 0x6C */ u8 unk6C;
};

class TRoulette : public TMapObjBase {
public:
	TRoulette(const char*);

	virtual void perform(u32, JDrama::TGraphics*);
	virtual void calcRootMatrix();
	virtual void initMapObj();
	virtual void moveObject();
	virtual void setRollSp(f32);

	void switchStop();

public:
	/* 0x138 */ f32 unk138;
	/* 0x13C */ f32 unk13C;
	/* 0x140 */ u8 unk140;
	/* 0x141 */ u8 unk141;
	/* 0x142 */ u8 unk142;
	/* 0x143 */ u8 unk143;
	/* 0x144 */ f32 unk144;
	/* 0x148 */ GXColorS10 unk148;
	/* 0x150 */ TRouletteSw* unk150;
};

class TItemSlotDrum : public TSlotDrum {
public:
	TItemSlotDrum(const char*);

	virtual void loadAfter();
	virtual void calcRootMatrix();
	virtual void moveObject();
	virtual u32 touchWater(THitActor*);

	void generateItem();
	int getForcastResult(int);
	int getResultFromAng(f32);

public:
	/* 0x198 */ u32 unk198;
	/* 0x19C */ u8 unk19C;
	/* 0x19D */ u8 unk19D;
	/* 0x19E */ u8 unk19E;
	/* 0x19F */ u8 unk19F;
	/* 0x1A0 */ u8 unk1A0;
	/* 0x1A1 */ u8 unk1A1;
	/* 0x1A2 */ u8 unk1A2;
	/* 0x1A3 */ u8 unk1A3;
	/* 0x1A4 */ u32 unk1A4;
	/* 0x1A8 */ f32 unk1A8;
};

#endif
