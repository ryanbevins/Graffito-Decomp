#include <Animal/AnimalBase.hpp>

TAnimalBase::TAnimalBase(u32 type, const char* name)
    : TSpineEnemy(name)
{
	(void)type;
	mFrameTimer = nullptr;
}

void TAnimalBase::load(JSUMemoryInputStream& stream)
{
	TSpineEnemy::load(stream);
}

void TAnimalBase::loadAfter() { TSpineEnemy::loadAfter(); }

void TAnimalBase::init(TLiveManager* mgr) { TSpineEnemy::init(mgr); }

void TAnimalBase::perform(u32 flags, JDrama::TGraphics* gfx)
{
	(void)flags;
	(void)gfx;
}

BOOL TAnimalBase::receiveMessage(THitActor* sender, u32 msg)
{
	(void)sender;
	(void)msg;
	return 0;
}

void TAnimalBase::calcRootMatrix() { }

void TAnimalBase::execWalk(bool flag) { (void)flag; }

void TAnimalBase::getRotationFlyToDir(JGeometry::TVec3<f32>* outQuat,
                                       const JGeometry::TVec3<f32>& dir,
                                       f32 minDot, f32 maxDot)
{
	(void)outQuat;
	(void)dir;
	(void)minDot;
	(void)maxDot;
}

void TAnimalBase::resetRandomCurPathNode() { }
