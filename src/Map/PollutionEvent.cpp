#include <Map/PollutionTest.hpp>

// rogue includes needed for matching __sinit & .bss layout
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

void TPollutionTest::loadAfter() { JDrama::TNameRef::loadAfter(); }

void TPollutionTest::perform(u32 flags, JDrama::TGraphics* gfx)
{
	(void)flags;
	(void)gfx;
}
