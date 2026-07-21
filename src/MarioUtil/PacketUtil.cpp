#include <MarioUtil/PacketUtil.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DShape.hpp>
#include <JSystem/J3D/J3DGraphBase/Blocks/J3DPEBlocks.hpp>
#include <dolphin/gx/GXCommandList.h>
#include <dolphin/gx/GXDispList.h>
#include <dolphin/gx/GXVert.h>

#define FIFO_WRITE_BP_REG(value)                                               \
	do {                                                                       \
		GXWGFifo.u8  = GX_LOAD_BP_REG;                                         \
		GXWGFifo.u32 = (u32)(value);                                           \
	} while (0)

#define PACKET_SET_REG_FIELD(reg, size, shift, val)                            \
	do {                                                                       \
		(reg) = ((u32)(reg) & ~(((1 << (size)) - 1) << (shift)))               \
		        | ((u32)(val) << (shift));                                     \
	} while (0)

static inline void FifoSetChanMatColor(GXChannelID chan, GXColor color)
{
	u32 reg = (color.r << 24) | (color.g << 16) | (color.b << 8) | color.a;

	GXWGFifo.u8  = GX_LOAD_XF_REG;
	GXWGFifo.u16 = 0;
	GXWGFifo.u16 = 0x100C + (chan & 1);
	GXWGFifo.u32 = reg;
}

static inline void FifoSetTevColorS10(GXTevRegID id, GXColorS10 color)
{
	u32 regRA = (color.r & 0x7FF) | ((color.a & 0x7FF) << 12)
	            | ((0xE0 + id * 2) << 24);
	u32 regBG = (color.b & 0x7FF) | ((color.g & 0x7FF) << 12)
	            | ((0xE1 + id * 2) << 24);

	FIFO_WRITE_BP_REG(regRA);
	FIFO_WRITE_BP_REG(regBG);
	FIFO_WRITE_BP_REG(regBG);
	FIFO_WRITE_BP_REG(regBG);
}

static inline void FifoSetTevKColor(GXTevKColorID id, GXColor color)
{
	u32 regRA = color.r | (color.a << 12) | (8 << 20)
	            | ((0xE0 + id * 2) << 24);
	u32 regBG = color.b | (color.g << 12) | (8 << 20)
	            | ((0xE1 + id * 2) << 24);

	FIFO_WRITE_BP_REG(regRA);
	FIFO_WRITE_BP_REG(regBG);
}

static void FifoSetFogRangeAdj(u8 enable, u16 center, GXFogAdjTable* table)
{
	if (enable) {
		for (int i = 0; i < 10; i += 2) {
			u32 rangeAdj = (((i / 2) + 0xE9) << 24)
			               | (table->r[i + 1] << 12) | table->r[i];
			FIFO_WRITE_BP_REG(rangeAdj);
		}
	}

	u32 rangeCenter = 0xE8000000 | (center + 342) | (enable << 10);
	FIFO_WRITE_BP_REG(rangeCenter);
}

static void FifoSetFog(GXFogType type, f32 startz, f32 endz, f32 nearz,
                       f32 farz, GXColor color)
{
	u32 fogColor;
	u32 fog0;
	u32 fog1;
	u32 fog2;
	u32 fog3;
	f32 A;
	f32 B;
	f32 B_mant;
	f32 a;
	f32 c;
	u32 B_expn;
	u32 b_m;
	u32 b_s;
	u32 a_hex;
	u32 c_hex;

	if (farz == nearz || endz == startz) {
		A = 0.0f;
		B = 0.5f;
		c = 0.0f;
	} else {
		A = (farz * nearz) / ((farz - nearz) * (endz - startz));
		B = farz / (farz - nearz);
		c = startz / (endz - startz);
	}

	B_mant = B;
	B_expn = 1;
	while (B_mant > 1.0) {
		B_mant *= 0.5f;
		B_expn++;
	}
	while (B_mant > 0.0f && B_mant < 0.5) {
		B_mant *= 2.0f;
		B_expn--;
	}

	a   = A / (f32)(1 << B_expn);
	b_m = 8388638.0f * B_mant;
	b_s = B_expn;

	a_hex = *(u32*)&a;
	c_hex = *(u32*)&c;

	fog0 = 0xEE000000 | (a_hex >> 12);
	FIFO_WRITE_BP_REG(fog0);

	fog1 = 0xEF000000 | b_m;
	FIFO_WRITE_BP_REG(fog1);

	fog2 = 0xF0000000 | b_s;
	FIFO_WRITE_BP_REG(fog2);

	fog3 = type << 21;
	fog3 |= c_hex >> 12;
	fog3 |= 0xF1000000;
	FIFO_WRITE_BP_REG(fog3);

	fogColor = color.b | (color.g << 8) | (color.r << 16);
	fogColor |= 0xF2000000;
	FIFO_WRITE_BP_REG(fogColor);
}

static void ShapePacketCallBackFunc(J3DCallBackPacket* packet, int timing)
{
	static const GXColor sFogOffColor = { 0, 0, 0, 0 };

	u32* userData = (u32*)packet->getUserArea();

	if (timing == 0) {
		switch (userData[0]) {
		case 0:
			FifoSetChanMatColor((GXChannelID)userData[1],
			                    *(const GXColor*)userData[2]);
			break;

		case 1:
			FifoSetTevColorS10((GXTevRegID)userData[1],
			                   *(const GXColorS10*)userData[2]);
			break;

		case 2:
			FifoSetTevColorS10((GXTevRegID)userData[1],
			                   *(const GXColorS10*)userData[3]);
			FifoSetTevColorS10((GXTevRegID)userData[2],
			                   *(const GXColorS10*)userData[4]);
			break;

		case 3:
			FifoSetTevColorS10((GXTevRegID)userData[1],
			                   *(const GXColorS10*)userData[4]);
			FifoSetTevColorS10((GXTevRegID)userData[2],
			                   *(const GXColorS10*)userData[5]);
			FifoSetTevColorS10((GXTevRegID)userData[3],
			                   *(const GXColorS10*)userData[6]);
			break;

		case 4:
			GXCallDisplayList((void*)userData[1], userData[2]);
			break;

		case 5: {
			J3DFogInfo* fog = (J3DFogInfo*)userData[1];
			FifoSetFog((GXFogType)fog->mType, fog->mStartZ, fog->mEndZ,
			           fog->mNearZ, fog->mFarZ, fog->mColor);
			FifoSetFogRangeAdj(fog->mAdjEnable, fog->mCenter,
			                    (GXFogAdjTable*)fog->mFogAdjTable);
			break;
		}

		case 6:
			FifoSetTevKColor((GXTevKColorID)userData[1],
			                 *(const GXColor*)userData[2]);
			break;

		case 7:
			FifoSetTevKColor((GXTevKColorID)userData[1],
			                 *(const GXColor*)userData[3]);
			FifoSetTevKColor((GXTevKColorID)userData[2],
			                 *(const GXColor*)userData[4]);
			break;

		case 8: {
			FifoSetTevKColor((GXTevKColorID)userData[2],
			                 *(const GXColor*)userData[3]);
			J3DFogInfo* fog = (J3DFogInfo*)userData[5];
			FifoSetFog((GXFogType)fog->mType, fog->mStartZ, fog->mEndZ,
			           fog->mNearZ, fog->mFarZ, fog->mColor);
			FifoSetFogRangeAdj(fog->mAdjEnable, fog->mCenter,
			                    (GXFogAdjTable*)fog->mFogAdjTable);
			break;
		}

		case 9:
			FifoSetTevColorS10((GXTevRegID)userData[1],
			                   *(const GXColorS10*)userData[2]);
			FifoSetTevKColor(GX_KCOLOR0, *(const GXColor*)userData[3]);
			break;

		case 10:
			FifoSetTevColorS10((GXTevRegID)userData[1],
			                   *(const GXColorS10*)userData[3]);
			FifoSetTevColorS10((GXTevRegID)userData[2],
			                   *(const GXColorS10*)userData[4]);
			FifoSetTevKColor(GX_KCOLOR0, *(const GXColor*)userData[5]);
			break;
		}
	} else if (timing == 1) {
		switch (userData[0]) {
		case 5:
		case 8:
			FifoSetFog(GX_FOG_NONE, 0.0f, 0.0f, 0.0f, 0.0f,
			           sFogOffColor);
			break;
		}
	}
}

static inline J3DShapePacket* InitPacket_Sub(J3DModel* model, u16 mat_idx)
{
	J3DMaterial* mat = model->getModelData()->getMaterialNodePointer(mat_idx);
	return model->getShapePacket(mat->getShape()->getIndex());
}

// fabricated
struct PacketUserData_MatColor {
	u32 unk0;
	GXChannelID unk4;
	const GXColor* unk8;
};

void SMS_InitPacket_MatColor(J3DModel* param_1, u16 param_2,
                             GXChannelID param_3, const GXColor* param_4)
{
	J3DShapePacket* packet = InitPacket_Sub(param_1, param_2);

	PacketUserData_MatColor* userData = new PacketUserData_MatColor;

	userData->unk0 = 0;
	userData->unk4 = param_3;
	userData->unk8 = param_4;

	packet->setUserArea((u32)userData);
	packet->setCallback(&ShapePacketCallBackFunc);
}

// fabricated
struct PacketUserData_OneTevColor {
	u32 unk0;
	GXTevRegID unk4;
	const GXColorS10* unk8;
};

void SMS_InitPacket_OneTevColor(J3DModel* param_1, u16 param_2,
                                GXTevRegID param_3, const GXColorS10* param_4)
{
	J3DShapePacket* packet = InitPacket_Sub(param_1, param_2);

	PacketUserData_OneTevColor* userData = new PacketUserData_OneTevColor;

	userData->unk0 = 1;
	userData->unk4 = param_3;
	userData->unk8 = param_4;

	packet->setUserArea((u32)userData);
	packet->setCallback(&ShapePacketCallBackFunc);
}

// fabricated
struct PacketUserData_TwoTevColor {
	u32 unk0;
	GXTevRegID unk4;
	GXTevRegID unk8;
	const GXColorS10* unkC;
	const GXColorS10* unk10;
};

void SMS_InitPacket_TwoTevColor(J3DModel* param_1, u16 param_2,
                                GXTevRegID param_3, const GXColorS10* param_4,
                                GXTevRegID param_5, const GXColorS10* param_6)
{
	J3DShapePacket* packet = InitPacket_Sub(param_1, param_2);

	PacketUserData_TwoTevColor* userData = new PacketUserData_TwoTevColor;

	userData->unk0  = 2;
	userData->unk4  = param_3;
	userData->unkC  = param_4;
	userData->unk8  = param_5;
	userData->unk10 = param_6;

	packet->setUserArea((u32)userData);
	packet->setCallback(&ShapePacketCallBackFunc);
}

// fabricated
struct PacketUserData_ThreeTevColor {
	u32 unk0;
	GXTevRegID unk4;
	GXTevRegID unk8;
	GXTevRegID unkC;
	const GXColorS10* unk10;
	const GXColorS10* unk14;
	const GXColorS10* unk18;
};

void SMS_InitPacket_ThreeTevColor(J3DModel* param_1, u16 param_2,
                                  GXTevRegID param_3, const GXColorS10* param_4,
                                  GXTevRegID param_5, const GXColorS10* param_6,
                                  GXTevRegID param_7, const GXColorS10* param_8)
{
	J3DShapePacket* packet = InitPacket_Sub(param_1, param_2);

	PacketUserData_ThreeTevColor* userData = new PacketUserData_ThreeTevColor;

	userData->unk0  = 3;
	userData->unk4  = param_3;
	userData->unk10 = param_4;
	userData->unk8  = param_5;
	userData->unk14 = param_6;
	userData->unkC  = param_7;
	userData->unk18 = param_8;

	packet->setUserArea((u32)userData);
	packet->setCallback(&ShapePacketCallBackFunc);
}

// fabricated
struct PacketUserData_Fog {
	u32 unk0;
	J3DFog* unk4;
};

void SMS_InitPacket_Fog(J3DModel* param_1, u16 param_2)
{
	J3DMaterial* mat = param_1->getModelData()->getMaterialNodePointer(param_2);
	J3DPEBlock* pe  = mat->getPEBlock();
	J3DShapePacket* packet
	    = param_1->getShapePacket(mat->getShape()->getIndex());

	J3DFog* fog = pe->getFog();

	PacketUserData_Fog* userData = new PacketUserData_Fog;
	userData->unk0               = 5;
	userData->unk4               = fog;

	packet->setUserArea((u32)userData);
	packet->setCallback(&ShapePacketCallBackFunc);
}

// fabricated
struct PacketUserData_OneTevKColor {
	u32 unk0;
	GXTevKColorID unk4;
	const GXColor* unk8;
};

void SMS_InitPacket_OneTevKColor(J3DModel* param_1, u16 param_2,
                                 GXTevKColorID param_3, const GXColor* param_4)
{
	J3DShapePacket* packet = InitPacket_Sub(param_1, param_2);

	PacketUserData_OneTevKColor* userData = new PacketUserData_OneTevKColor;

	userData->unk0 = 6;
	userData->unk4 = param_3;
	userData->unk8 = param_4;

	packet->setUserArea((u32)userData);
	packet->setCallback(&ShapePacketCallBackFunc);
}

// fabricated
struct PacketUserData_TwoTevKColor {
	u32 unk0;
	GXTevKColorID unk4;
	GXTevKColorID unk8;
	const GXColor* unkC;
	const GXColor* unk10;
};

void SMS_InitPacket_TwoTevKColor(J3DModel* param_1, u16 param_2,
                                 GXTevKColorID param_3, const GXColor* param_4,
                                 GXTevKColorID param_5, const GXColor* param_6)
{
	J3DShapePacket* packet = InitPacket_Sub(param_1, param_2);

	PacketUserData_TwoTevKColor* userData = new PacketUserData_TwoTevKColor;

	userData->unk0  = 7;
	userData->unk4  = param_3;
	userData->unkC  = param_4;
	userData->unk8  = param_5;
	userData->unk10 = param_6;

	packet->setUserArea((u32)userData);
	packet->setCallback(&ShapePacketCallBackFunc);
}

// fabricated
struct PacketUserData_OneTevKColorAndFog {
	u32 unk0;
	u32 unk4;
	GXTevKColorID unk8;
	const GXColor* unkC;
	u32 unk10;
	J3DFog* unk14;
};

void SMS_InitPacket_OneTevKColorAndFog(J3DModel* param_1, u16 param_2,
                                       GXTevKColorID param_3,
                                       const GXColor* param_4)
{
	J3DShapePacket* packet = InitPacket_Sub(param_1, param_2);

	PacketUserData_OneTevKColorAndFog* userData
	    = new PacketUserData_OneTevKColorAndFog;

	userData->unk0 = 8;
	userData->unk4 = 6;
	userData->unk8 = param_3;

	if (param_4 != nullptr) {
		userData->unkC = param_4;
	} else {
		userData->unkC = &param_1->getModelData()
		                      ->getMaterialNodePointer(param_2)
		                      ->getTevBlock()
		                      ->getTevKColor(param_3)
		                      ->color;
	}

	J3DFog* fog = param_1->getModelData()
	                  ->getMaterialNodePointer(param_2)
	                  ->getPEBlock()
	                  ->getFog();

	userData->unk10 = 5;
	userData->unk14 = fog;

	packet->setUserArea((u32)userData);
	packet->setCallback(&ShapePacketCallBackFunc);
}

// fabricated
struct PacketUserData_OneTevColorAndOneTevKColor {
	u32 unk0;
	GXTevRegID unk4;
	const GXColorS10* unk8;
	const GXColor* unkC;
};

void SMS_InitPacket_OneTevColorAndOneTevKColor(J3DModel* param_1, u16 param_2,
                                               GXTevRegID param_3,
                                               const GXColorS10* param_4,
                                               const GXColor* param_5)
{
	J3DShapePacket* packet = InitPacket_Sub(param_1, param_2);

	PacketUserData_OneTevColorAndOneTevKColor* userData
	    = new PacketUserData_OneTevColorAndOneTevKColor;

	userData->unk0 = 9;
	userData->unk4 = param_3;
	userData->unk8 = param_4;
	userData->unkC = param_5;

	packet->setUserArea((u32)userData);
	packet->setCallback(&ShapePacketCallBackFunc);
}

// fabricated
struct PacketUserData_TwoTevColorAndOneTevKColor {
	u32 unk0;
	GXTevRegID unk4;
	GXTevRegID unk8;
	const GXColorS10* unkC;
	const GXColorS10* unk10;
	const GXColor* unk14;
};

void SMS_InitPacket_TwoTevColorAndOneTevKColor(J3DModel* param_1, u16 param_2,
                                               GXTevRegID param_3,
                                               const GXColorS10* param_4,
                                               GXTevRegID param_5,
                                               const GXColorS10* param_6,
                                               const GXColor* param_7)
{
	J3DShapePacket* packet = InitPacket_Sub(param_1, param_2);

	PacketUserData_TwoTevColorAndOneTevKColor* userData
	    = new PacketUserData_TwoTevColorAndOneTevKColor;

	userData->unk0  = 10;
	userData->unk4  = param_3;
	userData->unkC  = param_4;
	userData->unk8  = param_5;
	userData->unk10 = param_6;
	userData->unk14 = param_7;

	packet->setUserArea((u32)userData);
	packet->setCallback(&ShapePacketCallBackFunc);
}

void SMS_HideAllShapePacket(J3DModel* model)
{
	u16 mats = model->getModelData()->getMaterialNum();
	for (u16 i = 0; i < mats; ++i)
		model->getShapePacket(i)->hide();
}

void SMS_ShowAllShapePacket(J3DModel* model)
{
	u16 mats = model->getModelData()->getMaterialNum();
	for (u16 i = 0; i < mats; ++i)
		model->getShapePacket(i)->show();
}
