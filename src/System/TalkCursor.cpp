#include <System/TalkCursor.hpp>
#include <M3DUtil/MActor.hpp>
#include <dolphin/gx.h>

void TTalkCursor::loadAfter() { }

void TTalkCursor::perform(u32 flags, JDrama::TGraphics* gfx)
{
	if (flags & 0x8) {
		GXSetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
	} else {
		unk10->perform(flags, gfx);
	}
}

void TTalkCursor::associateNPC(TBaseNPC* npc) { (void)npc; }
