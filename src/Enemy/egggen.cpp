#include <Enemy/EggGen.hpp>
#include <Player/Yoshi.hpp>
#include <Strategic/ObjModel.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <dolphin/mtx.h>

#include <M3DUtil/InfectiousStrings.hpp>

#include <Player/MarioMain.hpp>

TEggGenManager::TEggGenManager(const char* name)
    : TEnemyManager(name)
{
}

void TEggGenManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TSpineEnemyParams("/enemy/egggen.prm");
	TEnemyManager::load(stream);
}

void TEggGenManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "gene_egg_model1.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};

	createModelDataArray(entry);
}

TEggGenerator::TEggGenerator(const char* name)
    : TSpineEnemy(name)
{
	mLiveFlag |= LIVE_FLAG_UNK10;
	mLiveFlag |= LIVE_FLAG_UNK8;
}

void TEggGenerator::init(TLiveManager* param_1)
{
	mManager = param_1;
	mManager->manageActor(this);

	mMActorKeeper = new TMActorKeeper(mManager, 1);
	mMActor       = mMActorKeeper->createMActor("gene_egg_model1.bmd", 0);

	initHitActor(0x02000001, 1, 0x80000000, 10.0f, 10.0f, 10.0f, 10.0f);

	mMActor->setBckFromIndex(0);

	mRotation.x = MsWrap(mRotation.x - 90.0f, 0.0f, 360.0f);
}

// fabricated
static inline BOOL isYoshiAppeared(TYoshi* yoshi)
{
	return (u8)yoshi->mState == TYoshi::EGG ? 0 : 1;
}

// fabricated
static inline BOOL isMarioYoshiAppeared()
{
	return isYoshiAppeared(gpMarioOriginal->mYoshi);
}

// fabricated
static inline f32 squareDistanceToMario(Vec* pos)
{
	return PSVECSquareDistance(pos, &gpMarioOriginal->mPosition);
}

void TEggGenerator::control()
{
	f32 dist = squareDistanceToMario(&mPosition);

	if (dist < 250000.0f) {
		BOOL result = isMarioYoshiAppeared();
		if (!result) {
			mMActor->setBckFromIndex(0);
		}
	}
}
