#ifndef MOVE_BG_MAP_OBJ_CORONA_HPP
#define MOVE_BG_MAP_OBJ_CORONA_HPP

#include <MoveBG/MapObjBase.hpp>
#include <System/ParamInst.hpp>
#include <System/Params.hpp>

class TBathtub;
class TBathtubGrip;
class MActor;
class MActorAnmData;
class TMapCollisionMove;

class TBathtubGripParts : public TLiveActor {
public:
	TBathtubGripParts(TBathtubGrip* grip, s32 index, const char* name)
	    : TLiveActor(name)
	    , unkF4(grip)
	    , unkF8(index)
	{
	}

	virtual Mtx* getRootJointMtx() const;

public:
	/* 0xF4 */ TBathtubGrip* unkF4;
	/* 0xF8 */ s32 unkF8;
};

class TBathtubGripPartsHard : public TBathtubGripParts {
public:
	TBathtubGripPartsHard(TBathtubGrip* grip, s32 index, const char* name)
	    : TBathtubGripParts(grip, index, name)
	{
	}

	virtual BOOL receiveMessage(THitActor* sender, u32 message);
};

class TBathtubGripPartsFragile : public TBathtubGripParts {
public:
	TBathtubGripPartsFragile(TBathtubGrip* grip, s32 index, const char* name)
	    : TBathtubGripParts(grip, index, name)
	{
	}

	virtual BOOL receiveMessage(THitActor* sender, u32 message);
};

class TBathtubParams : public TParams {
public:
	TBathtubParams();

	/* 0x008 */ TParamRT<u8> resetGrip;
	/* 0x01C */ TParamRT<s32> trampleRelease;
	/* 0x030 */ TParamRT<s32> trampleRecover;
	/* 0x044 */ TParamRT<s32> quakeRelease;
	/* 0x058 */ TParamRT<s32> quakeRecover;
	/* 0x06C */ TParamRT<s32> hipdropRelease;
	/* 0x080 */ TParamRT<s32> hipdropRecover;
	/* 0x094 */ TParamRT<s32> breakCount0;
	/* 0x0A8 */ TParamRT<s32> breakCount1;
	/* 0x0BC */ TParamRT<s32> breakCount2;
	/* 0x0D0 */ TParamRT<s32> breakCount3;
	/* 0x0E4 */ TParamRT<s32> launchStopCount;
	/* 0x0F8 */ TParamRT<f32> animSpeed0;
	/* 0x10C */ TParamRT<f32> animSpeed1;
	/* 0x120 */ TParamRT<f32> animSpeed2;
	/* 0x134 */ TParamRT<f32> animSpeed3;
	/* 0x148 */ TParamRT<f32> animSpeed4;
	/* 0x15C */ TParamRT<f32> shake;
	/* 0x170 */ TParamRT<f32> watermark;
	/* 0x184 */ TParamRT<f32> maxAngle;
	/* 0x198 */ TParamRT<f32> angleVelDamp;
	/* 0x1AC */ TParamRT<f32> rebound;
	/* 0x1C0 */ TParamRT<f32> shakeDamp;
	/* 0x1D4 */ TParamRT<f32> marioWeight;
	/* 0x1E8 */ TParamRT<f32> marioDropWeight;
	/* 0x1FC */ TParamRT<f32> outerHeight;
};

class TBathtubGrip : public TMapObjBase {
public:
	TBathtubGrip(TBathtub*, f32, MActorAnmData*, const char*);

	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor* sender, u32 message);
	virtual Mtx* getRootJointMtx() const;
	virtual void calcRootMatrix();
	virtual void control();
	virtual void kill();

public:
	/* 0x138 */ JGeometry::TVec3<f32> unk138;
	/* 0x144 */ JGeometry::TVec3<f32> unk144;
	/* 0x150 */ TMapCollisionMove* unk150[5];
	/* 0x164 */ TMapCollisionMove* unk164[17];
	/* 0x1A8 */ TBathtubGripPartsFragile* unk1A8[5];
	/* 0x1BC */ TBathtubGripPartsHard* unk1BC[17];
	/* 0x200 */ s32 unk200[17];
	/* 0x244 */ TBathtub* unk244;
	/* 0x248 */ u8 unk248;
	/* 0x249 */ u8 unk249;
	/* 0x24A */ u8 unk24A;
	/* 0x24B */ u8 unk24B;
	/* 0x24C */ f32 unk24C;
	/* 0x250 */ f32 unk250;
	/* 0x254 */ s32 unk254;
	/* 0x258 */ s32 unk258;
	/* 0x25C */ MActor* unk25C;
	/* 0x260 */ u8 unk260;
	/* 0x261 */ u8 unk261[3];
};

class TBathtub : public TMapObjBase {
public:
	TBathtub(const char* name = "バスタブ");

	void loadAfter();
	void hipdrop(const JGeometry::TVec3<f32>&);
	void quake(const JGeometry::TVec3<f32>&);
	s32 getNumGripsDead() const;
	void tumble(f32, f32);
	MtxPtr getTakingMtx();
	MtxPtr getSubmarineMtxInDemo();
	MtxPtr getPeachMtxInDemo();
	MtxPtr getKoopaJrMtxInDemo();
	// fabricated
	u8 getUnk29A() const { return *((u8*)this + 0x29A); }
	u8 getUnk1D4() const { return unk1D4; }
	BOOL receiveMessage(THitActor* sender, u32 message);
	Mtx* getRootJointMtx() const;
	void perform(u32, JDrama::TGraphics*);
	void control();
	void calcBathtubData();
	void setupCollisions_();
	void removeCollisions_(); // Unused
	void startDemo();
	bool allowsTumble() const;
	void calcRootMatrix();
	bool getNearGrip(const JGeometry::TVec3<f32>&, f32, f32*) const;
	u8 getNextJuncture(const JGeometry::TVec3<f32>&,
	                   const JGeometry::TVec3<f32>&) const;
	u8 getNextGrip(const JGeometry::TVec3<f32>&, const JGeometry::TVec3<f32>&,
	               f32, f32*) const;
	void updatePosture_();
	void load(JSUMemoryInputStream&);
	s32 getNumKillerLaunchable() const;
	bool isKillerAttackable() const;
	s32 getNumKillerBurstable() const;
	bool isBreaking() const;                                // Unused
	bool isKillerLaunchable() const;                        // Unused
	void showMessage(u32);                                  // Unused
	u8 getNearJuncture(const JGeometry::TVec3<f32>&) const; // Unused
	MtxPtr getKoopaMtxInDemo();                             // Unused
	MtxPtr getWaterMtx(s32);                                // Unused
	MtxPtr getShineEffectMtx();                             // Unused
	MtxPtr getShineMtx();                                   // Unused
	void liftMario(const JGeometry::TVec3<f32>&);           // Unused
	void trample(const JGeometry::TVec3<f32>&);             // Unused

public:
	/* 0x138 */ MActorAnmData* unk138;
	/* 0x13C */ f32 unk13C[5];
	/* 0x150 */ f32 unk150[5];
	/* 0x164 */ TMapCollisionMove** unk164;
	/* 0x168 */ TBathtubGrip** unk168;
	/* 0x16C */ TBathtubParams* unk16C;
	/* 0x170 */ JGeometry::TVec3<f32> unk170;
	/* 0x17C */ JGeometry::TVec3<f32> unk17C;
	/* 0x188 */ f32 unk188[9];
	/* 0x1AC */ f32 unk1AC;
	/* 0x1B0 */ f32 unk1B0;
	/* 0x1B4 */ f32 unk1B4;
	/* 0x1B8 */ f32 unk1B8;
	/* 0x1BC */ JGeometry::TVec3<f32> unk1BC;
	/* 0x1C8 */ JGeometry::TVec3<f32> unk1C8;
	/* 0x1D4 */ u8 unk1D4;
	/* 0x1D5 */ u8 unk1D5[3];
	/* 0x1D8 */ JGeometry::TVec3<f32> unk1D8;
	/* 0x1E4 */ f32 unk1E4;
	/* 0x1E8 */ JGeometry::TVec3<f32> unk1E8;
	/* 0x1F4 */ u8 unk1F4[0x48];
	/* 0x23C */ JGeometry::TVec3<f32> unk23C;
	/* 0x248 */ s32 unk248;
	/* 0x24C */ s32 unk24C;
	/* 0x250 */ s32 unk250;
	/* 0x254 */ s32 unk254;
	/* 0x258 */ s32 unk258;
	/* 0x25C */ s32 unk25C;
	/* 0x260 */ s32 unk260;
	/* 0x264 */ s32 unk264;
	/* 0x268 */ s32 unk268;
	/* 0x26C */ s32 unk26C;
	/* 0x270 */ s32 unk270;
	/* 0x274 */ s32 unk274;
	/* 0x278 */ s32 unk278;
	/* 0x27C */ s32 unk27C;
	/* 0x280 */ s32 unk280;
	/* 0x284 */ s32 unk284;
	/* 0x288 */ s32 unk288;
	/* 0x28C */ s32 unk28C;
	/* 0x290 */ s32 unk290;
	/* 0x294 */ s32 unk294;
	/* 0x298 */ u8 unk298;
	/* 0x299 */ u8 unk299;
	/* 0x29A */ u8 unk29A;
	/* 0x29B */ u8 unk29B;
	/* 0x29C */ MActor* unk29C;
	/* 0x2A0 */ u32 unk2A0;
};

#endif
