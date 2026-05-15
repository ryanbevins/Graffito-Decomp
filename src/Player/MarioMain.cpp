#include <Player/MarioMain.hpp>
#include <Player/MarioAccess.hpp>

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
