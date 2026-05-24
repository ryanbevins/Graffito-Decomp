#ifndef MOVE_BG_MAP_OBJ_WAVE_HPP
#define MOVE_BG_MAP_OBJ_WAVE_HPP

#include <JSystem/JDrama/JDRViewObj.hpp>
#include <JSystem/ResTIMG.hpp>
#include <dolphin/gx/GXEnum.h>
#include <dolphin/gx/GXStruct.h>

class TMapObjWave;

extern TMapObjWave* gpMapObjWave;

class TMapObjWave : public JDrama::TViewObj {
public:
	TMapObjWave(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*);

	void updateTime();
	void updateHeightAndAlpha();
	void draw();
	void noWave();
	f32 getHeight(f32 x, f32 y, f32 z) const;
	f32 getWaveHeight(f32 x, f32 z) const;
	void initDraw();

public:
	/* 0x10 */ f32 mWaveSize;
	/* 0x14 */ f32 mHalfWaveSize;
	/* 0x18 */ f32 mInvHalfWaveSize;
	/* 0x1C */ f32 mGridSize;
	/* 0x20 */ int mGridCount;
	/* 0x24 */ f32 unk24;
	/* 0x28 */ f32 unk28;
	/* 0x2C */ f32 unk2C;
	/* 0x30 */ f32 unk30;
	/* 0x34 */ f32 unk34;
	/* 0x38 */ f32 unk38;
	/* 0x3C */ f32 unk3C;
	/* 0x40 */ f32 unk40;
	/* 0x44 */ f32 unk44;
	/* 0x48 */ f32 unk48;
	/* 0x4C */ f32 unk4C;
	/* 0x50 */ f32 unk50;
	/* 0x54 */ f32 unk54;
	/* 0x58 */ f32 unk58;
	/* 0x5C */ f32 unk5C;
	/* 0x60 */ f32 unk60;
	/* 0x64 */ f32 unk64;
	/* 0x68 */ f32 unk68;
	/* 0x6C */ f32 unk6C;
	/* 0x70 */ f32 unk70;
	/* 0x74 */ f32 unk74;
	/* 0x78 */ f32 unk78;
	/* 0x7C */ GXColorS10 unk7C;
	/* 0x84 */ GXColorS10 unk84;
	/* 0x8C */ GXColorS10 unk8C;
	/* 0x94 */ const ResTIMG* mTexInfo;
	/* 0x98 */ u16 unk98;
};

#endif
