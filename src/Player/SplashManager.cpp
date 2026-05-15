#include <Player/SplashManager.hpp>

void TSplashManager::load(JSUMemoryInputStream& stream) { (void)stream; }

void TSplashManager::perform(u32 flags, JDrama::TGraphics* gfx)
{
	(void)flags;
	(void)gfx;
}

void TSplashManager::newSplash(JGeometry::TVec3<f32> pos, f32 size)
{
	(void)pos;
	(void)size;
}

void TSplashManager::move() { }

void TSplashManager::makeDL(JDrama::TGraphics* gfx) const { (void)gfx; }

void TSplashManager::draw() const { }
