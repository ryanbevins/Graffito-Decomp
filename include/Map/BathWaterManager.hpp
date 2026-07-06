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

class TBathtubData {
public:
	JGeometry::TVec3<f32> getGravityDir(f32) const;
	JGeometry::TVec3<f32> getPos(int, int, f32) const;

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

class TBathWater : public THitActor {
public:
	class TDrop {
	public:
		TDrop();
		void reset(const JGeometry::TVec3<f32>&, f32);
		void doThing(f32);

		void calcBathtub(const TBathtubData&, f32,
		                  const JGeometry::TVec3<f32>&,
		                  const JGeometry::TVec3<f32>&, int&,
		                  JGeometry::TVec3<f32>&);
		static void calcWaterModel(TBathWater*, const TBathtubData&);

	public:
		/* 0x00 */ JGeometry::TVec3<f32> unk0;
		/* 0x0C */ JGeometry::TVec3<f32> unkC;
		/* 0x18 */ JGeometry::TBox3<f32> unk18;
		/* 0x30 */ JGeometry::TBox3<f32> unk30;
		/* 0x48 */ f32 unk48;
		/* 0x4C */ s32 unk4C;
	};

	TBathWater();
	virtual ~TBathWater() { }
	void initialize(TBathWaterParams*, const TBathtubData&);
	bool eraseDrop(TDrop*);
	bool tryHitMario(THitActor*);
	bool tryHitMario2(THitActor*, const TBathtubData&);

public:
	/* 0x68 */ JMath::TRandomFast unk68;
	/* 0x6C */ u32 unk6C;
	/* 0x70 */ s32 unk70;
	/* 0x74 */ s32 unk74;
	/* 0x78 */ JGeometry::TVec3<f32> unk78;
	/* 0x84 */ f32 unk84;
	/* 0x88 */ TDrop* unk88;
	/* 0x8C */ TBathWaterParams* unk8C;
};

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

class TBathWaterMeshRenderer : public TBathWaterRenderer {
public:
	TBathWaterMeshRenderer(TBathWaterGlobalParams*, JUTTexture*);
	void makeHeightMap(f32);
	void makeNormalMap();
	void calcCoord();
	void clearHeightMap();

	virtual void prerender(JDrama::TGraphics*, const TBathtubData&,
	                       TBathWater**, TBathWaterParams**, int);
	virtual void render(JDrama::TGraphics*, const TBathtubData&, TBathWater**,
	                    TBathWaterParams**, int);
	virtual f32 getHeight(f32, f32) const;

public:
	/* 0x00004 */ char unk4[0x1C];
	/* 0x00020 */ JGeometry::TVec3<f32> unk20[0x4000];
	/* 0x30020 */ JGeometry::TVec3<f32> unk30020[0x4000];
	/* 0x60020 */ JGeometry::TVec2<f32> unk60020[0x4000];
	/* 0x80020 */ TRotation3f unk80020;
	/* 0x80050 */ JGeometry::SMatrix34C<f32> unk80050;
	/* 0x80080 */ f32 unk80080[9];
	/* 0x800A4 */ void* unk800A4;
	/* 0x800A8 */ void* unk800A8;
	/* 0x800AC */ s16 unk800AC;
	/* 0x800AE */ s16 unk800AE;
	/* 0x800B0 */ f32 unk800B0;
	/* 0x800B4 */ GXTexObj unk800B4;
	/* 0x800D4 */ GXTexObj unk800D4;
	/* 0x800F4 */ GXTexObj unk800F4;
	/* 0x80114 */ GXTexObj unk80114;
	/* 0x80134 */ TBathWaterGlobalParams* unk80134;
	/* 0x80138 */ JUTTexture* unk80138;
	/* 0x8013C */ JUTTexture* unk8013C;
	/* 0x80140 */ JUTTexture* unk80140;
	/* 0x80144 */ J3DModelData* unk80144;
	/* 0x80148 */ J3DModelData* unk80148;
	/* 0x8014C */ J3DModel* unk8014C;
	/* 0x80150 */ void* unk80150;
	/* 0x80154 */ void* unk80154;
	/* 0x80158 */ u32 unk80158;
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
	/* 0x24 */ u8* unk24;
	/* 0x28 */ TBathWaterRenderer* unk28;
	/* 0x2C */ TBathWaterRenderer* unk2C;
	/* 0x30 */ TBathWaterRenderer* unk30;
	/* 0x34 */ TBathWaterPreprocessor unk34;
};

#endif
