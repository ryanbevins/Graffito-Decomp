#include <Player/SplashManager.hpp>

// rogue includes for matching __sinit (15 JALList<T> templates)
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

void TSplashManager::load(JSUMemoryInputStream& stream) { (void)stream; }

void TSplashManager::perform(u32 flags, JDrama::TGraphics* gfx)
{
	if (flags & 2)
		move();
	if (mFlags & 1) {
		if (flags & 4)
			makeDL(gfx);
		if (flags & 8)
			draw();
	}
}

void TSplashManager::newSplash(JGeometry::TVec3<f32> pos, f32 size)
{
	(void)pos;
	(void)size;
}

#pragma dont_inline on
void TSplashManager::move() { }

void TSplashManager::makeDL(JDrama::TGraphics* gfx) const { (void)gfx; }

void TSplashManager::draw() const { }
#pragma dont_inline off
