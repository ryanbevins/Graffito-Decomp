#include <Player/SplashManager.hpp>
#include <System/StageUtil.hpp>

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
	if (mFreeList.getNumLinks() != 0 && !SMS_isDivingMap()) {
		JSULink<TWaterSplash>* link = mFreeList.getFirst();
		mFreeList.remove(link);
		mActiveList.append(link);

		TWaterSplash* splash = link->getObject();
		splash->mPos          = pos;
		splash->mVelY         = size;
		splash->mLife         = mInitLife;
	}
}

#pragma dont_inline on
void TSplashManager::move()
{
	if (mFlags & 2) {
		JSULink<TWaterSplash>* link = mActiveList.getFirst();
		while (link != NULL) {
			TWaterSplash* splash = link->getObject();
			splash->mVelY += mGravity;
			splash->mPos.y += splash->mVelY;
			if (splash->mLife != 0)
				splash->mLife -= 1;
			if (splash->mLife == 0) {
				JSULink<TWaterSplash>* next = link->getNext();
				mActiveList.remove(link);
				mFreeList.append(link);
				link = next;
			} else {
				link = link->getNext();
			}
		}
	}
}

void TSplashManager::makeDL(JDrama::TGraphics* gfx) const { (void)gfx; }

void TSplashManager::draw() const { }
#pragma dont_inline off
