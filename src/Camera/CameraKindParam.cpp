#include <Camera/CameraKindParam.hpp>
#include <Camera/cameralib.hpp>

template <> s16 CLBRoundf<s16>(f32);

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

static inline void inbetweenS16(s16* dst, s16 target, f32 t)
{
	if (t < 0.001f)
		*dst = target;
	else
		*dst += CLBRoundf<s16>((1.0f / t) * (f32)(s16)(target - *dst));
}

void TCameraKindParam::inbetweenData(const TCameraKindParam& other, f32 t)
{
	CLBChaseConstantSpecifyFrame(&unk00, other.unk00, t);
	CLBChaseConstantSpecifyFrame(&unk04, other.unk04, t);
	CLBChaseConstantSpecifyFrame(&unk08, other.unk08, t);
	CLBChaseConstantSpecifyFrame(&unk0C, other.unk0C, t);
	CLBChaseConstantSpecifyFrame(&unk10, other.unk10, t);
	CLBChaseConstantSpecifyFrame(&unk14, other.unk14, t);
	inbetweenS16(&unk18, other.unk18, t);
	inbetweenS16(&unk1A, other.unk1A, t);
	CLBChaseConstantSpecifyFrame(&unk1C, other.unk1C, t);
	inbetweenS16(&unk20, other.unk20, t);
	inbetweenS16(&unk22, other.unk22, t);
	CLBChaseConstantSpecifyFrame(&unk24, other.unk24, t);
	CLBChaseConstantSpecifyFrame(&unk28, other.unk28, t);
	CLBChaseConstantSpecifyFrame(&unk2C, other.unk2C, t);
	CLBChaseConstantSpecifyFrame(&unk30, other.unk30, t);
	CLBChaseConstantSpecifyFrame(&unk34, other.unk34, t);
	CLBChaseConstantSpecifyFrame(&unk38, other.unk38, t);
	CLBChaseConstantSpecifyFrame(&unk3C, other.unk3C, t);
	CLBChaseConstantSpecifyFrame(&unk40, other.unk40, t);
	CLBChaseConstantSpecifyFrame(&unk44, other.unk44, t);
	CLBChaseConstantSpecifyFrame(&unk48, other.unk48, t);
	CLBChaseConstantSpecifyFrame(&unk4C, other.unk4C, t);
	CLBChaseConstantSpecifyFrame(&unk50, other.unk50, t);
	inbetweenS16(&unk54, other.unk54, t);
	inbetweenS16(&unk56, other.unk56, t);
	unk58 = other.unk58;
	unk5A = other.unk5A;
	CLBChaseConstantSpecifyFrame(&unk5C, other.unk5C, t);
	inbetweenS16(&unk60, other.unk60, t);
	unk64 = other.unk64;
	unk68 = other.unk68;
	CLBChaseConstantSpecifyFrame(&unk6C, other.unk6C, t);
	CLBChaseConstantSpecifyFrame(&unk70, other.unk70, t);
	CLBChaseConstantSpecifyFrame(&unk74, other.unk74, t);
	CLBChaseConstantSpecifyFrame(&unk78, other.unk78, t);
	CLBChaseConstantSpecifyFrame(&unk7C, other.unk7C, t);
	CLBChaseConstantSpecifyFrame(&unk80, other.unk80, t);
	CLBChaseConstantSpecifyFrame(&unk84, other.unk84, t);
	CLBChaseConstantSpecifyFrame(&unk88, other.unk88, t);
	CLBChaseConstantSpecifyFrame(&unk8C, other.unk8C, t);
	CLBChaseConstantSpecifyFrame(&unk90, other.unk90, t);
	CLBChaseConstantSpecifyFrame(&unk94, other.unk94, t);
	CLBChaseConstantSpecifyFrame(&unk98, other.unk98, t);
	CLBChaseConstantSpecifyFrame(&unk9C, other.unk9C, t);
	CLBChaseConstantSpecifyFrame(&unkA0, other.unkA0, t);
	CLBChaseConstantSpecifyFrame(&unkA4, other.unkA4, t);
	CLBChaseConstantSpecifyFrame(&unkA8, other.unkA8, t);
}
