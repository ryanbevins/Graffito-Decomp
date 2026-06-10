#include <Enemy/HanaSambo.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DNode.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <M3DUtil/MActor.hpp>
#include <Map/Map.hpp>
#include <MarioUtil/ShadowUtil.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <MSound/MSound.hpp>
#include <Player/MarioAccess.hpp>
#include <System/Application.hpp>
#include <System/EmitterViewObj.hpp>
#include <Strategic/ObjManager.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Strategy.hpp>

static const char* sambo_bastable[] = {
	"/scene/sambo/bas/sambo_down.bas",
	nullptr,
	nullptr,
	nullptr,
	"/scene/sambo/bas/sambo_Fhide.bas",
	nullptr,
	"/scene/sambo/bas/sambo_Fset.bas",
	"/scene/sambo/bas/sambo_Fwait.bas",
	"/scene/sambo/bas/sambo_hit.bas",
	nullptr,
	"/scene/sambo/bas/sambo_Ydown.bas",
};

static const char* sambohead_bastable[] = {
	"/scene/sambohead/bas/flower_shoot.bas",
	"/scene/sambohead/bas/samboHead_crash.bas",
	"/scene/sambohead/bas/samboHead_dance.bas",
	"/scene/sambohead/bas/samboHead_down.bas",
	"/scene/sambohead/bas/samboHead_Fhide.bas",
	"/scene/sambohead/bas/samboHead_hit.bas",
	"/scene/sambohead/bas/samboHead_hit_end.bas",
	"/scene/sambohead/bas/samboHead_jump_end.bas",
	"/scene/sambohead/bas/samboHead_jump_start.bas",
	nullptr,
	"/scene/sambohead/bas/samboHead_set.bas",
	"/scene/sambohead/bas/samboHead_turn.bas",
	nullptr,
};

u8 THanaSambo::mHeadJntIndex   = 3;
u8 THanaSambo::mPollenJntIndex = 6;
u8 TSamboHead::mBodyJntIndex;

static TSamboHead* gpCurSamboHead;

static int SamboHeadRollCallback(J3DNode*, int) { return 1; }

static inline void initMarioGoal(TSpineEnemy* sambo)
{
	THitActor* mario = (THitActor*)gpMarioAddress;
	JGeometry::TVec3<f32> pos(0.0f, 0.0f, 0.0f);
	if (mario)
		pos = mario->mPosition;

	sambo->unkF4.unk0  = mario;
	sambo->unkF4.unk4  = pos;
	sambo->unk104.unk0 = mario;
	sambo->unk104.unk4 = pos;
	sambo->unk114.clear();
}

DEFINE_NERVE(TNerveSamboHeadHitWall, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveSamboHeadRecoverWater, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveSamboHeadHitWater, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveSamboHeadHide, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveSamboHeadAttack, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveSamboHeadAppear, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveHanaSamboFreeze, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveHanaSamboDie, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveHanaSamboHide, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveHanaSamboAttack, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveHanaSamboWait, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveHanaSamboAppear, TLiveActor) { return FALSE; }

TSamboFlowerSaveLoadParams::TSamboFlowerSaveLoadParams(const char* path)
    : TSpineEnemyParams(path)
    , PARAM_INIT(mSLLeafVelocityXZ, 4.0f)
    , PARAM_INIT(mSLLeafVelocityY, 6.0f)
    , PARAM_INIT(mSLLeafGravity, 0.2f)
    , PARAM_INIT(mSLBudDist, 2000.0f)
    , PARAM_INIT(mSLBloomTimer, 300)
    , PARAM_INIT(mSLCoinCircleR, 100.0f)
    , PARAM_INIT(mSLCoinVelocityXZ, 12.0f)
    , PARAM_INIT(mSLCoinVelocityY, 10.0f)
    , PARAM_INIT(mSLSeedShootRange, 500.0f)
    , PARAM_INIT(mSLSeedShootInterval, 200)
    , PARAM_INIT(mSLSeedGravity, 0.1f)
    , PARAM_INIT(mSLSeedSpeedXZ, 10.0f)
    , PARAM_INIT(mSLSeedSpeedY, 10.0f)
{
	TParams::load(mPrmPath);
}

void TSamboFlower::setMActorAndKeeper()
{
	mMActorKeeper = new TMActorKeeper(mManager, 1);
	mMActor       = mMActorKeeper->createMActor("flower.bmd", 3);
	mMActor->getModel()->getModelData()->setMaterialTable(
	    ((TSamboFlowerManager*)mManager)->mMaterialTable, J3DMatCopyFlag_All);
	mMActor->initDL();
	mMActor->getModel()->lock();
}

void TSamboFlower::init(TLiveManager* manager)
{
	mManager = manager;
	mManager->manageActor(this);
	setMActorAndKeeper();

	unk130  = 1;
	mParams = (TSamboFlowerSaveLoadParams*)getSaveParam();
	if (mParams) {
		mBodyRadius       = mParams->mSLBodyRadius.get();
		mWallRadius       = mParams->mSLWallRadius.get();
		mHeadHeight       = mParams->mSLHeadHeight.get();
		mScaledBodyRadius = mBodyScale * mBodyRadius;
	}

	initHitActor(0x10000027, 1, 0x80000000, mBodyRadius, mHeadHeight,
	             mBodyRadius, mHeadHeight);
	onHitFlag(HIT_FLAG_NO_COLLISION);
	offLiveFlag(LIVE_FLAG_UNK800);
	initAnmSound();
	mActorType = 0x10000027;
	unk150     = false;
	onLiveFlag(LIVE_FLAG_DEAD);
}

void TSamboFlower::loadAfter()
{
	JDrama::TNameRef::loadAfter();
	if (unk15C >= 0) {
		JGeometry::TVec3<f32> pos(0.0f, 0.0f, 0.0f);
		JGeometry::TVec3<f32> rot(0.0f, 0.0f, 0.0f);
		JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
		TMapObjBase* coin = TMapObjBaseManager::newAndRegisterObj(
		    "coin", pos, rot, scale);
		if (coin)
			unk168 = coin;
	}
}

void TSamboFlower::load(JSUMemoryInputStream& stream)
{
	TSpineEnemy::load(stream);
	stream.read(&unk15C, sizeof(unk15C));
	stream.read(&unk158, sizeof(unk158));
	unk160 = true;
	setMActorAndKeeper();
	offLiveFlag(LIVE_FLAG_UNK800);
}

void TSamboFlower::drawObject(JDrama::TGraphics*)
{
	if (checkLiveFlag(LIVE_FLAG_DEAD))
		return;

	TCircleShadowRequest request;
	request.unk0  = mPosition;
	request.unkC  = 120.0f;
	request.unk10 = 120.0f;
	gpBindShadowManager->request(request, getActorType() & 0xFFFF0000);

	mMActor->setLightData(mGroundPlane, mPosition);
	mMActor->entry();
}

void TSamboFlower::moveObject()
{
	if (unk150) {
		if (checkCurAnmEnd(0) && mMActor->checkCurAnm("flower_hit", 0))
			mMActor->setBck("flower_fwait");

		if (unk160) {
			++unk154;
			if (unk154 > mParams->mSLBloomTimer.get()) {
				if (mMActor->checkCurAnm("flower_fwait", 0)) {
					unk150 = false;
					if (unk164)
						++*unk164;

					mMActor->setBck("flower_hit");
					J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(0);
					s16 end            = ctrl->getEnd();
					ctrl               = mMActor->getFrameCtrl(0);
					ctrl->setFrame(end);
					mMActor->setFrameRate(-SMSGetAnmFrameRate(), 0);
				} else {
					J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(0);
					if (ctrl->getFrame() < 1.0f) {
						unk154 = 0;
						mMActor->setBck("flower_wait");
					}
				}
			}
		}
	}

	if (!checkLiveFlag(LIVE_FLAG_UNK10)) {
		mPosition.y = gpMap->checkGround(mPosition.x, mPosition.y + 100.0f,
		                                 mPosition.z, &mGroundPlane);
	}
}

void TSamboFlower::reset()
{
	TSpineEnemy::reset();
	mMActor->setBck("flower_wait");
	offHitFlag(HIT_FLAG_NO_COLLISION);
	offLiveFlag(LIVE_FLAG_UNK800);
	offLiveFlag(LIVE_FLAG_DEAD);
	offLiveFlag(LIVE_FLAG_UNK10);
}

BOOL TSamboFlower::receiveMessage(THitActor*, u32 message)
{
	if (message != HIT_MESSAGE_SPRAYED_BY_WATER)
		return FALSE;

	if (!unk150) {
		unk150 = true;
		unk154 = 0;
		gpMarioParticleManager->emit(0xB2, &mPosition, 0, nullptr);
		mMActor->setBck("flower_hit");

		if (unk160 && unk164) {
			--*unk164;
			u32 soundID = *unk164 + 0x89B9;
			if (gpMSound->gateCheck(soundID)) {
				MSoundSESystem::MSoundSE::startSoundActor(
				    soundID, &mPosition, 0, nullptr, 0, 4);
			}
		}
	}

	return TRUE;
}

void TSamboFlower::control() { }

void TSamboFlowerManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk38          = new TSamboFlowerSaveLoadParams("/enemy/samboflower.prm");
	mMaterialTable = J3DModelLoaderDataBase::loadMaterialTable(
	    JKRFileLoader::getGlbResource(
	        "/scene/samboflower/flower_orange.bmt"));
}

void TSamboFlowerManager::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (mCoinUnitCount > 0 && (flags & 2)) {
		for (int i = 0; i < mCoinUnitCount; ++i)
			mCoinUnits[i]->checkGenCoin();
	}

	TEnemyManager::perform(flags, graphics);

	if (mLeaves) {
		for (int i = 0; i < 18; ++i)
			mLeaves[i]->perform(flags, graphics);
	}
}

void TSamboFlowerManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "flower.bmd", 0x10220000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TSpineEnemy* TSamboFlowerManager::createEnemyInstance()
{
	return new TSamboFlower("サンボフラワー");
}

void TSamboLeaf::perform(u32 flags, JDrama::TGraphics*)
{
	if (!mActive)
		return;

	if (flags & 1) {
		mPosition.add(mVelocity);
		if (mVelocity.y > -20.0f)
			mVelocity.y
			    -= ((TSamboFlowerSaveLoadParams*)mManager->getSaveParam())
			           ->mSLLeafGravity.get();

		const TBGCheckData* ground;
		f32 groundY = gpMap->checkGround(mPosition.x, mPosition.y + 20.0f,
		                                 mPosition.z, &ground);
		if (mPosition.y < groundY)
			mActive = false;
	}
}

TSamboHeadManager::TSamboHeadManager(const char* name)
    : TSmallEnemyManager(name)
{
	gpCurSamboHead = nullptr;
}

void TSamboHeadManager::load(JSUMemoryInputStream& stream)
{
	TSmallEnemyManager::load(stream);
	unk38 = new TSamboHeadSaveLoadParams("/enemy/sambohead.prm");
}

void TSamboHeadManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "samboHead.bmd", 0x10220000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TSpineEnemy* TSamboHeadManager::createEnemyInstance()
{
	return new TSamboHead("サンボヘッド");
}

TSamboHead::TSamboHead(const char* name)
    : TWalkerEnemy(name)
    , mParams(nullptr)
    , unk198(nullptr)
    , unk19C(0)
    , mRollAngle(0.0f)
    , unk1B0(0)
{
}

void TSamboHead::load(JSUMemoryInputStream& stream)
{
	TSmallEnemy::load(stream);
	setMActorAndKeeper();
	initMarioGoal(this);
}

void TSamboHead::init(TLiveManager* manager)
{
	TWalkerEnemy::init(manager);
	mActorType = 0x1000001B;
	unk150     = 17;
	mParams    = (TSamboHeadSaveLoadParams*)getSaveParam();
	mSpine->initWith(&TNerveSamboHeadHide::theNerve());
	initMarioGoal(this);
	mMActor->setJointCallback(mBodyJntIndex, SamboHeadRollCallback);
}

void TSamboHead::calcRootMatrix()
{
	gpCurSamboHead = this;
	TSpineEnemy::calcRootMatrix();
}

void TSamboHead::setMActorAndKeeper()
{
	mMActorKeeper = new TMActorKeeper(mManager, 1);
	mMActor       = mMActorKeeper->createMActor("samboHead.bmd", 3);
}

const char** TSamboHead::getBasNameTable() const { return sambohead_bastable; }

void TSamboHead::reset()
{
	gpCurSamboHead = this;
	TWalkerEnemy::reset();
	unk165     = false;
	unk1B0     = 0;
	unk19C     = 0;
	mRollAngle = 0.0f;
	offLiveFlag(LIVE_FLAG_UNK8);
}

void TSamboHead::kill()
{
	mHitPoints = 1;
	if (mSpine->getCurrentNerve() != &TNerveSmallEnemyDie::theNerve()) {
		mSpine->reset();
		mSpine->setNext(&TNerveSmallEnemyDie::theNerve());
		mSpine->pushAfterCurrent(&TNerveSmallEnemyDie::theNerve());
	}
	onLiveFlag(LIVE_FLAG_UNK40);
}

void TSamboHead::setDeadAnm() { setBckAnm(3); }

f32 TSamboHead::getGravityY() const
{
	f32 gravity = mGravity;
	if (mSpine->getCurrentNerve() == &TNerveSamboHeadAttack::theNerve())
		gravity = mParams->mSLMoveGravity.get();
	if (mSpine->getCurrentNerve() == &TNerveSamboHeadHitWater::theNerve())
		gravity = mParams->mSLHitJumpGravity.get();
	return gravity;
}

THanaSamboManager::THanaSamboManager(const char* name)
    : TSmallEnemyManager(name)
{
}

void THanaSamboManager::load(JSUMemoryInputStream& stream)
{
	TSmallEnemyManager::load(stream);
	unk38 = new THanaSamboSaveLoadParams("/enemy/hanasambo.prm");
}

void THanaSamboManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "sambo.bmd", 0x10220000, 0 },
		{ "samboD.bmd", 0x10220000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TSpineEnemy* THanaSamboManager::createEnemyInstance()
{
	return new THanaSambo("ハナサンボ");
}

THanaSambo::THanaSambo(const char* name)
    : TSmallEnemy(name)
    , mHead(nullptr)
    , mParams(nullptr)
    , mFlower(nullptr)
    , mBindShadow(nullptr)
{
}

void THanaSambo::load(JSUMemoryInputStream& stream)
{
	TSmallEnemy::load(stream);
	setMActorAndKeeper();
	initMarioGoal(this);
}

void THanaSambo::init(TLiveManager* manager)
{
	TSmallEnemy::init(manager);
	mActorType = 0x1000001A;
	unk150     = 17;
	mParams    = (THanaSamboSaveLoadParams*)getSaveParam();
	mSpine->initWith(&TNerveHanaSamboHide::theNerve());

	mBindShadow = new TMBindShadowBody(this, getModel(), 1.5f);
	initMarioGoal(this);

	mHead = new THanaSamboHead("ハナサンボ頭あたり");
	JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ")->add(mHead);
	mHead->initHitActor(0x1000001B, 2, 0x80000000,
	                    mParams->mSLHeadAttackRadius.get() * mBodyScale,
	                    mParams->mSLHeadAttackHeight.get() * mBodyScale,
	                    mParams->mSLHeadDamageRadius.get() * mBodyScale,
	                    mParams->mSLHeadDamageHeight.get() * mBodyScale);
	mHead->mOwner = this;
}

void THanaSambo::setMActorAndKeeper()
{
	mMActorKeeper = new TMActorKeeper(mManager, 2);
	mMActor       = mMActorKeeper->createMActor("sambo.bmd", 3);
	mMActorKeeper->createMActor("samboD.bmd", 3);
}

void THanaSambo::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TSmallEnemy::perform(flags, graphics);
	if ((mLiveFlag & 7) == 0)
		mHead->perform(flags, graphics);
}

const char** THanaSambo::getBasNameTable() const { return sambo_bastable; }

void THanaSambo::reset()
{
	TSmallEnemy::reset();
	unk165        = false;
	mRootPosition = mPosition;
}

void THanaSambo::drawObject(JDrama::TGraphics* graphics)
{
	TLiveActor::drawObject(graphics);
	if ((mLiveFlag & 0xB) == 0)
		mBindShadow->entryDrawShadow();
}

void THanaSambo::moveObject()
{
	TSmallEnemy::moveObject();
	if (checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)) {
		mHead->mPosition = mPosition;
	} else {
		J3DModel* model = getModel();
		MtxPtr mtx      = model->getAnmMtx(mHeadJntIndex);
		mHead->mPosition.set(mtx[0][3], mtx[1][3], mtx[2][3]);
	}

	for (int i = 0; i < mHead->getColNum(); ++i) {
		THitActor* actor = mHead->getCollision(i);
		if (actor->isActorType(0x80000001))
			SMS_SendMessageToMario(mHead, HIT_MESSAGE_ATTACK);
	}

	mPosition.x = mRootPosition.x;
	mPosition.z = mRootPosition.z;
}

void THanaSambo::kill()
{
	if (checkLiveFlag(LIVE_FLAG_DEAD))
		return;

	mHitPoints = 1;
	if (isBckAnm(7))
		unk18C = 3;

	mSpine->setNext(&TNerveHanaSamboDie::theNerve());
	mHead->onHitFlag(HIT_FLAG_NO_COLLISION);
	onLiveFlag(LIVE_FLAG_UNK40);
}

void THanaSambo::setDeadAnm()
{
	mMActor = mMActorKeeper->getMActor("samboD.bmd");
	if (mUseYDownAnim)
		setBckAnm(10);
	else
		setBckAnm(0);

	mHead->onHitFlag(HIT_FLAG_NO_COLLISION);
	mUseYDownAnim = false;
	onLiveFlag(LIVE_FLAG_UNK8);
}

void THanaSambo::setWaitAnm()
{
	setBckAnm(7);
	mUseYDownAnim = false;
}

void THanaSambo::behaveToWater(THitActor*) { }

bool THanaSambo::isHitValid(u32 message)
{
	if (message == HIT_MESSAGE_UNKB)
		return true;

	return false;
}

bool THanaSambo::isCollidMove(THitActor*) { return false; }

void THanaSambo::createPollen() { }

BOOL THanaSamboHead::receiveMessage(THitActor*, u32 message)
{
	if (message <= HIT_MESSAGE_HIP_DROP) {
		mOwner->kill();
		return TRUE;
	}

	if (message == HIT_MESSAGE_SPRAYED_BY_WATER) {
		mOwner->unk165 = true;
		if (!mOwner->changeByJuice()) {
			if (mOwner->mSpine->getCurrentNerve()
			    == &TNerveHanaSamboWait::theNerve()) {
				mOwner->mSpine->setNext(&TNerveHanaSamboFreeze::theNerve());
			}
		}
		return TRUE;
	}

	return FALSE;
}
