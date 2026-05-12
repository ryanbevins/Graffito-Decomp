#include <Camera/CameraKindParam.hpp>

// TCamSaveKindParam layout: starts with vtable/header, then TParamRT<T> entries
// at +0x18, +0x2C, +0x40, ... spaced 0x14 apart. .value field at offset 0x10
// within each entry. We use raw offsets here.

template <typename T> static T readParam(const void* base, u32 off)
{
	return *(const T*)((const u8*)base + off);
}

void TCameraKindParam::copySaveParam(const TCamSaveKindParam& src)
{
	const u8* s = (const u8*)&src;
	unk00       = *(const f32*)(s + 0x18);
	unk04       = *(const f32*)(s + 0x2C);
	unk08       = *(const f32*)(s + 0x40);
	unk0C       = *(const f32*)(s + 0x54);
	unk10       = *(const f32*)(s + 0x68);
	unk14       = *(const f32*)(s + 0x7C);
	unk18       = *(const s16*)(s + 0x90);
	unk1A       = *(const s16*)(s + 0xA4);
	unk1C       = *(const f32*)(s + 0xB8);
	unk20       = *(const s16*)(s + 0xCC);
	unk22       = *(const s16*)(s + 0xE0);
	unk24       = *(const f32*)(s + 0xF4);
	unk28       = *(const f32*)(s + 0x108);
	unk2C       = *(const f32*)(s + 0x11C);
	unk30       = *(const f32*)(s + 0x130);
	unk34       = *(const f32*)(s + 0x144);
	unk38       = *(const f32*)(s + 0x158);
	unk3C       = *(const f32*)(s + 0x16C);
	unk40       = *(const f32*)(s + 0x180);
	unk44       = *(const f32*)(s + 0x194);
	unk48       = *(const f32*)(s + 0x1A8);
	unk4C       = *(const f32*)(s + 0x1BC);
	unk50       = *(const f32*)(s + 0x1D0);
	unk54       = *(const s16*)(s + 0x1E4);
	unk56       = *(const s16*)(s + 0x1F8);
	unk58       = *(const s16*)(s + 0x20C);
	unk5A       = *(const s16*)(s + 0x220);
	unk5C       = *(const f32*)(s + 0x234);
	unk60       = *(const s16*)(s + 0x248);
	unk64       = *(const u32*)(s + 0x25C);
	unk68       = *(const u32*)(s + 0x270);
	unk6C       = *(const f32*)(s + 0x284);
	unk70       = *(const f32*)(s + 0x298);
	unk74       = *(const f32*)(s + 0x2AC);
	unk78       = *(const f32*)(s + 0x2C0);
	unk7C       = *(const f32*)(s + 0x2D4);
	unk80       = *(const f32*)(s + 0x2E8);
	unk84       = *(const f32*)(s + 0x2FC);
	unk88       = *(const f32*)(s + 0x310);
	unk8C       = *(const f32*)(s + 0x324);
	unk90       = *(const f32*)(s + 0x338);
	unk94       = *(const f32*)(s + 0x34C);
	unk98       = *(const f32*)(s + 0x360);
	unk9C       = *(const f32*)(s + 0x374);
	unkA0       = *(const f32*)(s + 0x388);
	unkA4       = *(const f32*)(s + 0x39C);
	unkA8       = *(const f32*)(s + 0x3B0);
}

void TCameraKindParam::inbetweenData(const TCameraKindParam& other, f32 t)
{
	// Linear interpolation between *this and other by ratio t (TODO match)
	f32 inv = 1.0f - t;
	(void)inv;
	(void)other;
	(void)t;
}
