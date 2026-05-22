#ifndef CAMERA_CAMERAKINDPARAM_HPP
#define CAMERA_CAMERAKINDPARAM_HPP

#include <types.h>

// Flat extracted params (size 0xAC)
class TCameraKindParam {
public:
	void copySaveParam(const class TCamSaveKindParam&);
	void inbetweenData(const TCameraKindParam&, f32 ratio);

public:
	/* 0x00 */ f32 unk00;
	/* 0x04 */ f32 unk04;
	/* 0x08 */ f32 unk08;
	/* 0x0C */ f32 unk0C;
	/* 0x10 */ f32 unk10;
	/* 0x14 */ f32 unk14;
	/* 0x18 */ s16 unk18;
	/* 0x1A */ s16 unk1A;
	/* 0x1C */ f32 unk1C;
	/* 0x20 */ s16 unk20;
	/* 0x22 */ s16 unk22;
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
	/* 0x54 */ s16 unk54;
	/* 0x56 */ s16 unk56;
	/* 0x58 */ s16 unk58;
	/* 0x5A */ s16 unk5A;
	/* 0x5C */ f32 unk5C;
	/* 0x60 */ s16 unk60;
	/* 0x64 */ u32 unk64;
	/* 0x68 */ u32 unk68;
	/* 0x6C */ f32 unk6C;
	/* 0x70 */ f32 unk70;
	/* 0x74 */ f32 unk74;
	/* 0x78 */ f32 unk78;
	/* 0x7C */ f32 unk7C;
	/* 0x80 */ f32 unk80;
	/* 0x84 */ f32 unk84;
	/* 0x88 */ f32 unk88;
	/* 0x8C */ f32 unk8C;
	/* 0x90 */ f32 unk90;
	/* 0x94 */ f32 unk94;
	/* 0x98 */ f32 unk98;
	/* 0x9C */ f32 unk9C;
	/* 0xA0 */ f32 unkA0;
	/* 0xA4 */ f32 unkA4;
	/* 0xA8 */ f32 unkA8;
};

// TCamSaveKindParam: TParamRT-based save struct. Defined in camerasave.cpp.
class TCamSaveKindParam;

#endif
