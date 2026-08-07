#include <Enemy/Generator.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Enemy/Enemy.hpp>
#include <Enemy/Graph.hpp>
#include <Strategic/Strategy.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <dolphin/mtx.h>

TGenerator::TGenerator(const char* name)
    : JDrama::TViewObj(name)
{
	mManagerName = nullptr;
	mManager     = nullptr;
	mGraphName   = nullptr;
	mGraph       = nullptr;
	mTimerMax    = 1;
	mTimer       = 0;
}

void TGenerator::load(JSUMemoryInputStream& stream)
{
	JDrama::TNameRef::load(stream);

	stream.read(&mPos.x, 4);
	stream.read(&mPos.y, 4);
	stream.read(&mPos.z, 4);
	stream.read(&mRot.x, 4);
	stream.read(&mRot.y, 4);
	stream.read(&mRot.z, 4);

	JGeometry::TVec3<f32> tmp;
	stream.read(&tmp.x, 4);
	stream.read(&tmp.y, 4);
	stream.read(&tmp.z, 4);

	stream.readString();

	int count;
	stream.read(&count, 4);
	int loopMax = count;
	for (int i = 0; i < loopMax; ++i) {
		int unused;
		stream.read(&unused, 4);
		stream.readString();
	}

	mGraphName   = stream.readString();
	mManagerName = stream.readString();
	stream.read(&mTimerMax, 4);

	s32 max = mTimerMax;
	mTimer  = (s32)((f32)max * MsRandF());

	gpConductor->registerGenerator(this);
}

void TGenerator::perform(u32 param_1, JDrama::TGraphics* param_2)
{
	if (!(param_1 & 1))
		return;

	--mTimer;
	if (mTimer < 0)
		mTimer = mTimerMax;

	if (mTimer != 0)
		return;

	if (mManager == nullptr)
		mManager = gpConductor->getManagerByName(mManagerName);

	TSpineEnemy* enemy = ((TEnemyManager*)mManager)->getFarOutEnemy();
	if (enemy == nullptr)
		return;

	if (mGraph == nullptr)
		mGraph = gpConductor->getGraphByName(mGraphName);

	enemy->getTracer()->init(mGraph);

	JGeometry::TVec3<f32> rot(0.0f, 0.0f, 0.0f);
	JGeometry::TVec3<f32> samp(0.0f, 4.0f, 0.0f);
	Mtx mtx;
	MsMtxSetRotRPH(mtx, mRot.x, mRot.y, mRot.z);
	MTXMultVec(mtx, &samp, &samp);

	enemy->resetSRTV(mPos, rot, enemy->mScaling, samp);
}

TOneShotGenerator::TOneShotGenerator(const char* name)
    : THitActor(name)
{
	mManagerName = nullptr;
	mManager     = nullptr;
	mGraphName   = nullptr;
	mGraph       = nullptr;
	mPending     = 1;
}

void TOneShotGenerator::load(JSUMemoryInputStream& stream)
{
	JDrama::TActor::load(stream);
	mGraphName   = stream.readString();
	mManagerName = stream.readString();
}

void TOneShotGenerator::loadAfter()
{
	if (mCollisions != nullptr)
		return;

	mManager = gpConductor->getManagerByName(mManagerName);

	if (mGraph == nullptr)
		mGraph = gpConductor->getGraphByName(mGraphName);

	initHitActor(0x02000001, 1, -0x80000000, 80.0f, 120.0f, 80.0f, 120.0f);

	offHitFlag(HIT_FLAG_NO_COLLISION);
	JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ")
	    ->getChildren()
	    .push_back(this);

	gpConductor->registerOtherObj(this);
}

BOOL TOneShotGenerator::receiveMessage(THitActor* sender, u32 message)
{
	if (sender->isActorType(0x01000001)) {
		if (mPending != 0) {
			TSpineEnemy* enemy
			    = ((TEnemyManager*)mManager)->getFarOutEnemy();
			if (enemy != nullptr) {
				enemy->getTracer()->init(mGraph);

				JGeometry::TVec3<f32> rot(0.0f, 0.0f, 0.0f);
				JGeometry::TVec3<f32> samp(0.0f, 4.0f, 0.0f);
				Mtx mtx;
				MsMtxSetRotRPH(mtx, mRotation.x, mRotation.y,
				               mRotation.z);
				MTXMultVec(mtx, &samp, &samp);

				enemy->resetSRTV(mPosition, rot, enemy->mScaling, samp);
			}
			mPending = 0;
		}
		return TRUE;
	}
	return FALSE;
}
