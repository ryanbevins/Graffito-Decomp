#include <Enemy/EggGen.hpp>
#include <Player/Yoshi.hpp>
#include <Strategic/ObjModel.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <dolphin/mtx.h>

#include <M3DUtil/InfectiousStrings.hpp>

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
	static TModelDataLoadEntry entry[] = {
		{ "gene_egg_model1.bmd", 0, 0 },
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

	initHitActor(0x02000001, 1, 0x80000000, 150.0f, 150.0f, 150.0f, 150.0f);

	mMActor->setBckFromIndex(0);

	mRotation.x = MsWrap(mRotation.x - 90.0f, 0.0f, 360.0f);
}

void TEggGenerator::control()
{
	f32 _pad[6];
	(void)_pad;

	f32 dist = PSVECSquareDistance((Vec*)&mPosition, (Vec*)(gpMarioAddress + 0x10));

	if (dist < 250000.0f) {
		TYoshi* yoshi = *(TYoshi**)(gpMarioAddress + 0x3F0);
		int result;
		if ((u8)yoshi->mState == 0) {
			result = 0;
		} else {
			result = 1;
		}
		if (!result) {
			mMActor->setBckFromIndex(0);
		}
	}
}
