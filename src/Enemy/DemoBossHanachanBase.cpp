#include <Enemy/DemoBossHanachan.hpp>
#include <Strategic/ObjModel.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <Map/Map.hpp>

TDemoBossHanachanSaveParams::TDemoBossHanachanSaveParams(const char* name)
    : TParams(name)
    , PARAM_INIT(mSLViewClipFar, 25000.0f)
    , PARAM_INIT(mSLViewClipRadius, 3500.0f)
{
	TParams::load(mPrmPath);
}

void TDemoBossHanachanManager::clipEnemies(JDrama::TGraphics* gfx)
{
	clipActorsAux(gfx, mSaveParams->mSLViewClipFar.get(),
	              mSaveParams->mSLViewClipRadius.get());
}

void TDemoBossHanachan::initBase(TLiveManager* manager, u32 flags)
{
	mManager = manager;
	manager->manageActor(this);

	mMActorKeeper = new TMActorKeeper(manager);
	mMActor       = mMActorKeeper->createMActorFromNthData(0, flags);

	mBodyScale        = 1.0f;
	mBodyRadius       = 280.0f;
	mWallRadius       = mBodyRadius;
	mHeadHeight       = 200.0f;
	mMarchSpeed       = 0.0f;
	mScaledBodyRadius = 1000.0f;

	mLiveFlag |= 0x18;

	f32 scaledHead = mBodyScale * mHeadHeight;
	mGroundHeight  = gpMap->checkGroundIgnoreWaterSurface(
	     mPosition.x, mPosition.y + scaledHead, mPosition.z, &mGroundPlane);

	mMActor->setLightType(1);
}

BOOL TDemoBossHanachan::receiveMessage(THitActor*, u32) { return FALSE; }
