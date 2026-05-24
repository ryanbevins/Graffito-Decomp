#include <NPC/NpcInitData.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <dolphin/gx/GXEnum.h>
#include <dolphin/gx/GXStruct.h>

void SMS_InitChangeNpcColor(const MActor* mActor, const TColorChangeInfo* info,
                            s16 idx, const GXColor* dstColor)
{
	J3DModel* model = ((MActor*)mActor)->getModel();
	s32 nameIdx     = model->getModelData()->getMaterialName()->getIndex(
	    info->unk4);

	switch (info->unk0) {
	case 0:
		if (info->unk8 != NULL) {
			GXColor* col = new GXColor;
			col->r       = (u8)info->unk8[idx].r;
			col->g       = (u8)info->unk8[idx].g;
			col->b       = (u8)info->unk8[idx].b;
			col->a       = 0xFF;
			SMS_InitPacket_MatColor(model, nameIdx, GX_COLOR0, col);
		}
		break;
	case 1: {
		if (info->unk8 == NULL)
			break;
		const GXColorS10* col = &info->unk8[idx];
		if (dstColor != NULL)
			SMS_InitPacket_OneTevColorAndOneTevKColor(model, nameIdx,
			    GX_TEVREG0, col, dstColor);
		else
			SMS_InitPacket_OneTevColor(model, nameIdx, GX_TEVREG0, col);
		break;
	}
	case 2: {
		if (info->unk8 != NULL && info->unkC != NULL) {
			const GXColorS10* c8 = &info->unk8[idx];
			const GXColorS10* cC = &info->unkC[idx];
			if (dstColor != NULL)
				SMS_InitPacket_TwoTevColorAndOneTevKColor(model,
				    nameIdx, GX_TEVREG1, c8, GX_TEVREG2, cC,
				    dstColor);
			else
				SMS_InitPacket_TwoTevColor(model, nameIdx,
				    GX_TEVREG1, c8, GX_TEVREG2, cC);
			break;
		}
		if (info->unk8 != NULL && info->unkC == NULL) {
			const GXColorS10* col = &info->unk8[idx];
			if (dstColor != NULL)
				SMS_InitPacket_OneTevColorAndOneTevKColor(model,
				    nameIdx, GX_TEVREG1, col, dstColor);
			else
				SMS_InitPacket_OneTevColor(model, nameIdx,
				    GX_TEVREG1, col);
			break;
		}
		if (info->unk8 == NULL && info->unkC != NULL) {
			const GXColorS10* col = &info->unkC[idx];
			if (dstColor != NULL)
				SMS_InitPacket_OneTevColorAndOneTevKColor(model,
				    nameIdx, GX_TEVREG2, col, dstColor);
			else
				SMS_InitPacket_OneTevColor(model, nameIdx,
				    GX_TEVREG2, col);
		}
		break;
	}
	}
}
