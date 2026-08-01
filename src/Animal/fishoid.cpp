#include <Animal/fishoid.hpp>
#include <Animal/BoidLeader.hpp>
#include <Enemy/Enemy.hpp>
#include <Enemy/Graph.hpp>
#include <Enemy/Launcher.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/ObjManager.hpp>
#include <Strategic/Spine.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <MoveBG/MapObjBase.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MoveBG/Item.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <Camera/Camera.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <dolphin/mtx.h>

namespace {
const char* const cFishoidMdlNames[] = {
	"fishA.bmd",
	"fishB.bmd",
	"fishC.bmd",
	"fishD.bmd",
};
}

TFishoidManager::TFishoidManager(const char* name)
    : TEnemyManager(name)
{
}

void TFishoidManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "fishA.bmd", 0x10210000, 0 },
		{ "fishB.bmd", 0x10210000, 0 },
		{ "fishC.bmd", 0x10210000, 0 },
		{ "fishD.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TRealoidActor::TRealoidActor(MActor* actor)
    : TTakeActor("boid")
    , mMActor(actor)
    , unk74(0)
{
}

TRealoid::TRealoid(const char* name)
    : TSpineEnemy(name)
{
	mBoidLeader = nullptr;
	onLiveFlag(0x38);
}

TFishoid::TFishoid(int count, const char* name)
    : TRealoid(name)
{
	mModelType = count;
	mCoinObj   = nullptr;
}

TRealoidActor* TFishoid::createRealoidActor(MActor* actor)
{
	return new TFish(actor);
}

void TFishoid::load(JSUMemoryInputStream& stream)
{
	loadDefault(stream, cFishoidMdlNames[mModelType], 0);

	u32 eventId;
	stream.read(&eventId, 4);
	mCoinObj = TMapObjBaseManager::newAndRegisterObjByEventID(eventId, "");
	if (mCoinObj != nullptr) {
		if (mCoinObj->isActorType(0x2000000e))
			mCoinObj = gpItemManager->newAndRegisterCoinReal();
	}

	mBoidLeader->mParam20 = 4.0f;
	mBoidLeader->mParam24 = 200.0f;
	mBoidLeader->mParam28 = 1.0f;
	mBoidLeader->mParam2C = 0.5f;
	mBoidLeader->mParam30 = 5.0f;
	mBoidLeader->mParam34 = 0.5f;

	mBoidLeader->mRepelTarget = (THitActor*)gpMarioAddress;
	mBoidLeader->mRepelRange = 400.0f;
	mBoidLeader->mRepelForce = 3.0f;
	mBoidLeader->mFlags |= 2;

	for (int i = 0; i < mBoidLeader->mNumActors; i++)
		mActors[i]->mMActor->setBck("fish_swim");

	if (mCoinObj != nullptr) {
		TRealoidActor* last = mActors[mBoidLeader->mNumActors - 1];
		last->unk74 |= 2;
		mCoinObj->makeObjAppeared();
		mCoinObj->mPosition = last->mPosition;
	}
}

void TFishoid::init(TLiveManager* manager)
{
	mManager = manager;
	mManager->manageActor(this);
	mSpine->initWith(&TNerveWaitForever<TLiveActor>::theNerve());
	initHitActor(0, 1, 0, 0.0f, 0.0f, 0.0f, 0.0f);
	unk64 |= 1;
}

void TFishoid::perform(u32 flags, JDrama::TGraphics* gfx)
{
	mBoidLeader->perform(flags, gfx);

	if (flags & 2) {
		clipBoids(gfx);
		for (int i = 0; i < mBoidLeader->getBoidNum(); i++)
			mActors[i]->calcRootMatrix(mBoidLeader->getBoid(i));
	}

	for (int i = 0; i < mBoidLeader->mNumActors; i++)
		mActors[i]->perform(flags, gfx);

	for (int i = 0; i < mBoidLeader->mNumActors; i++) {
		TBoid* boid = mBoidLeader->getBoid(i);
		JGeometry::TVec3<f32> pos;
		pos = boid->mPosition;
		if (pos.y > 0.0f)
			pos.y = 0.0f;
		boid->mPosition = pos;
	}

	if (mCoinObj != nullptr && (flags & 1))
		mCoinObj->mPosition
		    = mBoidLeader->mBoidData[mBoidLeader->mNumActors - 1].mPosition;
}

void TFish::init()
{
	unk64 |= 1;
}

void TRealoid::loadDefault(JSUMemoryInputStream& stream, const char* model_name,
                           int count)
{
	TSpineEnemy::load(stream);

	int num;
	stream.read(&num, 4);

	mMActorKeeper = new TMActorKeeper(mManager, (u16)(num + count));
	mBoidLeader = new TBoidLeader(
	    num, "\x83\x52\x83\x93\x83\x67\x83\x8d\x81\x5b\x83\x89");

	TBoidLeader* leader = mBoidLeader;
	leader->mGoalTarget = mPosition;
	mBoidLeader->setGraph(getTracer()->getGraph(), mPosition);

	mActors = new TRealoidActor*[num];

	TMActorKeeper* keeper = mMActorKeeper;
	JGeometry::TVec3<f32> pos = mPosition;
	for (int i = 0; i < num; i++) {
		MActor* mactor = keeper->createMActor(model_name, 3);
		mBoidLeader->mBoidData[i].mPosition = pos;
		mActors[i]                          = createRealoidActor(mactor);
		pos.y += 10.0f;
	}
}

void TRealoid::perform(u32 flags, JDrama::TGraphics* gfx)
{
	mBoidLeader->perform(flags, gfx);

	if (flags & 2) {
		SetViewFrustumClipCheckPerspective(gpCamera->getFovy(),
		                                   gpCamera->getAspect(),
		                                   gfx->mNearPlane, 10000.0f);
		for (int i = 0; i < mBoidLeader->mNumActors; i++) {
			JGeometry::TVec3<f32> pos;
			pos = mBoidLeader->mBoidData[i].mPosition;
			if (ViewFrustumClipCheck(gfx, (Vec*)&pos, 100.0f))
				mActors[i]->unk74 &= ~1;
			else
				mActors[i]->unk74 |= 1;
		}
		for (int i = 0; i < mBoidLeader->mNumActors; i++)
			mActors[i]->calcRootMatrix(&mBoidLeader->mBoidData[i]);
	}

	for (int i = 0; i < mBoidLeader->mNumActors; i++)
		mActors[i]->perform(flags, gfx);
}

void TRealoid::clipBoids(JDrama::TGraphics* gfx)
{
	SetViewFrustumClipCheckPerspective(gpCamera->getFovy(),
	                                   gpCamera->getAspect(), gfx->mNearPlane,
	                                   10000.0f);
	for (int i = 0; i < mBoidLeader->getBoidNum(); i++) {
		JGeometry::TVec3<f32> pos;
		pos = mBoidLeader->getBoid(i)->mPosition;
		if (ViewFrustumClipCheck(gfx, (Vec*)&pos, 100.0f))
			mActors[i]->unk74 &= ~1;
		else
			mActors[i]->unk74 |= 1;
	}
}

MtxPtr TRealoidActor::getTakingMtx()
{
	return mTakingMtx;
}

void TRealoidActor::perform(u32 flags, JDrama::TGraphics* gfx)
{
	if (!(unk74 & 6)) {
		THitActor::perform(flags, gfx);
		if (!(unk74 & 1))
			mMActor->perform(flags, gfx);
	}
}

void TRealoidActor::checkHitActors()
{
	if (unk74 & 6)
		return;

	THitActor** p   = mCollisions;
	THitActor** end = mCollisions + mColCount;
	s32 targetType  = 0x80000000;
	targetType += 1;
	for (; p != end; p++) {
		if ((s32)(*p)->mActorType != targetType)
			continue;
		SMS_SendMessageToMario(this, 0xe);
	}
}

void TRealoidActor::calcRootMatrix(TBoid* boid)
{
	if (unk74 & 6)
		return;

	mPosition = boid->mPosition;

	if (mHolder != nullptr) {
		MtxPtr holderMtx = mHolder->getTakingMtx();
		JGeometry::TVec3<f32> holderPos;
		holderPos.x    = holderMtx[0][3];
		holderPos.y    = holderMtx[1][3];
		holderPos.z    = holderMtx[2][3];
		boid->mPosition = holderPos;
		mMActor->getModel()->setBaseTRMtx(mHolder->getTakingMtx());
		return;
	}

	mPosition = boid->mPosition;
	JGeometry::TVec3<f32> trans = boid->mPosition;

	MtxPtr root = mMActor->getModel()->getBaseTRMtx();

	JGeometry::TVec3<f32> up(0.0f, 1.0f, 0.0f);
	JGeometry::TVec3<f32> dir = boid->mForward;
	JGeometry::TVec3<f32> n;

	n.cross(up, dir);
	VECNormalize(&n, &n);

	up.cross(dir, n);
	VECNormalize(&up, &up);

	dir.cross(n, up);
	VECNormalize(&dir, &dir);

	root[0][0] = -dir.x;
	root[1][0] = -dir.y;
	root[2][0] = -dir.z;
	root[0][1] = up.x;
	root[1][1] = up.y;
	root[2][1] = up.z;
	root[0][2] = n.x;
	root[1][2] = n.y;
	root[2][2] = n.z;
	root[0][3] = trans.x;
	root[1][3] = trans.y;
	root[2][3] = trans.z;
}
