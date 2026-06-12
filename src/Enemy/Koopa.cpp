#include <Enemy/Koopa.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DNode.hpp>
#include <JSystem/JDrama/JDRNameRef.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JGadget/std-list.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MSound/MAnmSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSSetSound.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/ObjManager.hpp>
#include <Strategic/Spine.hpp>
#include <System/Particles.hpp>

static const char dummyMactorStringValue1[] = "\0\0\0\0\0\0\0\0\0\0\0";
static const char SMS_NO_MEMORY_MESSAGE[]   = "メモリが足りません\n";
static const char MtxCalcTypeName0[]
    = "MActorMtxCalcType_Basic クラシックスケールＯＮ";
static const char MtxCalcTypeName1[]
    = "MActorMtxCalcType_Softimage クラシックスケールＯＦＦ";
static const char MtxCalcTypeName2[]
    = "MActorMtxCalcType_MotionBlend モーションブレンド";
static const char MtxCalcTypeName3[] = "MActorMtxCalcType_User ユーザー定義";
static const f32 dummy2850[3]        = { 0.0f, 0.0f, 0.0f };
static const f32 dummy2852[3]        = { 1.0f, 1.0f, 1.0f };

static const char* koopa_bastable[] = {
	"/scene/koopa/bas/koopa_down.bas",
	nullptr,
	"/scene/koopa/bas/koopa_fall.bas",
	"/scene/koopa/bas/koopa_fire_end.bas",
	"/scene/koopa/bas/koopa_fire_loop.bas",
	"/scene/koopa/bas/koopa_fire_start.bas",
	"/scene/koopa/bas/koopa_first.bas",
	"/scene/koopa/bas/koopa_getup.bas",
	"/scene/koopa/bas/koopa_hipdrop.bas",
	"/scene/koopa/bas/koopa_stagger.bas",
	"/scene/koopa/bas/koopa_turn_l.bas",
	"/scene/koopa/bas/koopa_turn_r.bas",
	"/scene/koopa/bas/koopa_wait.bas",
	nullptr,
	"/scene/koopa/bas/koopa_waterhit.bas",
};

namespace {
int KoopaNeckCallBack(J3DNode*, int) { return 1; }
} // namespace

inline TKoopaParams::TKoopaParams(const char* path)
    : TSpineEnemyParams(path)
    , PARAM_INIT(turnSpeed, 1.6f)
    , PARAM_INIT(turnAnim, 3.7f)
    , PARAM_INIT(waitStep, 600.0f)
    , PARAM_INIT(downStep, 1000.0f)
    , PARAM_INIT(attackRadius, 800.0f)
    , PARAM_INIT(attackHeight, 1000.0f)
    , PARAM_INIT(focusRange, 2.0f)
    , PARAM_INIT(waitRange, 12.0f)
    , PARAM_INIT(fireSpeed, 3.5f)
    , PARAM_INIT(tumbleWeight, 8.3f)
    , PARAM_INIT(tumbleSpeed, 2.0f)
    , PARAM_INIT(tumbleStartFrame, 95.0f)
    , PARAM_INIT(tumbleEndFrame, 160.0f)
    , PARAM_INIT(waitSpeed, 2.0f)
    , PARAM_INIT(staggerSpeed, 2.0f)
    , PARAM_INIT(downSpeed, 2.0f)
    , PARAM_INIT(flameVelocity, 35.0f)
    , PARAM_INIT(flameScale, 1.0f)
    , PARAM_INIT(flameCount, 500)
    , PARAM_INIT(flameFocusStartStep, 25)
    , PARAM_INIT(flameFocusEndStep, 500)
    , PARAM_INIT(flameRadius, 300.0f)
    , PARAM_INIT(flameHeight, 1000.0f)
    , PARAM_INIT(headRadius, 400.0f)
    , PARAM_INIT(waterhitSpeed, 2.0f)
    , PARAM_INIT(flameOverStart, 1.0f)
    , PARAM_INIT(flameNeckRange, 17.0f)
    , PARAM_INIT(flameNeckDownRate, 0.3f)
    , PARAM_INIT(flameJump, 80.0f)
    , PARAM_INIT(fallSpeed, 2.0f)
    , PARAM_INIT(marioEstimationFire, 20.0f)
    , PARAM_INIT(marioEstimationWait, 10.0f)
{
	TParams::load(mPrmPath);
}

TKoopaManager::TKoopaManager(const char* name)
    : TEnemyManager(name)
{
}

TSpineEnemy* TKoopaManager::createEnemyInstance() { return nullptr; }

void TKoopaManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "koopa_model.bmd", 0x14240000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TKoopaManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk38 = new TKoopaParams("/enemy/koopa.prm");
}

void TKoopaManager::loadAfter()
{
	JDrama::TNameRef::loadAfter();
	SMS_LoadParticle("/scene/koopa/jpa/ms_kp_fire_a.jpa", 0x1c0);
	SMS_LoadParticle("/scene/koopa/jpa/ms_kp_fire_b.jpa", 0x1c1);
	SMS_LoadParticle("/scene/koopa/jpa/ms_kp_fire_c.jpa", 0x1c2);
	SMS_LoadParticle("/scene/koopa/jpa/ms_kp_fire_d.jpa", 0x1c3);
	SMS_LoadParticle("/scene/koopa/jpa/ms_kp_hipdrop.jpa", 0xf5);
	SMS_LoadParticle("/scene/koopa/jpa/ms_kp_fire_e.jpa", 0x1f3);
}

TKoopaParts::TKoopaParts(const char* name, u32 actorType, TKoopa* owner,
                         f32 radius)
    : THitActor(name)
    , mOwner(owner)
{
	JDrama::TNameRef* root
	    = JDrama::TNameRefGen::getInstance()->getRootNameRef();
	const char* groupName = "敵グループ";
	JDrama::TNameRef* group
	    = root->searchF(JDrama::TNameRef::calcKeyCode(groupName), groupName);
	JGadget::TList_pointer_void* list
	    = (JGadget::TList_pointer_void*)((u8*)group + 0x10);
	void* self = this;
	list->insert(list->end(), self);

	initHitActor(actorType, 5, 0x88000000, radius, radius, radius, radius);
	onHitFlag(0x2);
	onHitFlag(0x4);
	onHitFlag(0x1);
	unk64 |= 0x10000000;
	unk64 |= 0x08000000;
}

void TKoopaParts::perform(u32 flags, JDrama::TGraphics* graphics)
{
	THitActor::perform(flags, graphics);

	if (flags & 1) {
		for (int i = 0; i < mColCount; ++i)
			attack_(mCollisions[i]);
	}
}

void TKoopaParts::control() { }

void TKoopaBody::attack_(THitActor* actor) { actor->receiveMessage(this, 0xE); }

BOOL TKoopaBody::receiveMessage(THitActor* sender, u32 message)
{
	return TRUE;
}

void TKoopaHead::attack_(THitActor* actor) { actor->receiveMessage(this, 0xE); }

BOOL TKoopaHead::receiveMessage(THitActor* sender, u32 message)
{
	if (message == 0xF) {
		if (mOwner->mSpine->getCurrentNerve() == &TNerveKoopaTumble::theNerve())
			return TRUE;
		if (mOwner->mSpine->getCurrentNerve() == &TNerveKoopaGetDown::theNerve())
			return TRUE;
		if (mOwner->mSpine->getCurrentNerve()
		    == &TNerveKoopaStagger::theNerve())
			mOwner->mSpine->setNext(&TNerveKoopaGetShowered::theNerve());

		mOwner->mSpine->pushNerve(&TNerveKoopaGetShowered::theNerve());
	}

	return TRUE;
}

void TKoopaHand::attack_(THitActor* actor) { actor->receiveMessage(this, 0xE); }

BOOL TKoopaHand::receiveMessage(THitActor* sender, u32 message) { return TRUE; }

void TKoopaFlame::attack_(THitActor* actor)
{
	if (actor->receiveMessage(this, 0xA)) {
		MActor* mactor = mOwner->getMActor();
		if (!mactor->checkCurBckFromIndex(3))
			mactor->setBckFromIndex(3);
		mOwner->getMActor()->getFrameCtrl(0)->setRate(
		    mOwner->getSaveParam2()->fireSpeed.get());
	}
}

BOOL TKoopaFlame::receiveMessage(THitActor* sender, u32 message)
{
	BOOL result;
	if ((s32)message == 0xF)
		result = FALSE;
	else
		result = TRUE;
	return result;
}

void TKoopaFlame::control() { }

TKoopa::TKoopa(const char* name)
    : TSpineEnemy(name)
{
	mLiveFlag |= 0x80;
	mLiveFlag &= ~0x100;
	mLiveFlag |= 0x10;
}

void TKoopa::load(JSUMemoryInputStream& stream) { TSpineEnemy::load(stream); }

BOOL TKoopa::receiveMessage(THitActor* sender, u32 message)
{
	return TSpineEnemy::receiveMessage(sender, message);
}

const char** TKoopa::getBasNameTable() const { return koopa_bastable; }

void TKoopa::reset()
{
	TSpineEnemy::reset();

	f32 waitSpeed = getSaveParam2()->waitSpeed.get();
	if (!mMActor->checkCurBckFromIndex(12)) {
		mMActor->setBckFromIndex(12);
		const char** bas = getBasNameTable();
		const char* sound;
		if (bas == nullptr)
			sound = nullptr;
		else
			sound = bas[12];
		setAnmSound(sound);
	}

	if (mMActor->getCurAnmIdx(3) != 1)
		mMActor->setBtpFromIndex(1);

	J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(0);
	ctrl->setRate(waitSpeed * SMSGetAnmFrameRate() * 0.5f);
	unk19C = 600;
}

void TKoopa::loadAfter()
{
	JDrama::TNameRef::loadAfter();

	for (int i = 0; i < 10; ++i) {
		mFlameHitActors[i] = new TKoopaFlame("クッパの吐く火", 0x08000029,
		                                     this, 100.0f);
	}
	for (int i = 0; i < 2; ++i)
		mHandHitActors[i] = new TKoopaHand("クッパの手", 0x0800002B, this,
		                                   100.0f);

	mHeadHitActor = new TKoopaHead("クッパの頭", 0x0800002A, this, 100.0f);
	mBodyHitActor = new TKoopaBody("クッパの体", 0x0800002A, this, 100.0f);
}

void TKoopa::init(TLiveManager* manager)
{
	mBodyRadius = getSaveParam2()->attackRadius.get();
	mHeadHeight = 2000.0f;

	TSpineEnemy::init(manager);
	onHitFlag(0x1);
	onHitFlag(0x4);
	offHitFlag(0x2);
	mSpine->initWith(&TNerveKoopaProvoke::theNerve());

	if (!mMActor->checkCurBckFromIndex(12)) {
		mMActor->setBckFromIndex(12);
		const char** bas = getBasNameTable();
		setAnmSound(bas ? bas[12] : nullptr);
	}

	if (mMActor->getCurAnmIdx(3) != 1)
		mMActor->setBtpFromIndex(1);

	mMActor->getFrameCtrl(0)->setRate(0.5f * 2.0f * SMSGetAnmFrameRate());

	if (mMActor->getAnmBck())
		mMActor->getAnmBck()->initSimpleMotionBlend(0x10);

	initAnmSound();
	loadAfter();

	J3DModelData* modelData = getModel()->getModelData();
	JUTNameTab* nameTab     = modelData->getJointName();
	mHeadJointIndex         = nameTab->getIndex("ago");
	mNeckJointIndex         = nameTab->getIndex("head");
	mJointIndex2            = nameTab->getIndex("neck");

	J3DNode* node = (J3DNode*)getModel()->getModelData()->getJointNodePointer(
	    mNeckJointIndex);
	node->setCallBack(&KoopaNeckCallBack);
	node->setCallBackUserData(this);

	unk1B8 = 1.0f;
	unk155 = 0;
}

void TKoopa::calcRootMatrix()
{
	mRotation.y = getFlameDirDegree();
	TSpineEnemy::calcRootMatrix();
}

void TKoopa::updateAnmSound()
{
	if (mMActor->getCurAnmIdx(0) == 8) {
		unk158.set(mPosition);
	} else {
		MtxPtr mtx = getModel()->getAnmMtx(mNeckJointIndex);
		unk158.x   = mtx[0][3];
		unk158.y   = mtx[1][3];
		unk158.z   = mtx[2][3];
	}

	if (mAnmSound && mAnmSoundPath) {
		J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(0);
		mAnmSound->animeLoop(&unk158, ctrl->getFrame(), ctrl->getRate(), 0, 4);
	}
}

void TKoopa::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TSpineEnemy::perform(flags, graphics);

	for (int i = 0; i < 10; ++i)
		mFlameHitActors[i]->perform(flags, graphics);
	for (int i = 0; i < 2; ++i)
		mHandHitActors[i]->perform(flags, graphics);
	mHeadHitActor->perform(flags, graphics);
	mBodyHitActor->perform(flags, graphics);

	if (flags & 1) {
		setUpHitActors();
		if (unk19C > 0)
			--unk19C;
	}
}

void TKoopa::fall()
{
	mRotation.y = 180.0f;
	mSpine->setNext(&TNerveKoopaFall::theNerve());
}

f32 TKoopa::getTargetDir(const JGeometry::TVec3<f32>& pos) const
{
	JGeometry::TVec3<f32> diff = pos;
	diff.sub(mPosition);
	return matan(diff.x, diff.z) * (360.0f / 65536.0f);
}

void TKoopa::stagger(bool down)
{
	if (down)
		mSpine->pushNerve(&TNerveKoopaGetDown::theNerve());
	else
		mSpine->pushNerve(&TNerveKoopaStagger::theNerve());
}

void TKoopa::getShowered() { mSpine->pushNerve(&TNerveKoopaGetShowered::theNerve()); }

BOOL TKoopa::effectsTumble() const
{
	if (mSpine->getCurrentNerve() == &TNerveKoopaGetDown::theNerve()) {
		int time = mSpine->getTime();
		if (time < 900 && time > 190)
			return TRUE;
	}
	return FALSE;
}

void TKoopa::getDown() { mSpine->pushNerve(&TNerveKoopaGetDown::theNerve()); }

BOOL TKoopa::allowsLaunch() const
{
	if (mSpine->getCurrentNerve() == &TNerveKoopaGetDown::theNerve())
		return FALSE;
	return TRUE;
}

JGeometry::TVec3<f32> TKoopa::getNeckFocus() const
{
	MtxPtr mtx = getMActor()->getModel()->getAnmMtx(mNeckJointIndex);
	return JGeometry::TVec3<f32>(mtx[0][3], mtx[1][3], mtx[2][3]);
}

BOOL TKoopa::isFlaming() const
{
	int idx = mMActor->getCurAnmIdx(0);
	if (idx < 6) {
		if (idx >= 3)
			return TRUE;
	}
	return FALSE;
}

f32 TKoopa::getFlameDirRate() const { return unk150; }

f32 TKoopa::getFlameDirDegree() const { return unk150 * 360.0f + -180.0f; }

void TKoopa::changeAnm(int bck, int btp, f32 rate)
{
	if (!mMActor->checkCurBckFromIndex(bck)) {
		mMActor->setBckFromIndex(bck);
		const char** bas = getBasNameTable();
		const char* sound;
		if (bas == nullptr)
			sound = nullptr;
		else
			sound = bas[bck];
		setAnmSound(sound);
	}
	if (mMActor->getCurAnmIdx(3) != btp)
		mMActor->setBtpFromIndex(btp);
	J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(0);
	ctrl->setRate(rate * SMSGetAnmFrameRate() * 0.5f);
}

void TKoopa::setUpHitActors()
{
	TKoopaParams* prm = getSaveParam2();
	for (int i = 0; i < 10; ++i) {
		TKoopaFlame* hit = mFlameHitActors[i];
		hit->mAttackRadius = prm->flameRadius.get();
		hit->mAttackHeight = prm->flameHeight.get();
		hit->mDamageRadius = prm->flameRadius.get();
		hit->mDamageHeight = prm->flameHeight.get();
		hit->calcEntryRadius();
	}

	mHeadHitActor->mDamageRadius = prm->headRadius.get();
	mHeadHitActor->mDamageHeight = prm->headRadius.get() * 2.0f;
	mHeadHitActor->calcEntryRadius();
}

inline const TNerveKoopaWait& TNerveKoopaWait::theNerve()
{
	static TNerveKoopaWait instance;
	return instance;
}

inline const TNerveKoopaTumble& TNerveKoopaTumble::theNerve()
{
	static TNerveKoopaTumble instance;
	return instance;
}

inline const TNerveKoopaFlame& TNerveKoopaFlame::theNerve()
{
	static TNerveKoopaFlame instance;
	return instance;
}

inline const TNerveKoopaTurnL& TNerveKoopaTurnL::theNerve()
{
	static TNerveKoopaTurnL instance;
	return instance;
}

inline const TNerveKoopaTurnR& TNerveKoopaTurnR::theNerve()
{
	static TNerveKoopaTurnR instance;
	return instance;
}

BOOL TNerveKoopaWait::execute(TSpineBase<TLiveActor>* spine) const
{
	TKoopa* self = (TKoopa*)spine->getBody();
	if (spine->getTime() == 0)
		self->changeAnm(12, 1, self->getSaveParam2()->waitSpeed.get());
	return FALSE;
}

BOOL TNerveKoopaTumble::execute(TSpineBase<TLiveActor>* spine) const
{
	TKoopa* self = (TKoopa*)spine->getBody();
	self->changeAnm(8, 1, self->getSaveParam2()->tumbleSpeed.get());
	if (self->mMActor->curAnmEndsNext(0, nullptr))
		return TRUE;
	return FALSE;
}

BOOL TNerveKoopaFlame::execute(TSpineBase<TLiveActor>* spine) const
{
	TKoopa* self = (TKoopa*)spine->getBody();
	if (spine->getTime() == 0)
		self->changeAnm(5, 1, self->getSaveParam2()->fireSpeed.get());
	return FALSE;
}

BOOL TNerveKoopaTurnL::execute(TSpineBase<TLiveActor>* spine) const
{
	TKoopa* self = (TKoopa*)spine->getBody();
	self->changeAnm(10, 1, self->getSaveParam2()->turnAnim.get());
	return self->mMActor->curAnmEndsNext(0, nullptr) ? TRUE : FALSE;
}

BOOL TNerveKoopaTurnR::execute(TSpineBase<TLiveActor>* spine) const
{
	TKoopa* self = (TKoopa*)spine->getBody();
	self->changeAnm(11, 1, self->getSaveParam2()->turnAnim.get());
	return self->mMActor->curAnmEndsNext(0, nullptr) ? TRUE : FALSE;
}

DEFINE_NERVE(TNerveKoopaGetDown, TLiveActor)
{
	TKoopa* self = (TKoopa*)spine->getBody();
	self->changeAnm(0, 1, self->getSaveParam2()->downSpeed.get());
	return self->mMActor->curAnmEndsNext(0, nullptr) ? TRUE : FALSE;
}

DEFINE_NERVE(TNerveKoopaGetShowered, TLiveActor)
{
	TKoopa* self = (TKoopa*)spine->getBody();
	self->changeAnm(14, 1, self->getSaveParam2()->waterhitSpeed.get());
	return self->mMActor->curAnmEndsNext(0, nullptr) ? TRUE : FALSE;
}

DEFINE_NERVE(TNerveKoopaStagger, TLiveActor)
{
	TKoopa* self = (TKoopa*)spine->getBody();
	self->changeAnm(9, 1, self->getSaveParam2()->staggerSpeed.get());
	return self->mMActor->curAnmEndsNext(0, nullptr) ? TRUE : FALSE;
}

DEFINE_NERVE(TNerveKoopaProvoke, TLiveActor)
{
	TKoopa* self = (TKoopa*)spine->getBody();
	if (spine->getTime() == 0)
		self->changeAnm(6, 0, 2.0f);
	return self->mMActor->curAnmEndsNext(0, nullptr) ? TRUE : FALSE;
}

DEFINE_NERVE(TNerveKoopaFall, TLiveActor)
{
	TKoopa* self = (TKoopa*)spine->getBody();
	self->changeAnm(2, 1, self->getSaveParam2()->fallSpeed.get());
	return self->mMActor->curAnmEndsNext(0, nullptr) ? TRUE : FALSE;
}
