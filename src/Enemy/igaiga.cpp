#include <Enemy/Igaiga.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/Graph.hpp>
#include <Enemy/Launcher.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <Player/ModelWaterManager.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/SharedParts.hpp>
#include <Strategic/Spine.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/Particles.hpp>
#include <stdlib.h>

class JAISound;
namespace MSoundSESystem {
class MSoundSE {
public:
	static void startSoundActor(u32, const Vec*, u32, JAISound**, u32, u8);
};
}

static TRollEnemy* gpCurRollEnemy;

f32 TRollEnemy::mBoundVal     = 80.0f;
f32 TRollEnemy::mTransYOffset = 0.0f;
f32 TIgaiga::mReachNodeDist   = 300.0f;

static const char* igaiga_bastable[] = {
	"/scene/igaiga/bas/igaiga_down1.bas",
	"/scene/igaiga/bas/igaiga_down2.bas",
	nullptr,
	nullptr,
	"/scene/igaiga/bas/igaiga_shoot1.bas",
	"/scene/igaiga/bas/igaiga_waterdown1.bas",
	"/scene/igaiga/bas/igaiga_waterhit1.bas",
	nullptr,
};

static const char* gorogoro_bastable[] = {
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

static const char* anmlist[] = { "bosspaku_head_move", nullptr };
static const char* graphlist[] = { "gorogoro0", "gorogoro1" };

DEFINE_NERVE(TNerveGorogoroDie, TLiveActor)
{
	TGorogoro* self = (TGorogoro*)spine->getBody();

	if (spine->getTime() < 2) {
		self->onLiveFlag(LIVE_FLAG_CLIPPED_OUT);
		if (self->isRolling())
			self->bound();
	}

	if (self->checkCurAnmEnd(0) || spine->getTime() > 360) {
		self->onLiveFlag(LIVE_FLAG_CLIPPED_OUT);
		self->onLiveFlag(0x10000);
		self->offLiveFlag(0x4);
		self->stopAnmSound();
		spine->setNext(&TNerveSmallEnemyDie::theNerve());
		self->setAfterDeadEffect();
		return TRUE;
	}

	if (self->checkLiveFlag(0x10000))
		self->walkBehavior(2, 0.5f);

	return FALSE;
}

DEFINE_NERVE(TNerveGorogoroRollOnGraph, TLiveActor)
{
	TGorogoro* self = (TGorogoro*)spine->getBody();

	if (spine->getTime() == 0) {
		self->goToShortestNextGraphNode();
		self->setBckAnm(2);
	}

	self->walkBehavior(0, 0.0f);
	return FALSE;
}

DEFINE_NERVE(TNerveIgaigaShootFromCannon, TLiveActor)
{
	TIgaiga* self = (TIgaiga*)spine->getBody();

	if (spine->getTime() == 0) {
		self->offLiveFlag(LIVE_FLAG_CLIPPED_OUT);
		self->setBckAnm(4);
	}

	self->moveObject();
	if (self->isReachedToGoalXZ()) {
		spine->pushAfterCurrent(&TNerveIgaigaRollOnGraph::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveIgaigaWaterHit, TLiveActor)
{
	TIgaiga* self = (TIgaiga*)spine->getBody();

	if (spine->getTime() == 0)
		self->setBckAnm(6);

	self->walkBehavior(0, 0.0f);
	if (self->checkCurAnmEnd(0) || spine->getTime() > 120) {
		spine->pushAfterCurrent(&TNerveIgaigaRollOnGraph::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveIgaigaRollOnGraph, TLiveActor)
{
	TIgaiga* self = (TIgaiga*)spine->getBody();

	if (spine->getTime() == 0) {
		self->goToShortestNextGraphNode();
		self->setBckAnm(0);
	}

	self->walkBehavior(0, 0.0f);
	return FALSE;
}

TRollEnemy::TRollEnemy(const char* name)
    : TWalkerEnemy(name)
    , unk194(0.0f)
    , unk198(0.0f)
    , unk19C(0.0f)
    , unk1A0(0.0f)
    , unk1A4(0)
    , unk1A8(false)
    , unk1AC(0.0f)
    , unk1B0(1.0f)
{
}

void TRollEnemy::setBehavior()
{
	if (isReachedToGoalXZ())
		goToShortestNextGraphNode();

	walkBehavior(0, 0.0f);
}

bool TRollEnemy::isReachedToGoalXZ()
{
	JGeometry::TVec3<f32> diff = unk104.getPoint();
	diff.y                     = mPosition.y;
	diff.sub(mPosition);
	return MsVECMag2((Vec*)&diff) < 90000.0f;
}

bool TRollEnemy::isCollidMove(THitActor* actor)
{
	return (size_t)actor != gpMarioAddress ? true : false;
}

void TRollEnemy::attackToMario()
{
	sendAttackMsgToMario();
}

void TRollEnemy::behaveToWater(THitActor*)
{
	unk1A8 = true;
	setBckAnm(6);
}

void TRollEnemy::flagJump()
{
	unk1A8 = true;
	mVelocity.y = getRollParams()->mSLBoundVYMax.get();
}

void TRollEnemy::walkBehavior(int, f32)
{
	if (unk1A8) {
		mPosition.y += mVelocity.y;
		mVelocity.y -= mGravity;
		if (mVelocity.y < -getRollParams()->mSLBoundVYMax.get())
			mVelocity.y = -getRollParams()->mSLBoundVYMax.get();
	} else {
		walkToCurPathNode(mMarchSpeed, mTurnSpeed, 0.0f);
	}

	unk194 += unk198;
	if (unk194 >= 360.0f)
		unk194 -= 360.0f;

	if (unk128 > 300) {
		onLiveFlag(0x10000);
		kill();
	}
}

void TRollEnemy::reset()
{
	gpCurRollEnemy = this;
	TWalkerEnemy::reset();

	unk194 = MsRandF(0.0f, 360.0f);
	unk158 = 1.0f;

	if (unk124 && unk124->getGraph()) {
		JGeometry::TVec3<f32> point = unk124->getGraph()->getFirstGraphNode().getPoint();
		mPosition = point;
		mPosition.y += 10.0f;

		JGeometry::TVec3<f32> next = unk124->getGraph()->getGraphNode(1).getPoint();
		JGeometry::TVec3<f32> dir  = next;
		dir.sub(mPosition);
		mRotation.y = MsGetRotFromZaxisY(dir);
	}

	unk198 = mMarchSpeed * 1.5f;
	unk19C = mMarchSpeed;
	unk1A0 = 0.0f;
	if (unk124)
		unk124->mCurrIdx = 0;
}

TIgaiga::TIgaiga(const char* name)
    : TRollEnemy(name)
    , unk1B4(0)
    , unk1B8(nullptr)
    , unk1BC(true)
    , unk1CC(1.0f)
    , unk1D0(nullptr)
    , unk1E4(1.0f)
    , unk1E8(0)
{
}

void TIgaiga::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TSmallEnemy::perform(flags, graphics);
}

void TIgaiga::calcRootMatrix()
{
	gpCurRollEnemy = this;
	TSpineEnemy::calcRootMatrix();
}

void TIgaiga::setMActorAndKeeper()
{
	mMActorKeeper = new TMActorKeeper(mManager, 1);
	mMActor       = mMActorKeeper->createMActor("igaiga_model1.bmd", 0);
}

void TIgaiga::init(TLiveManager* manager)
{
	TWalkerEnemy::init(manager);
	mActorType = 0x10000017;
	unk150      = 0x11;
	mLiveFlag &= ~0x78000000;
	mLiveFlag |= 0x40000000;
	mSpine->initWith(&TNerveIgaigaRollOnGraph::theNerve());
	unk1A4 = getUnkF4().unk0 ? 1 : 0;

	mMActor->setJointCallback(1, RollEnemyBodyCallback);
	mMActor->setBtkFromIndex(0);
	unk124->setGraph(gpConductor->getGraphByName("igaiga"));
}

void TIgaiga::moveObject()
{
	TWalkerEnemy::moveObject();
}

void TIgaiga::kill()
{
	unk194 = 0.0f;
	TSmallEnemy::kill();
}

const char** TIgaiga::getBasNameTable() const
{
	return igaiga_bastable;
}

void TIgaiga::reset()
{
	TRollEnemy::reset();
	unk1BC = true;
	unk1E4 = 1.0f;
	unk1E8 = 0;
}

void TIgaiga::behaveToWater(THitActor* actor)
{
	TRollEnemy::behaveToWater(actor);
	mSpine->pushAfterCurrent(&TNerveIgaigaWaterHit::theNerve());
}

void TIgaiga::setWalkAnm()
{
	setBckAnm(3);
}

void TIgaiga::setDeadAnm()
{
	setBckAnm(1);
	((TIgaigaManager*)mManager)
	    ->mPolluteModelManager->generatePolluteModel(mPosition, mScaling);
}

void TIgaiga::setMeltAnm()
{
	setBckAnm(5);
}

bool TIgaiga::isHitValid(u32 message)
{
	return message == 0xE ? false : TSmallEnemy::isHitValid(message);
}

bool TIgaiga::isReachedToGoalXZ()
{
	JGeometry::TVec3<f32> diff = unk104.getPoint();
	diff.y                     = mPosition.y;
	diff.sub(mPosition);
	return MsVECMag2((Vec*)&diff) < mReachNodeDist * mReachNodeDist;
}

void TIgaiga::walkBehavior(int mode, f32 speed)
{
	TRollEnemy::walkBehavior(mode, speed);
}

void TIgaiga::bound()
{
	unk1A8      = true;
	mVelocity.y = TRollEnemy::mBoundVal;
}

bool TIgaiga::isRolling()
{
	const TNerveBase<TLiveActor>* current = mSpine->getCurrentNerve();
	return current == &TNerveIgaigaRollOnGraph::theNerve()
	    || current == &TNerveIgaigaWaterHit::theNerve();
}

void TIgaiga::rollSE()
{
	MSoundSESystem::MSoundSE::startSoundActor(0x285E, &mPosition, 0, nullptr,
	                                          0, 4);
}

void TIgaiga::boundSE()
{
	MSoundSESystem::MSoundSE::startSoundActor(0x285F, &mPosition, 0, nullptr,
	                                          0, 4);
}

void TIgaiga::shoot(JGeometry::TVec3<f32>& velocity)
{
	mVelocity = velocity;
	mSpine->setNext(&TNerveIgaigaShootFromCannon::theNerve());
}

void TGorogoro::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TSmallEnemy::perform(flags, graphics);
}

void TGorogoro::init(TLiveManager* manager)
{
	TRollEnemy::init(manager);
	mSpine->initWith(&TNerveGorogoroRollOnGraph::theNerve());
	mMActor->setJointCallback(1, RollEnemyBodyCallback);
}

void TGorogoro::calcRootMatrix()
{
	gpCurRollEnemy = this;
	TSpineEnemy::calcRootMatrix();
}

void TGorogoro::kill()
{
	mSpine->setNext(&TNerveGorogoroDie::theNerve());
}

const char** TGorogoro::getBasNameTable() const
{
	return gorogoro_bastable;
}

void TGorogoro::reset()
{
	TRollEnemy::reset();
	unk1E4 = false;
	unk1E8 = 0;
	unk1EC = false;
}

void TGorogoro::setMActorAndKeeper()
{
	mMActorKeeper = new TMActorKeeper(mManager, 1);
	mMActor       = mMActorKeeper->createMActor("bosspaku_head.bmd", 3);
}

void TGorogoro::behaveToWater(THitActor* actor)
{
	TRollEnemy::behaveToWater(actor);
}

void TGorogoro::setDeadAnm()
{
	setBckAnm(0);
}

void TGorogoro::setMeltAnm()
{
	setBckAnm(0);
}

void TGorogoro::forceKill()
{
	mSpine->setNext(&TNerveGorogoroDie::theNerve());
}

void TGorogoro::walkBehavior(int mode, f32 speed)
{
	TRollEnemy::walkBehavior(mode, speed);
}

void TGorogoro::flagJump()
{
	TRollEnemy::flagJump();
}

void TGorogoro::bound()
{
	unk1A8      = true;
	mVelocity.y = TRollEnemy::mBoundVal;
}

bool TGorogoro::isRolling()
{
	return mSpine->getCurrentNerve() == &TNerveGorogoroRollOnGraph::theNerve();
}

void TGorogoro::rollSE()
{
	MSoundSESystem::MSoundSE::startSoundActor(0x285E, &mPosition, 0, nullptr,
	                                          0, 4);
}

void TGorogoro::boundSE()
{
	MSoundSESystem::MSoundSE::startSoundActor(0x285F, &mPosition, 0, nullptr,
	                                          0, 4);
}

void TGorogoro::generateByGateKeeper(const JGeometry::TVec3<f32>& position,
                                     const JGeometry::TVec3<f32>& velocity)
{
	mPosition = position;
	mVelocity = velocity;
	offLiveFlag(LIVE_FLAG_CLIPPED_OUT);
	mSpine->setNext(&TNerveGorogoroRollOnGraph::theNerve());
}

void TIgaigaPolluteModel::setAnm()
{
	unk10->unk18->setBckFromIndex(7);
}

void TGorogoroPolluteModel::setAnm()
{
	unk10->unk18->setBckFromIndex(0);
}

void TIgaigaPolluteModelManager::init(TLiveActor* owner)
{
	TEnemyPolluteModelManager::init(owner);
}

void TGorogoroPolluteModelManager::init(TLiveActor* owner)
{
	TEnemyPolluteModelManager::init(owner);
}

TIgaigaManager::TIgaigaManager(const char* name)
    : TSmallEnemyManager(name)
    , unk64(nullptr)
    , mWaterEmitInfo(nullptr)
{
	gpCurRollEnemy = nullptr;
}

void TIgaigaManager::load(JSUMemoryInputStream& stream)
{
	TSmallEnemyManager::load(stream);
	unk38          = new TRollEnemySaveLoadParams("/enemy/igaiga.prm");
	mWaterEmitInfo = new TWaterEmitInfo("/enemy/igaigawater.prm");
}

void TIgaigaManager::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TSmallEnemyManager::perform(flags, graphics);
	mPolluteModelManager->perform(flags, graphics);
}

void TIgaigaManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "igaiga_model1.bmd", 0x11240000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TSpineEnemy* TIgaigaManager::createEnemyInstance()
{
	return new TIgaiga("イガイガ");
}

void TIgaigaManager::initSetEnemies()
{
	mPolluteModelManager = new TIgaigaPolluteModelManager("イガイガモデル汚染");
	mPolluteModelManager->init(getObj(0));
}

TGorogoroManager::TGorogoroManager(const char* name)
    : TSmallEnemyManager(name)
    , unk60(nullptr)
    , unk64(nullptr)
    , unk68(true)
    , mPolluteModelManager(nullptr)
    , unk70(nullptr)
{
}

void TGorogoroManager::load(JSUMemoryInputStream& stream)
{
	TSmallEnemyManager::load(stream);
	unk38 = new TRollEnemySaveLoadParams("/enemy/gorogoro.prm");
	createSharedMActorSet(anmlist);
}

void TGorogoroManager::loadAfter()
{
	unk64 = (TLiveActor*)JDrama::TNameRefGen::getInstance()
	           ->getRootNameRef()
	           ->search("イベント（寝込むビアンコ）");
	unk70 = (TLiveActor*)gpConductor->search("ゴロゴロ発生マネージャー");
}

void TGorogoroManager::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TSmallEnemyManager::perform(flags, graphics);
	if (mPolluteModelManager)
		mPolluteModelManager->perform(flags, graphics);
}

void TGorogoroManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "bosspaku_head.bmd", 0x10300000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TSpineEnemy* TGorogoroManager::createEnemyInstance()
{
	return new TGorogoro("ゴロゴロ");
}

void TGorogoroManager::initSetEnemies()
{
	mPolluteModelManager = new TGorogoroPolluteModelManager("ゴロゴロモデル汚染");
	mPolluteModelManager->init(getObj(0));

	for (int i = 0; i < getObjNum(); ++i) {
		TGorogoro* enemy = (TGorogoro*)getObj(i);
		TGraphWeb* graph = gpConductor->getGraphByName(graphlist[i & 1]);
		if (graph && !graph->isDummy()) {
			enemy->unk124->setGraph(graph);
			enemy->mPosition = graph->getFirstGraphNode().getPoint();
			enemy->unk1E8    = graph->getNodeNum() - 1;
		}
	}
}

int RollEnemyBodyCallback(J3DNode*, int timing)
{
	if (timing != 0)
		return 1;

	if (!gpCurRollEnemy || !gpCurRollEnemy->isRolling())
		return 1;

	gpCurRollEnemy->rollSE();
	return 1;
}
