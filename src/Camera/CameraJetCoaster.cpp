#include <Camera/Camera.hpp>

class TCamSaveJetCoaster {
public:
	TCamSaveJetCoaster() { }
};

class TCameraJetCoaster {
public:
	TCameraJetCoaster();

	/* 0x00 */ TCamSaveJetCoaster* unk0;
	/* 0x04 */ u16 unk4;
	/* 0x06 */ u16 unk6;
	/* 0x08 */ u16 unk8;
	/* 0x0A */ u16 unkA;
	/* 0x0C */ u8 unkC;
	/* 0x10 */ f32 unk10;
	/* 0x14 */ f32 unk14;
	/* 0x18 */ f32 unk18;
	/* 0x1C */ f32 unk1C;
	/* 0x20 */ f32 unk20;
	/* 0x24 */ f32 unk24;
	/* 0x28 */ f32 unk28;
	/* 0x2C */ f32 unk2C;
	/* 0x30 */ f32 unk30;
	/* 0x34 */ f32 unk34;
	/* 0x38 */ u16 unk38;
};

TCameraJetCoaster::TCameraJetCoaster()
{
	unk4  = 0;
	unk6  = 0;
	unk8  = 0;
	unkA  = 0;
	unkC  = 1;
	unk10 = 0.0f;
	unk14 = 0.0f;
	unk18 = 500.0f;
	unk1C = 0.0f;
	unk20 = 0.0f;
	unk24 = 0.0f;
	unk28 = 0.0f;
	unk2C = 1.0f;
	unk30 = 0.0f;
	unk34 = 60.0f;
	unk38 = 0;
	unk0  = new TCamSaveJetCoaster();
}

void CPolarSubCamera::ctrlJetCoasterCamera_() { }
