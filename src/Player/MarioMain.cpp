#include <Player/MarioMain.hpp>
#include <Player/MarioAccess.hpp>
#include <MSound/MSoundBGM.hpp>
#include <JSystem/JGeometry.hpp>

static JGeometry::TVec3<f32> cDeformedTerrainCenter(0.0f, 5000.0f, 0.0f);

void TMario::drawSyncCallback(u16 token) { (void)token; }

void TMario::perform(u32 flags, JDrama::TGraphics* gfx)
{
	(void)flags;
	(void)gfx;
	// TODO: main Mario tick — calls into MarioMove/Jump/etc.
}

BOOL TMario::isMario()
{
	if (gpMarioOriginal == this)
		return TRUE;
	return FALSE;
}
