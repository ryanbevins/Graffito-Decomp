#ifndef MARIO_UTIL_SHADOW_UTIL_HPP
#define MARIO_UTIL_SHADOW_UTIL_HPP

#include <JSystem/JDrama/JDRViewObj.hpp>
#include <dolphin/gx/GXStruct.h>

class THitActor;
class J3DModel;
class SDLModelData;
class TMBindShadowParts;

class TCircleShadowRequest {
public:
	TCircleShadowRequest()
	    : unkC(0.0f)
	    , unk10(0.0f)
	    , unk14(0.0f)
	    , unk18(0.0f)
	    , unk1C(0)
	    , unk1D(1)
	    , unk20(0)
	{
		unk0.set(0.0f, 0.0f, 0.0f);
	}

public:
	/* 0x0 */ JGeometry::TVec3<f32> unk0;
	/* 0xC */ f32 unkC;
	/* 0x10 */ f32 unk10;
	/* 0x14 */ f32 unk14;
	/* 0x18 */ f32 unk18;
	/* 0x1C */ u8 unk1C;
	/* 0x1D */ u8 unk1D;
	/* 0x20 */ u32 unk20;
};

class TAlphaShadowBlendQuad {
public:
	TAlphaShadowBlendQuad()
	{
		unk0.set(-1.0f, -1.0f, -1.0f);
		unkC.set(1.0f, 1.0f, 1.0f);
		unk18 = 0;
		unk1C = 0;
	}

public:
	/* 0x0 */ JGeometry::TVec3<f32> unk0;
	/* 0xC */ JGeometry::TVec3<f32> unkC;
	/* 0x18 */ u32 unk18;
	/* 0x1C */ u32 unk1C;
};

class TAlphaShadowQuadAry {
public:
	TAlphaShadowQuadAry()
	{
		unk4  = 0;
		unk8  = 0;
		unkC  = 0;
		unk10 = 0;
	}

public:
	/* 0x0 */ u32 unk0;
	/* 0x4 */ u32 unk4;
	/* 0x8 */ u32 unk8;
	/* 0xC */ u32 unkC;
	/* 0x10 */ u32 unk10;
};

class TMBindShadowBody {
public:
	TMBindShadowBody(THitActor*, J3DModel*, float);
	bool isUseThisJoint(int);
	bool isCircleJoint(int);
	bool isBodyJoint(int);
	void entryDrawShadow();
	void calc();

public:
	/* 0x0 */ TMBindShadowParts** unk0;
	/* 0x4 */ THitActor* unk4;
	/* 0x8 */ s32 unk8;
	/* 0xC */ const char* unkC;
	/* 0x10 */ f32 unk10;
	/* 0x14 */ f32 unk14;
	/* 0x18 */ f32 unk18;
};

class TAlphaShadowQuad {
public:
	TAlphaShadowQuad()
	{
		unk0  = 0.01f;
		unk64 = 0;
		unk68 = 0;
		unk6C = 0;
	}

public:
	/* 0x0 */ f32 unk0;
	/* 0x4 */ u8 unk4[0x60];
	/* 0x64 */ u32 unk64;
	/* 0x68 */ u32 unk68;
	/* 0x6C */ u32 unk6C;
};

class TModelShadowInfo {
public:
	TModelShadowInfo();

public:
	/* 0x0 */ JGeometry::TVec3<f32> unk0;
	/* 0xC */ u8 unkC;
	/* 0xD */ u8 unkD;
	/* 0x10 */ f32 unk10;
};

class TSquareShadowInfo {
public:
	TSquareShadowInfo();

public:
	/* 0x0 */ Vec unk0[5];
};

class TMBindShadowParts {
public:
	TMBindShadowParts(J3DModel*, u8, TMBindShadowBody*, f32);
	void calc(f32);

public:
	/* 0x0 */ f32 unk0;
	/* 0x4 */ TMBindShadowBody* unk4;
	/* 0x8 */ const char* unk8;
	/* 0xC */ MtxPtr unkC;
	/* 0x10 */ MtxPtr unk10;
	/* 0x14 */ u8 unk14;
	/* 0x15 */ u8 unk15;
	/* 0x16 */ u8 unk16;
};

class TMBindShadowManager;

extern TMBindShadowManager* gpBindShadowManager;

class TMBindShadowManager : public JDrama::TViewObj {
public:
	TMBindShadowManager(const char* name = "<TMBindShadowManager>");

	virtual void load(JSUMemoryInputStream& stream);
	virtual void perform(u32, JDrama::TGraphics*);

	void reset();
	void initEntry(TMBindShadowBody*);
	void drawShadowVolume(bool, TAlphaShadowQuad*);
	void drawShadowGD(u32, JDrama::TGraphics*);
	void drawShadow(u32, JDrama::TGraphics*);
	void request(const TCircleShadowRequest&, u32);
	void forceRequest(const TCircleShadowRequest&, u32);
	void calcVtx();

public:
	static f32 mSquareShadowHeight;
	static f32 mTreeScale;
	static f32 mYScalePlus;
	static f32 mJoinDist;
	static u8 mTestSw;
	static u8 mDLSw;

	/* 0x10 */ TCircleShadowRequest* unk10;
	/* 0x14 */ u32 unk14;
	/* 0x18 */ TAlphaShadowQuad* unk18;
	/* 0x1C */ TAlphaShadowQuadAry* unk1C;
	/* 0x20 */ u32 unk20;
	/* 0x24 */ TAlphaShadowBlendQuad* unk24;
	/* 0x28 */ TSquareShadowInfo* unk28;
	/* 0x2C */ u32 unk2C;
	/* 0x30 */ JGeometry::TVec3<f32> unk30;
	/* 0x3C */ SDLModelData** unk3C;
	/* 0x40 */ u16 unk40;
	/* 0x44 */ u32 unk44;
	/* 0x48 */ u8 unk48;
	/* 0x49 */ u8 unk49;
	/* 0x4C */ JGadget::TList<TMBindShadowBody*> unk4C;
	/* 0x5C */ GXColor unk5C;
	/* 0x60 */ f32 unk60;
	/* 0x64 */ u8 unk64;
	/* 0x65 */ u8 unk65;
	/* 0x68 */ f32 unk68;
	/* 0x6C */ f32 unk6C;
	/* 0x70 */ TModelShadowInfo* unk70;
};

#endif
