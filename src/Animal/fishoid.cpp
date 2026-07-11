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
const char* cFishoidMdlNames[] = {
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
	if (mCoinObj != nullptr && mCoinObj->mActorType == 0x2000000e) {
		mCoinObj = gpItemManager->newAndRegisterCoinReal();
	}

	mBoidLeader->mParam20 = 4.0f;
	mBoidLeader->mParam24 = 200.0f;
	mBoidLeader->mParam28 = 1.0f;
	mBoidLeader->mParam2C = 0.5f;
	mBoidLeader->mParam30 = 5.0f;
	mBoidLeader->mParam34 = 0.5f;

	THitActor* mario = (THitActor*)gpMarioAddress;
	JGeometry::TVec3<f32> repelPos(0.0f, 0.0f, 0.0f);
	if (mario != nullptr)
		repelPos.set(mario->mPosition);

	mBoidLeader->mRepelActor = mario;
	mBoidLeader->mRepelPos   = repelPos;
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
		for (int i = 0; i < mBoidLeader->mNumActors; i++)
			mActors[i]->calcRootMatrix(&mBoidLeader->mBoidData[i]);
	}

	for (int i = 0; i < mBoidLeader->mNumActors; i++)
		mActors[i]->perform(flags, gfx);

	for (int i = 0; i < mBoidLeader->mNumActors; i++) {
		JGeometry::TVec3<f32> pos;
		pos = mBoidLeader->mBoidData[i].mPosition;
		if (pos.y > 0.0f)
			pos.y = 0.0f;
		mBoidLeader->mBoidData[i].mPosition = pos;
	}

	if (mCoinObj != nullptr && (flags & 1))
		mCoinObj->mPosition
		    = mBoidLeader->mBoidData[mBoidLeader->mNumActors - 1].mPosition;
}

void TFish::init()
{
	unk64 |= 1;
}

TRealoid::TRealoid(const char* name)
    : TSpineEnemy(name)
{
	mBoidLeader = nullptr;
	onLiveFlag(0x38);
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

	mBoidLeader->mGoalActor = nullptr;
	mBoidLeader->mGoalPos   = mPosition;
	mBoidLeader->setGraph(getTracer()->getGraph(), mPosition);

	mActors = new TRealoidActor*[num];

	JGeometry::TVec3<f32> pos = mPosition;
	for (int i = 0; i < num; i++) {
		MActor* mactor = mMActorKeeper->createMActor(model_name, 3);
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
	for (; p != end; p++) {
		if ((*p)->mActorType == 0x80000001)
			SMS_SendMessageToMario(this, 0xe);
	}
}

void TRealoidActor::calcRootMatrix(TBoid* boid)
{
	if (unk74 & 6)
		return;

	mPosition = boid->mPosition;

	if (mHolder != nullptr) {
		MtxPtr m = mHolder->getTakingMtx();
		JGeometry::TVec3<f32> t;
		t.set(m[0][3], m[1][3], m[2][3]);
		boid->mPosition = t;
		PSMTXCopy(mHolder->getTakingMtx(),
		          mMActor->getModel()->getBaseTRMtx());
		return;
	}

	mPosition = boid->mPosition;
	JGeometry::TVec3<f32> pos = boid->mPosition;

	MtxPtr dst = mMActor->getModel()->getBaseTRMtx();

	JGeometry::TVec3<f32> up(0.0f, 1.0f, 0.0f);
	JGeometry::TVec3<f32> fwd = boid->mForward;

	JGeometry::TVec3<f32> xaxis;
	xaxis.cross(up, fwd);
	PSVECNormalize((Vec*)&xaxis, (Vec*)&xaxis);

	JGeometry::TVec3<f32> yaxis;
	yaxis.x = fwd.y * xaxis.z - fwd.z * xaxis.y;
	yaxis.y = fwd.z * xaxis.x - fwd.x * xaxis.z;
	yaxis.z = fwd.x * xaxis.y - fwd.y * xaxis.x;
	PSVECNormalize((Vec*)&yaxis, (Vec*)&yaxis);

	JGeometry::TVec3<f32> zaxis;
	zaxis.x = xaxis.y * yaxis.z - xaxis.z * yaxis.y;
	zaxis.y = xaxis.z * yaxis.x - xaxis.x * yaxis.z;
	zaxis.z = xaxis.x * yaxis.y - xaxis.y * yaxis.x;
	PSVECNormalize((Vec*)&zaxis, (Vec*)&zaxis);

	dst[0][0] = -zaxis.x;
	dst[1][0] = -zaxis.y;
	dst[2][0] = -zaxis.z;
	dst[0][1] = yaxis.x;
	dst[1][1] = yaxis.y;
	dst[2][1] = yaxis.z;
	dst[0][2] = xaxis.x;
	dst[1][2] = xaxis.y;
	dst[2][2] = xaxis.z;
	dst[0][3] = pos.x;
	dst[1][3] = pos.y;
	dst[2][3] = pos.z;
}
