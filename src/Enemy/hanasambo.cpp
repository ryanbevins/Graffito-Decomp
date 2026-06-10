#include <Enemy/HanaSambo.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/ShadowUtil.hpp>
#include <Player/MarioAccess.hpp>
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

static inline void initMarioGoal(THanaSambo* sambo)
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

DEFINE_NERVE(TNerveHanaSamboFreeze, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveHanaSamboDie, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveHanaSamboHide, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveHanaSamboAttack, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveHanaSamboWait, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveHanaSamboAppear, TLiveActor) { return FALSE; }

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
