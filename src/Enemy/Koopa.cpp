#include <Enemy/Koopa.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DNode.hpp>
#include <JSystem/JDrama/JDRNameRef.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JGadget/std-list.hpp>
#include <JSystem/JGeometry/JGUtil.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MSound/MAnmSound.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSSetSound.hpp>
#include <MoveBG/MapObjCorona.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/ObjManager.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Strategy.hpp>
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
	JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ")->add(this);

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
		control();
		for (int i = 0; i < mColCount; ++i)
			attack_(mCollisions[i]);
	}
}

void TKoopaParts::control() { }

void TKoopaBody::attack_(THitActor* actor)
{
	if (actor->receiveMessage(this, 0xE)) {
		if (actor == SMS_GetMarioLiveActor()) {
			JGeometry::TVec3<f32> throwVec(0.0f, 1.0f, 0.0f);
			SMS_ThrowMario(throwVec, 60.0f);
		}
	}
}

BOOL TKoopaBody::receiveMessage(THitActor* sender, u32 message)
{
	if (message == HIT_MESSAGE_ATTACK && sender->mActorType == 0x08000024)
		mOwner->stagger(false);
	return TRUE;
}

void TKoopaHead::attack_(THitActor* actor)
{
	if (actor->receiveMessage(this, 0xE)) {
		if (actor == SMS_GetMarioLiveActor()) {
			JGeometry::TVec3<f32> throwVec(0.0f, 1.0f, 0.0f);
			SMS_ThrowMario(throwVec, 60.0f);
		}
	}
}

BOOL TKoopaHead::receiveMessage(THitActor* sender, u32 message)
{
	switch ((s32)message) {
	case HIT_MESSAGE_SPRAYED_BY_WATER:
		if (mOwner->getShowered()) {
			gpMarioParticleManager->emit(0xe7, &sender->mPosition, 0, nullptr);
			gpMSound->startSoundSet(0x6802, &mOwner->mPosition, 0, 0.0f,
			                         0, 0, 4);
		}
		break;
	case HIT_MESSAGE_ATTACK:
		if (sender->mActorType == 0x08000024)
			mOwner->stagger(false);
		break;
	}

	return TRUE;
}

void TKoopaHand::attack_(THitActor* actor) { actor->receiveMessage(this, 0xE); }

BOOL TKoopaHand::receiveMessage(THitActor* sender, u32 message) { return TRUE; }

void TKoopaFlame::attack_(THitActor* actor)
{
	if (actor->receiveMessage(this, 0xA)) {
		if (actor == (THitActor*)gpMarioAddress) {
			f32 jump = mOwner->getSaveParam2()->flameJump.get();
			JGeometry::TVec3<f32> throwVec(0.0f, 1.0f, 0.0f);
			SMS_ThrowMario(throwVec, jump);
			mOwner->unk155 = 1;
			mOwner->changeAnm(3, 0, mOwner->getSaveParam2()->fireSpeed.get());
			mOwner->unk19C = 240;
		}
	}
}

BOOL TKoopaFlame::receiveMessage(THitActor* sender, u32 message)
{
	switch ((s32)message) {
	case 0xF:
		return FALSE;
	}
	return TRUE;
}

void TKoopaFlame::control()
{
	if (!(unk8C < unk88)) {
		onHitFlag(0x2);
		onHitFlag(0x4);
		onHitFlag(0x1);
	} else {
		unk8C += unk84;

		f32 time   = unk8C;
		f32 height = unk94;
		f32 x      = unk6C + unk78 * time;
		f32 y      = unk70 + unk7C * time;
		f32 z      = unk74 + unk80 * time;
		f32 radius = unk90;
		if (height <= 0.0f)
			height = 2.0f * radius;

		mPosition.x = x;
		mPosition.y = y;
		mPosition.z = z;
		offHitFlag(0x2);
		offHitFlag(0x4);
		offHitFlag(0x1);
		mAttackRadius = radius;
		mAttackHeight = height;
		mDamageRadius = radius;
		mDamageHeight = height;
		calcEntryRadius();
	}
}

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
		MtxPtr mtx = mMActor->getModel()->getAnmMtx(mNeckJointIndex);
		f32 z      = mtx[2][3];
		f32 y      = mtx[1][3];
		f32 x      = mtx[0][3];
		unk158.x   = x;
		unk158.y   = y;
		unk158.z   = z;
	}

	if (mAnmSound && mAnmSoundPath) {
		J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(0);
		mAnmSound->animeLoop(&unk158, ctrl->getFrame(), ctrl->getRate(), 0, 4);
	}
}

void TKoopa::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 1) {
		unk1B8 = getNeckFocus();
		if (unk19C > 0)
			--unk19C;
	}

	TSpineEnemy::perform(flags, graphics);

	for (int i = 0; i < 10; ++i)
		mFlameHitActors[i]->perform(flags, graphics);

	mHeadHitActor->perform(flags, graphics);
	for (int i = 0; i < 2; ++i)
		mHandHitActors[i]->perform(flags, graphics);
	mBodyHitActor->perform(flags, graphics);

	if (flags & 1) {
		TKoopaParams* prm  = getSaveParam2();
		J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(0);
		f32 frame          = ctrl->getFrame();
		BOOL shouldTumble  = FALSE;
		if (mSpine->getCurrentNerve() == &TNerveKoopaTumble::theNerve()
		    && frame >= prm->tumbleStartFrame.get()) {
			if (frame <= prm->tumbleEndFrame.get())
				shouldTumble = TRUE;
		}

		if (shouldTumble) {
			TBathtub* bathtub = JDrama::TNameRefGen::search<TBathtub>("バスタブ");
			bathtub->tumble(mRotation.y, prm->tumbleWeight.get());
		}

		setUpHitActors();
	}

	if (flags & 2) {
		BOOL emitFlame = FALSE;
		int idx        = mMActor->getCurAnmIdx(0);
		if (idx == 4) {
			emitFlame = TRUE;
		} else if (idx == 5) {
			if (mMActor->getFrameCtrl(0)->getFrame() >= 85.0f)
				emitFlame = TRUE;
		} else if (idx == 6) {
			f32 frame = mMActor->getFrameCtrl(0)->getFrame();
			if (frame >= 68.0f && frame <= 164.0f)
				emitFlame = TRUE;
		}

		if (emitFlame) {
			mMActor->calc();

			f32 flameScale = getSaveParam2()->flameScale.get();
			JGeometry::TVec3<f32> scale;
			scale.x = flameScale;
			scale.y = flameScale;
			scale.z = flameScale;

			MtxPtr mtx = mMActor->getModel()->getAnmMtx(mNeckJointIndex);
			JPABaseEmitter* emitter
			    = gpMarioParticleManager->emitAndBindToMtxPtr(0x1F3, mtx, 3,
			                                                  this);
			if (emitter) {
				emitter->unk154.x = scale.x;
				emitter->unk154.y = scale.y;
				emitter->unk154.z = scale.z;
				emitter->unk174.x = scale.x;
				emitter->unk174.y = scale.y;
				emitter->unk174.z = scale.z;
			}

			emitter = gpMarioParticleManager->emitAndBindToMtxPtr(0x1C3, mtx, 1,
			                                                      this);
			if (emitter) {
				emitter->unk154.set(scale);
				emitter->unk174.set(scale);
			}

			emitter = gpMarioParticleManager->emitAndBindToMtxPtr(0x1C2, mtx, 1,
			                                                      this);
			if (emitter) {
				emitter->unk154.set(scale);
				emitter->unk174.set(scale);
			}

			emitter = gpMarioParticleManager->emitAndBindToMtxPtr(0x1C1, mtx, 1,
			                                                      this);
			if (emitter) {
				emitter->unk154.set(scale);
				emitter->unk174.set(scale);
			}

			emitter = gpMarioParticleManager->emitAndBindToMtxPtr(0x1C0, mtx, 1,
			                                                      this);
			if (emitter) {
				emitter->unk154.set(scale);
				emitter->unk174.set(scale);
			}
		}
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

void TKoopa::stagger(bool ignoreFlame)
{
	if (mSpine->getCurrentNerve() == &TNerveKoopaFall::theNerve())
		return;
	if (mSpine->getCurrentNerve() == &TNerveKoopaProvoke::theNerve())
		return;
	if (!ignoreFlame
	    && mSpine->getCurrentNerve() == &TNerveKoopaFlame::theNerve())
		return;
	if (mSpine->getCurrentNerve() == &TNerveKoopaTumble::theNerve())
		return;
	if (mSpine->getCurrentNerve() == &TNerveKoopaGetDown::theNerve())
		return;
	if (mSpine->getCurrentNerve() == &TNerveKoopaGetShowered::theNerve())
		return;
	if (mSpine->getCurrentNerve() == &TNerveKoopaStagger::theNerve())
		return;

	mSpine->pushNerve(&TNerveKoopaStagger::theNerve());
}

BOOL TKoopa::getShowered()
{
	if (mSpine->getCurrentNerve() == &TNerveKoopaFall::theNerve())
		return FALSE;
	if (mSpine->getCurrentNerve() == &TNerveKoopaProvoke::theNerve())
		return FALSE;
	if (mSpine->getCurrentNerve() == &TNerveKoopaTumble::theNerve())
		return FALSE;
	if (mSpine->getCurrentNerve() == &TNerveKoopaGetDown::theNerve())
		return FALSE;
	if (mSpine->getCurrentNerve() == &TNerveKoopaGetShowered::theNerve())
		return TRUE;

	if (mSpine->getCurrentNerve() == &TNerveKoopaStagger::theNerve()) {
		mSpine->setNext(&TNerveKoopaGetShowered::theNerve());
		return TRUE;
	}

	if (mSpine->getCurrentNerve() == &TNerveKoopaFlame::theNerve()) {
		mSpine->setNext(&TNerveKoopaWait::theNerve());
		return FALSE;
	}

	mSpine->pushNerve(&TNerveKoopaGetShowered::theNerve());
	return TRUE;
}

BOOL TKoopa::effectsTumble() const
{
	if (mSpine->getCurrentNerve() == &TNerveKoopaGetDown::theNerve()) {
		int time = mSpine->getTime();
		if (time < 900 && time > 190)
			return TRUE;
	}
	return FALSE;
}

void TKoopa::getDown()
{
	if (mSpine->getCurrentNerve() == &TNerveKoopaFall::theNerve())
		return;
	if (mSpine->getCurrentNerve() == &TNerveKoopaProvoke::theNerve())
		return;
	if (mSpine->getCurrentNerve() == &TNerveKoopaTumble::theNerve())
		return;

	if (mSpine->getCurrentNerve() == &TNerveKoopaStagger::theNerve())
		mSpine->setNext(&TNerveKoopaGetDown::theNerve());
	if (mSpine->getCurrentNerve() == &TNerveKoopaGetShowered::theNerve())
		mSpine->setNext(&TNerveKoopaGetDown::theNerve());

	mSpine->pushNerve(&TNerveKoopaGetDown::theNerve());
}

BOOL TKoopa::allowsLaunch() const
{
	if (mSpine->getCurrentNerve() == &TNerveKoopaGetDown::theNerve())
		return FALSE;
	return TRUE;
}

f32 TKoopa::getNeckFocus() const
{
	int idx            = mMActor->getCurAnmIdx(0);
	J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(0);
	f32 end            = ctrl->getEnd();
	f32 frame          = ctrl->getFrame();

	switch (idx) {
	case 0:
		if (frame <= 40.0f)
			return 1.0f - frame / 40.0f;
		return 0.0f;

	case 1:
	case 2:
	case 4:
		return 0.0f;

	case 3:
		return frame / end;

	case 5:
		if (frame <= 103.0f)
			return 1.0f - frame / 103.0f;
		return 0.0f;

	case 6:
		if (frame >= 164.0f)
			return (frame - 164.0f) / (end - 164.0f);
		return 0.0f;

	case 7:
		if (frame <= 125.0f)
			return 0.0f;
		return (frame - 125.0f) / (end - 125.0f);

	case 8:
		if (frame <= 30.0f)
			return 1.0f - frame / 30.0f;
		if (frame <= 170.0f)
			return 0.0f;
		return (frame - 170.0f) / (end - 170.0f);

	case 9:
		if (frame <= 30.0f)
			return 1.0f - frame / 30.0f;
		if (frame <= 65.0f)
			return 0.0f;
		return (frame - 65.0f) / (end - 65.0f);

	case 12:
		if (frame <= 200.0f)
			return 1.0f;
		if (frame <= 255.0f)
			return 1.0f - (frame - 200.0f) / 55.0f;
		if (frame <= 330.0f)
			return 0.0f;
		if (frame <= 390.0f)
			return (frame - 330.0f) / 60.0f;
		if (frame <= 440.0f)
			return 1.0f;
		if (frame <= 480.0f)
			return 1.0f - (frame - 440.0f) / 40.0f;
		if (frame <= 555.0f)
			return 0.0f;
		if (frame <= 615.0f)
			return (frame - 555.0f) / 60.0f;
		return 1.0f;

	case 14:
		if (frame <= 20.0f)
			return 1.0f - frame / 20.0f;
		if (frame <= 40.0f)
			return 0.0f;
		return (frame - 40.0f) / (end - 40.0f);
	}

	return 1.0f;
}

BOOL TKoopa::isFlaming() const
{
	int idx = mMActor->getCurAnmIdx(0);
	switch (idx) {
	case 3:
	case 4:
	case 5:
		return TRUE;
	}
	return FALSE;
}

f32 TKoopa::getFlameDirRate() const
{
	J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(0);
	f32 frame          = ctrl->getFrame();
	f32 end            = mMActor->getFrameCtrl(0)->getEnd();
	TKoopaParams* prm  = getSaveParam2();
	f32 overStart      = prm->flameOverStart.get();
	s32 focusStart     = prm->flameFocusStartStep.get();
	s32 focusEnd       = prm->flameFocusEndStep.get();
	s32 time           = mSpine->getTime();
	int idx            = mMActor->getCurAnmIdx(0);

	switch (idx) {
	case 5:
		return -((frame * overStart) / end);

	case 3:
	case 4: {
		f32 rate;
		if (time <= focusStart) {
			rate = -overStart;
		} else if (time <= focusEnd) {
			rate = (overStart * (time - focusStart)) / (focusEnd - focusStart)
			       - overStart;
		} else {
			rate = 0.0f;
		}

		if (idx == 3)
			rate *= 1.0f - frame / end;

		return rate;
	}
	}

	return 1.0f;
}

f32 TKoopa::getFlameDirDegree() const
{
	f32 degree = getFlameDirRate() * getSaveParam2()->flameNeckRange.get();
	if (unk154)
		degree = -degree;

	return mRotation.y + degree;
}

#pragma dont_inline on
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
#pragma dont_inline off

void TKoopa::setUpHitActors()
{
	TKoopaParams* prm = getSaveParam2();
	BOOL canEmitFlame = FALSE;
	if (mMActor->getCurAnmIdx(0) == 4) {
		canEmitFlame = TRUE;
	} else if (mMActor->getCurAnmIdx(0) == 5) {
		if (mMActor->getFrameCtrl(0)->getFrame() >= 85.0f)
			canEmitFlame = TRUE;
	}

	if (canEmitFlame) {
		int available = -1;
		BOOL waiting  = FALSE;
		for (int i = 0; i < 10; ++i) {
			TKoopaFlame* flame = mFlameHitActors[i];
			if (!(flame->unk8C < flame->unk88)) {
				available = i;
			} else if (flame->unk8C < 2.0f * prm->flameRadius.get()) {
				waiting = TRUE;
			}
		}

		if (!waiting && available >= 0) {
			MtxPtr mtx = mMActor->getModel()->getAnmMtx(mNeckJointIndex);
			f32 axisX  = mtx[0][0];
			f32 axisZ  = mtx[2][0];
			f32 dirX;
			f32 dirY;
			f32 dirZ;
			f32 mag = axisX * axisX + axisZ * axisZ;
			if (mag <= 0.0000038146973f) {
				dirY = 0.0f;
				dirX = dirY;
				dirZ = dirY;
			} else {
				f32 inv = 1.0f * JGeometry::TUtil<f32>::inv_sqrt(mag);
				dirX    = axisX * inv;
				dirY    = 0.0f * inv;
				dirZ    = axisZ * inv;
			}

			TKoopaFlame* flame = mFlameHitActors[available];
			flame->mPosition.x = mtx[0][3];
			flame->mPosition.y = mtx[1][3] - 500.0f;
			flame->mPosition.z = mtx[2][3];
			flame->unk78       = dirX;
			flame->unk7C       = dirY;
			flame->unk80       = dirZ;
			flame->unk6C       = flame->mPosition.x;
			flame->unk70       = flame->mPosition.y;
			flame->unk74       = flame->mPosition.z;
			flame->unk84       = prm->flameVelocity.get();
			flame->unk88       = 4000.0f;
			flame->unk8C       = 0.0f;
			flame->unk90       = prm->flameRadius.get();
			flame->unk94       = prm->flameHeight.get();
		}
	} else {
		for (int i = 0; i < 10; ++i) {
			mFlameHitActors[i]->unk88 = 0.0f;
			mFlameHitActors[i]->unk8C = 1.0f;
		}
	}

	MtxPtr headMtx = mMActor->getModel()->getAnmMtx(mHeadJointIndex);
	mHeadHitActor->mPosition.x = headMtx[0][3];
	mHeadHitActor->mPosition.y = headMtx[1][3] - 200.0f;
	mHeadHitActor->mPosition.z = headMtx[2][3];
	mHeadHitActor->offHitFlag(0x2);
	mHeadHitActor->offHitFlag(0x4);
	mHeadHitActor->offHitFlag(0x1);
	mHeadHitActor->mAttackRadius = prm->headRadius.get();
	mHeadHitActor->mAttackHeight = prm->headRadius.get() * 2.0f;
	mHeadHitActor->mDamageRadius = prm->headRadius.get();
	mHeadHitActor->mDamageHeight = prm->headRadius.get() * 2.0f;
	mHeadHitActor->calcEntryRadius();

	mBodyHitActor->mPosition.x = mPosition.x;
	mBodyHitActor->mPosition.y = mPosition.y;
	mBodyHitActor->mPosition.z = mPosition.z;
	mBodyHitActor->offHitFlag(0x2);
	mBodyHitActor->offHitFlag(0x4);
	mBodyHitActor->offHitFlag(0x1);
	mBodyHitActor->mAttackRadius = 800.0f;
	mBodyHitActor->mAttackHeight = 2000.0f;
	mBodyHitActor->mDamageRadius = 800.0f;
	mBodyHitActor->mDamageHeight = 2000.0f;
	mBodyHitActor->calcEntryRadius();
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
	TKoopaParams* prm = self->getSaveParam2();

	switch (self->mMActor->getCurAnmIdx(0)) {
	case 0:
		if (self->mMActor->curAnmEndsNext(0, nullptr))
			self->changeAnm(1, 0, prm->downSpeed.get());
		break;

	case 1: {
		TBathtub* bathtub = JDrama::TNameRefGen::search<TBathtub>("バスタブ");
		s32 step = spine->getTime() * (bathtub->getNumGripsDead() + 2);
		if ((f32)step >= prm->downStep.get()) {
			if (self->mMActor->curAnmEndsNext(0, nullptr))
				self->changeAnm(7, 0, prm->downSpeed.get());
		}
		break;
	}

	case 7:
		if (self->mMActor->curAnmEndsNext(0, nullptr))
			return TRUE;
		break;

	default: {
		self->changeAnm(0, 0, prm->downSpeed.get());
		TBathtub* bathtub = JDrama::TNameRefGen::search<TBathtub>("バスタブ");
		gpMarioParticleManager->emitAndBindToMtx(
		    0xF5, *bathtub->getRootJointMtx(), 0, this);
		break;
	}
	}

	return FALSE;
}

DEFINE_NERVE(TNerveKoopaGetShowered, TLiveActor)
{
	TKoopa* self = (TKoopa*)spine->getBody();
	self->changeAnm(14, 0, self->getSaveParam2()->waterhitSpeed.get());
	return self->mMActor->curAnmEndsNext(0, nullptr) ? TRUE : FALSE;
}

DEFINE_NERVE(TNerveKoopaStagger, TLiveActor)
{
	TKoopa* self = (TKoopa*)spine->getBody();
	self->changeAnm(9, 0, self->getSaveParam2()->staggerSpeed.get());
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
