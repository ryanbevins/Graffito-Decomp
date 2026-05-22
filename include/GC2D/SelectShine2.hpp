#ifndef GC2D_SELECT_SHINE_2_HPP
#define GC2D_SELECT_SHINE_2_HPP

#include <JSystem/JDrama/JDRViewObj.hpp>
#include <JSystem/JGeometry.hpp>

class J3DModel;
class J3DModelData;
class J3DAnmColor;
class J3DDrawBuffer;
class JPAEmitterManager;
class JPABaseEmitter;

class TSelectShine {
public:
	TSelectShine(J3DModelData*, J3DAnmColor*, JPAEmitterManager*,
	             JGeometry::TVec3<f32>&, s16, u8, f32, f32, f32);
	virtual ~TSelectShine() { }
	virtual void move();

public:
	/* 0x04 */ J3DModel* mModel;
	/* 0x08 */ J3DAnmColor* mAnmColor;
	/* 0x0C */ JGeometry::TVec3<f32> mPos;
	/* 0x18 */ JGeometry::TVec3<f32> unk18;
	/* 0x24 */ u8 unk24;
	/* 0x28 */ f32 unk28;
	/* 0x2C */ f32 unk2C;
	/* 0x30 */ f32 unk30;
	/* 0x34 */ s32 unk34;
	/* 0x38 */ u8 unk38;
	/* 0x3A */ s16 unk3A;
	/* 0x3C */ s16 unk3C;
	/* 0x3E */ u8 unk3E;
	/* 0x40 */ f32 unk40;
	/* 0x44 */ f32 unk44;
	/* 0x48 */ u8 unk48;
	/* 0x49 */ u8 unk49;
	/* 0x4A */ u8 unk4A;
	/* 0x4C */ JPAEmitterManager* mEmitterMgr;
	/* 0x50 */ JPABaseEmitter* unk50;
	/* 0x54 */ JPABaseEmitter* unk54;
	/* 0x58 */ JPABaseEmitter* unk58;
};

class TSelectShineManager : public JDrama::TViewObj {
public:
	TSelectShineManager(const char*);

	virtual void perform(u32, JDrama::TGraphics*);

	void initData(u8*, u8, u8, JPAEmitterManager*);
	void startDecrease(int);
	void startIncrease(int);
	void startClose();

	static JGeometry::TVec3<f32> cCenter;

public:
	/* 0x10 */ TSelectShine* mShines[8];
	/* 0x30 */ char unk30[0x20];
	/* 0x50 */ J3DDrawBuffer* mDrawBufOpa;
	/* 0x54 */ J3DDrawBuffer* mDrawBufXlu;
	/* 0x58 */ char unk58[0x20];
	/* 0x78 */ f32 unk78;
	/* 0x7C */ f32 unk7C;
	/* 0x80 */ char unk80[0x8];
	/* 0x88 */ s32 mShineCount;
	/* 0x8C */ s32 unk8C;
	/* 0x90 */ f32 unk90;
	/* 0x94 */ f32 unk94;
	/* 0x98 */ s32 unk98;
	/* 0x9C */ s32 unk9C;
	/* 0xA0 */ f32 unkA0;
	/* 0xA4 */ u8 unkA4;
	/* 0xA5 */ u8 unkA5;
	/* 0xA6 */ u8 unkA6;
	/* 0xA7 */ u8 unkA7;
	/* 0xA8 */ JGeometry::TVec3<f32> unkA8[8];
	/* 0x108 */ u8 _108[0x120 - 0x108];
};

#endif
