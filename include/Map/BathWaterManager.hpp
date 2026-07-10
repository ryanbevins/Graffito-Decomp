#ifndef MAP_BATH_WATER_MANAGER_HPP
#define MAP_BATH_WATER_MANAGER_HPP

#include <Strategic/HitActor.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JGeometry.hpp>
#include <System/ParamInst.hpp>
#include <System/Params.hpp>
#include <dolphin/gx.h>

class J3DModel;
class J3DModelData;
class JUTTexture;
class TBathtub;

class TBathtubData {
public:
	JGeometry::TVec3<f32> getGravityDir(f32) const;
	JGeometry::TVec3<f32> getPos(int, int, f32) const;
	JGeometry::TVec3<f32> getThing() const
	{
		return JGeometry::TVec3<f32>(unk0.x, unk0.y - unk44, unk0.z);
	}

public:
	/* 0x00 */ JGeometry::TVec3<f32> unk0;
	/* 0x0C */ JGeometry::TVec3<f32> unkC;
	/* 0x18 */ JGeometry::TVec3<f32> unk18;
	/* 0x24 */ JGeometry::TVec3<f32> unk24;
	/* 0x30 */ JGeometry::TVec3<f32> unk30;
	/* 0x3C */ f32 unk3C;
	/* 0x40 */ f32 unk40;
	/* 0x44 */ f32 unk44;
	/* 0x48 */ f32 unk48;
	/* 0x4C */ u8 unk4C[0x58 - 0x4C];
	/* 0x58 */ JGeometry::TVec3<f32> unk58;
	/* 0x64 */ u8 unk64;
	/* 0x65 */ u8 unk65;
};

class TBathWaterParams : public TParams {
public:
	TBathWaterParams(const char*);

public:
	/* 0x008 */ TParamRT<u8> suppliesDrops;
	/* 0x01C */ TParamRT<u8> bathtubGravity;
	/* 0x030 */ TParamRT<u8> intersects;
	/* 0x044 */ TParamRT<u8> isVisible;
	/* 0x058 */ TParamRT<u8> checksMario;
	/* 0x06C */ TParamRT<s32> numDrops;
	/* 0x080 */ TParamRT<f32> dropRadius;
	/* 0x094 */ TParamRT<f32> texScale;
	/* 0x0A8 */ TParamRT<f32> hitScale;
	/* 0x0BC */ TParamRT<f32> modelScale;
	/* 0x0D0 */ TParamRT<f32> modelScale2;
	/* 0x0E4 */ TParamRT<f32> modelScaleY;
	/* 0x0F8 */ TParamRT<f32> gravity;
	/* 0x10C */ TParamRT<f32> bounceY;
	/* 0x120 */ TParamRT<f32> bounceXZ;
	/* 0x134 */ TParamRT<f32> damp;
	/* 0x148 */ TParamRT<f32> jump;
	/* 0x15C */ TParamRT<f32> overGravity;
	/* 0x170 */ TParamRT<f32> emitVel;
	/* 0x184 */ TParamRT<s32> lifeTime;
};

class TBathWaterGlobalParams : public TParams {
public:
	TBathWaterGlobalParams();

public:
	/* 0x008 */ TParamRT<u8> regR;
	/* 0x01C */ TParamRT<u8> regG;
	/* 0x030 */ TParamRT<u8> regB;
	/* 0x044 */ TParamRT<u8> regA;
	/* 0x058 */ TParamRT<u8> kRegR;
	/* 0x06C */ TParamRT<u8> kRegG;
	/* 0x080 */ TParamRT<u8> kRegB;
	/* 0x094 */ TParamRT<u8> kRegA;
	/* 0x0A8 */ TParamRT<u8> polygonR;
	/* 0x0BC */ TParamRT<u8> polygonG;
	/* 0x0D0 */ TParamRT<u8> polygonB;
	/* 0x0E4 */ TParamRT<f32> indTexScale;
	/* 0x0F8 */ TParamRT<u8> showsCap;
	/* 0x10C */ TParamRT<u8> bendsNormal;
	/* 0x120 */ TParamRT<u8> showsMist;
	/* 0x134 */ TParamRT<u8> clearsAlpha;
	/* 0x148 */ TParamRT<u8> alpha;
	/* 0x15C */ TParamRT<u8> scrolls;
	/* 0x170 */ TParamRT<u8> displaysMesh;
	/* 0x184 */ TParamRT<u8> mode;
	/* 0x198 */ TParamRT<u8> mask;
	/* 0x1AC */ TParamRT<s32> indirectScale;
	/* 0x1C0 */ TParamRT<s32> scrollSpan;
	/* 0x1D4 */ TParamRT<s32> meshTexWidth;
	/* 0x1E8 */ TParamRT<f32> envMapScale;
	/* 0x1FC */ TParamRT<f32> capHeight;
	/* 0x210 */ TParamRT<f32> meshWidth;
};

class TBathWater;

class TBathWaterRenderer {
public:
	virtual void prerender(JDrama::TGraphics*, const TBathtubData&,
	                       TBathWater**, TBathWaterParams**, int)
	    = 0;
	virtual void render(JDrama::TGraphics*, const TBathtubData&, TBathWater**,
	                    TBathWaterParams**, int)
	    = 0;
	virtual f32 getHeight(f32, f32) const = 0;
};

class TBathWaterFlatRenderer : public TBathWaterRenderer {
public:
	TBathWaterFlatRenderer(TBathWaterGlobalParams*);
	virtual void prerender(JDrama::TGraphics*, const TBathtubData&,
	                       TBathWater**, TBathWaterParams**, int);
	virtual void render(JDrama::TGraphics*, const TBathtubData&, TBathWater**,
	                    TBathWaterParams**, int);
	virtual f32 getHeight(f32, f32) const;

public:
	/* 0x04 */ GXTexObj unk4;
	/* 0x24 */ void* unk24;
	/* 0x28 */ void* unk28;
	/* 0x2C */ TBathWaterGlobalParams* unk2C;
};

class TBathWaterManager;

class TBathWaterPreprocessor : public JDrama::TViewObj {
public:
	TBathWaterPreprocessor(TBathWaterManager*);
	virtual void perform(u32, JDrama::TGraphics*);

public:
	/* 0x10 */ TBathWaterManager* unk10;
};

class TBathWaterManager : public JDrama::TViewObj {
public:
	TBathWaterManager();
	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual void perform(u32, JDrama::TGraphics*);

	void wave(JGeometry::TVec3<f32>&, JGeometry::TVec3<f32>&, f32, f32) const;
	void initializeIfYet_();
	void preprocess(JDrama::TGraphics*);
	f32 getWaterHeight(f32, f32) const;
	void throwMario(f32);

	static const char* fileNames[];

	// Convenience accessor for the embedded preprocessor view object.
	TBathWaterPreprocessor* getPreprocessor() { return &unk34; }

public:
	/* 0x10 */ JMath::TRandomFast unk10;
	/* 0x14 */ TBathWaterParams** unk14;
	/* 0x18 */ TBathWaterGlobalParams* unk18;
	/* 0x1C */ u8 unk1C;
	/* 0x20 */ TBathWater** unk20;
	/* 0x24 */ TBathtub* unk24;
	/* 0x28 */ TBathWaterRenderer* unk28[2];
	/* 0x30 */ TBathWaterRenderer* unk30;
	/* 0x34 */ TBathWaterPreprocessor unk34;
};

#endif
