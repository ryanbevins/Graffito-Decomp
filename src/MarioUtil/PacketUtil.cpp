#include <MarioUtil/PacketUtil.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DShape.hpp>
#include <JSystem/J3D/J3DGraphBase/Blocks/J3DPEBlocks.hpp>
#include <dolphin/gx/GXCommandList.h>
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

static void FifoSetChanMatColor(GXChannelID, GXColor) { }

static void FifoSetTevColorS10(GXTevRegID, GXColorS10) { }

static void FifoSetTevKColor(GXTevKColorID, GXColor) { }

static void FifoSetFogRangeAdj(u8 enable, u16 center, GXFogAdjTable* table)
{
	if (enable) {
		for (int i = 0; i < 10; i += 2) {
			u32 rangeAdj = 0;
			PACKET_SET_REG_FIELD(rangeAdj, 12, 0, table->r[i]);
			PACKET_SET_REG_FIELD(rangeAdj, 12, 12, table->r[i + 1]);
			PACKET_SET_REG_FIELD(rangeAdj, 8, 24, (i >> 1) + 0xE9);
			FIFO_WRITE_BP_REG(rangeAdj);
		}
	}

	u32 rangeCenter = 0;
	PACKET_SET_REG_FIELD(rangeCenter, 10, 0, center + 342);
	PACKET_SET_REG_FIELD(rangeCenter, 1, 10, enable);
	PACKET_SET_REG_FIELD(rangeCenter, 8, 24, 0xE8);
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
	f32 C;
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
		C = 0.0f;
	} else {
		A = (farz * nearz) / ((farz - nearz) * (endz - startz));
		B = farz / (farz - nearz);
		C = startz / (endz - startz);
	}

	B_mant = B;
	B_expn = 0;
	while (B_mant > 1.0) {
		B_mant *= 0.5f;
		B_expn++;
	}
	while (B_mant > 0.0f && B_mant < 0.5) {
		B_mant *= 2.0f;
		B_expn--;
	}

	a   = A / (f32)(1 << (B_expn + 1));
	b_m = 8388638.0f * B_mant;
	b_s = B_expn + 1;
	c   = C;

	fog1 = 0;
	PACKET_SET_REG_FIELD(fog1, 24, 0, b_m);
	PACKET_SET_REG_FIELD(fog1, 8, 24, 0xEF);

	fog2 = 0;
	PACKET_SET_REG_FIELD(fog2, 5, 0, b_s);
	PACKET_SET_REG_FIELD(fog2, 8, 24, 0xF0);

	a_hex = *(u32*)&a;
	c_hex = *(u32*)&c;

	fog0 = 0;
	PACKET_SET_REG_FIELD(fog0, 11, 0, (a_hex >> 12) & 0x7FF);
	PACKET_SET_REG_FIELD(fog0, 8, 11, (a_hex >> 23) & 0xFF);
	PACKET_SET_REG_FIELD(fog0, 1, 19, a_hex >> 31);
	PACKET_SET_REG_FIELD(fog0, 8, 24, 0xEE);

	fog3 = 0;
	PACKET_SET_REG_FIELD(fog3, 11, 0, (c_hex >> 12) & 0x7FF);
	PACKET_SET_REG_FIELD(fog3, 8, 11, (c_hex >> 23) & 0xFF);
	PACKET_SET_REG_FIELD(fog3, 1, 19, c_hex >> 31);
	PACKET_SET_REG_FIELD(fog3, 1, 20, 0);
	PACKET_SET_REG_FIELD(fog3, 3, 21, type);
	PACKET_SET_REG_FIELD(fog3, 8, 24, 0xF1);

	fogColor = 0;
	PACKET_SET_REG_FIELD(fogColor, 8, 0, color.b);
	PACKET_SET_REG_FIELD(fogColor, 8, 8, color.g);
	PACKET_SET_REG_FIELD(fogColor, 8, 16, color.r);
	PACKET_SET_REG_FIELD(fogColor, 8, 24, 0xF2);

	FIFO_WRITE_BP_REG(fog0);
	FIFO_WRITE_BP_REG(fog1);
	FIFO_WRITE_BP_REG(fog2);
	FIFO_WRITE_BP_REG(fog3);
	FIFO_WRITE_BP_REG(fogColor);
}

static void SetFogBase(const J3DFogInfo*) { }

static void ShapePacketCallBackFunc(J3DCallBackPacket*, int) { }

static J3DShapePacket* InitPacket_Sub(J3DModel* model, u16 mat_idx)
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
	J3DShapePacket* packet = InitPacket_Sub(param_1, param_2);

	J3DFog* fog = param_1->getModelData()
	                  ->getMaterialNodePointer(param_2)
	                  ->getPEBlock()
	                  ->getFog();

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
