#define JGEOMETRY_DRAWUTIL_OWNER_HELPERS
#define JGEOMETRY_TVEC3_IMPLICIT_COPY_CTOR
#include <System/TalkCursor.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorData.hpp>
#include <NPC/NpcBase.hpp>
#include <dolphin/gx.h>

void TTalkCursor::loadAfter()
{
	MActorAnmData* anmData = new MActorAnmData;
	anmData->init("/common/cursor_b", nullptr);

	unk10         = new MActor(anmData);
	MActor* actor = unk10;

	void* res = JKRFileLoader::getGlbResource("/common/cursor_b/default.bmd");
	actor->setModel(
	    new J3DModel(J3DModelLoaderDataBase::load(res, 0x10020000), 0, 1), 0);
	actor->setBck("icon_rot");
	actor->setBrk("icon_flash");

	unkC.on(0x204);
}

void TTalkCursor::perform(u32 flags, JDrama::TGraphics* gfx)
{
	if (flags & 0x8) {
		GXSetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
	} else {
		unk10->perform(flags, gfx);
	}
}

void TTalkCursor::associateNPC(TBaseNPC* npc)
{
	if (npc) {
		JGeometry::TVec3<f32> pos = npc->getCursorPos();
		TPosition3f mtx;
		mtx.translation(pos.x, pos.y, pos.z);
		PSMTXCopy(mtx, unk10->getModel()->unk20);
		unkC.off(0x204);
	} else {
		unkC.on(0x204);
	}
}
