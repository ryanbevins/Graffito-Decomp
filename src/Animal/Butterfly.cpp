#include <Animal/Realoid.hpp>
#include <Animal/BoidLeader.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Enemy/Enemy.hpp>
#include <Strategic/ObjManager.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Strategy.hpp>
#include <Enemy/Launcher.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <MoveBG/MapObjBase.hpp>
#include <MoveBG/ItemManager.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <System/EmitterViewObj.hpp>
#include <Player/MarioAccess.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>

class TButterfloid;

class TButterfly : public TRealoidActor {
public:
	TButterfly(MActor* actor, TButterfloid* floid)
	    : TRealoidActor(actor)
	    , mFloid(floid)
	{
	}

	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void init();

	/* 0xA8 */ TButterfloid* mFloid;
};

class TButterfloid : public TRealoid {
public:
	TButterfloid(int, const char*);
	virtual TRealoidActor* createRealoidActor(MActor*);
	virtual void load(JSUMemoryInputStream&);
	virtual void init(class TLiveManager*);

	/* 0x158 */ int mModelType;
	/* 0x15C */ TMapObjBase* mCoinObj;
	/* 0x160 */ int mDeadCount;
};

class TButterfloidManager : public TEnemyManager {
public:
	TButterfloidManager(const char*);
	virtual void createModelData();
};

namespace {
const char* const cButterflyMdlNames[] = {
	"butterflyA.bmd",
	"butterflyB.bmd",
	"butterflyC.bmd",
};
}

TButterfloidManager::TButterfloidManager(const char* name)
    : TEnemyManager(name)
{
}

void TButterfloidManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "butterflyA.bmd", 0x10210000, 0 },
		{ "butterflyB.bmd", 0x10210000, 0 },
		{ "butterflyC.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TRealoidActor* TButterfloid::createRealoidActor(MActor* actor)
{
	return new TButterfly(actor, this);
}

void TButterfloid::load(JSUMemoryInputStream& stream)
{
	loadDefault(stream, cButterflyMdlNames[mModelType], 0);

	u32 eventId;
	stream.read(&eventId, 4);

	switch (mModelType) {
	case 0:
		mCoinObj = TMapObjBaseManager::newAndRegisterObj("coin");
		break;
	case 1:
		mCoinObj = TMapObjBaseManager::newAndRegisterObjByEventID(eventId, "");
		break;
	case 2:
		mCoinObj = TMapObjBaseManager::newAndRegisterObj("mushroom1upR");
		break;
	}

	mBoidLeader->mParam24 = 100.0f;
	mBoidLeader->mParam20 = 6.0f;
	mBoidLeader->mParam28 = 25.0f;
	mBoidLeader->mParam2C = 8.0f;
	mBoidLeader->mParam30 = 16.0f;
	mBoidLeader->mParam34 = 0.7f;

	mBoidLeader->mRepelTarget = (THitActor*)gpMarioAddress;
	mBoidLeader->mRepelRange = 500.0f;
	mBoidLeader->mRepelForce = 2.0f;
	mBoidLeader->mFlags |= 2;

	for (int i = 0; i < mBoidLeader->mNumActors; i++) {
		TButterfly* butterfly = (TButterfly*)mActors[i];
		butterfly->mMActor->setBck("butterfly_fly");
	}

	for (int i = 0; i < mBoidLeader->mNumActors; i++) {
		TButterfly* butterfly = (TButterfly*)mActors[i];
		butterfly->init();
	}
}

void TButterfloid::init(TLiveManager* manager)
{
	mManager = manager;
	mManager->manageActor(this);
	mSpine->initWith(&TNerveWaitForever<TLiveActor>::theNerve());
	initHitActor(0, 1, 0, 0.0f, 0.0f, 0.0f, 0.0f);
	unk64 |= 1;
}

TButterfloid::TButterfloid(int count, const char* name)
    : TRealoid(name)
{
	mModelType = count;
	mCoinObj   = nullptr;
	mDeadCount = 0;
}

BOOL TButterfly::receiveMessage(THitActor* sender, u32 msg)
{
	switch (msg) {
	case 4:
		if (mHolder == nullptr) {
			mHolder = (TTakeActor*)sender;
			gpMarioParticleManager->emit(0xe7, &mPosition, 0, 0);
			return 1;
		}
		break;
	case 8:
		if (mHolder != nullptr) {
			mHolder = nullptr;
			return 1;
		}
		break;
	case 0xb: {
		TButterfloid* floid = mFloid;
		unk74 |= 4;
		onHitFlag(1);
		if (++floid->mDeadCount == floid->mBoidLeader->mNumActors) {
			TMapObjBase* coin;
			if ((coin = floid->mCoinObj) != nullptr) {
				if (coin->isActorType(0x2000000e))
					coin = gpItemManager->makeObjAppear(0x2000000e);
				if (coin != nullptr) {
					coin->appear();
					coin->mPosition = mPosition;
					coin->mVelocity.set(0.0f, 15.0f, 0.0f);
					coin->offLiveFlag(0x10);
				}
			}
		}
		return 1;
	}
	default:
		break;
	}
	return 0;
}

void TButterfly::init()
{
	initHitActor(0x10000030, 0, 0, 0.0f, 0.0f, 50.0f, 50.0f);
	offHitFlag(1);
	onHitFlag(2);
	JDrama::TNameRefGen::search<TIdxGroupObj>(
	    "\x93\x47\x83\x4f\x83\x8b\x81\x5b\x83\x76")
	    ->getChildren()
	    .push_back(this);
}
