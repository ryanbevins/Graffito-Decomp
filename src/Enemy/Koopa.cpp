#define MSL_STDFMODF_OUT_OF_LINE
#include <StackPadding.h>
#include <Enemy/Koopa.hpp>
#include <Camera/CameraShake.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DNode.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/JDrama/JDRNameRef.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JGadget/std-list.hpp>
#include <JSystem/JGeometry/JGMatrix34.hpp>
#include <JSystem/JGeometry/JGQuat4.hpp>
#include <JSystem/JGeometry/JGRotation3.hpp>
#include <JSystem/JGeometry/JGUtil.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
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
#undef MSL_STDFMODF_OUT_OF_LINE

namespace std {
float fmodf(float, float);
}

static const char SMS_NO_MEMORY_MESSAGE[]   = "メモリが足りません\n";
static const char MtxCalcTypeName0[]
    = "MActorMtxCalcType_Basic クラシックスケールＯＮ";
static const char MtxCalcTypeName1[]
    = "MActorMtxCalcType_Softimage クラシックスケールＯＦＦ";
static const char MtxCalcTypeName2[]
    = "MActorMtxCalcType_MotionBlend モーションブレンド";
static const char MtxCalcTypeName3[] = "MActorMtxCalcType_User ユーザー定義";
static const f32 dummy2850[3]        = { 0.0f, 0.0f, 0.0f };

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
int KoopaNeckCallBack(J3DNode* node, int timing)
{
	if (timing != 0)
		return 1;

	TKoopa* koopa  = (TKoopa*)node->mCallBackUserData;
	J3DJoint* joint = (J3DJoint*)node;
	MtxPtr mtx = j3dSys.getModel()->mNodeMatrices[joint->getJntNo()];

	JGeometry::TVec3<f32> toMario(*gpMarioPos);
	toMario.y += 180.0f;
	toMario.x -= mtx[0][3];
	toMario.y -= mtx[1][3];
	toMario.z -= mtx[2][3];

	if (koopa->isFlaming()) {
		TKoopaParams* prm = koopa->getSaveParam2();
		f32 angle = koopa->getFlameDirRate() * 6.2831855f
		            * prm->flameNeckRange.get() / 360.0f;
		f32 pitch = angle * prm->flameNeckDownRate.get();
		if (pitch > 0.0f)
			pitch = -pitch;
		if (koopa->unk154)
			angle = -angle;

		TPosition3f flameRot;
		flameRot.setEularY(angle);
		flameRot.zeroTrans();
		((TMtx34f*)mtx)->concat(flameRot, *(TMtx34f*)mtx);
		flameRot.setEularZ(pitch);
		flameRot.zeroTrans();
		((TMtx34f*)mtx)->concat(*(TMtx34f*)mtx, flameRot);
	}

	f32 focus = koopa->getNeckFocus();
	if (focus > 0.0f) {
		JGeometry::TVec3<f32> localX(mtx[0][0], mtx[1][0], mtx[2][0]);
		JGeometry::TVec3<f32> localY(mtx[0][1], mtx[1][1], mtx[2][1]);

		f32 yDot = localY.dot(toMario);
		JGeometry::TVec3<f32> projected(
		    toMario.x - localY.x * yDot, toMario.y - localY.y * yDot,
		    toMario.z - localY.z * yDot);

		if (!projected.isZero())
			projected.normalize();
		if (!toMario.isZero())
			toMario.normalize();

		f32 frontDot = localX.dot(projected);
		if (frontDot < 0.5f)
			focus *= (1.0f + frontDot) / 1.5f;

		JGeometry::TQuat4<f32> turn;
		turn.setRotate(localX, projected, focus);
		turn.normalize();

		if (!koopa->isFlaming()) {
			JGeometry::TVec3<f32> cross;
			cross.cross(projected, toMario);
			f32 angle = fabsf(atan2f(cross.length(), projected.dot(toMario)));
			if (toMario.dot(localY) < 0.0f)
				angle = -angle;

			JGeometry::TVec3<f32> axis;
			turn.getZDir(axis);
			JGeometry::TQuat4<f32> pitch;
			pitch.setRotate(axis, angle * focus);
			pitch.normalize();
			turn.mul(pitch, turn);
		}

		TPosition3f turnMtx;
		turnMtx.setQuat(turn);
		turnMtx.zeroTrans();
		((TMtx34f*)mtx)->concat(*(TMtx34f*)mtx, turnMtx);
	}

	PSMTXCopy(mtx, J3DSys::mCurrentMtx);
	return 1;
}
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

TKoopaParts::TKoopaParts(const char* name, u32 actorType, TKoopa* owner,
                         f32 radius)
    : THitActor(name)
    , mOwner(owner)
{
	TIdxGroupObj* group
	    = JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ");
	group->getChildren().push_back(this);

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

void TKoopaParts::set(const JGeometry::TVec3<f32>& position, f32 radius,
                      f32 height)
{
	if (height <= 0.0f)
		height = 2.0f * radius;
	mPosition.set(position);
	offHitFlag(0x2);
	offHitFlag(0x4);
	offHitFlag(0x1);
	mAttackRadius = radius;
	mAttackHeight = height;
	mDamageRadius = radius;
	mDamageHeight = height;
	calcEntryRadius();
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
	switch ((s32)message) {
	case HIT_MESSAGE_SPRAYED_BY_WATER:
		break;
	case HIT_MESSAGE_ATTACK:
		if (sender->mActorType == 0x08000024)
			mOwner->stagger(false);
		break;
	}
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

static inline void changeKoopaAnm(TKoopa* koopa, int bck, int btp, f32 rate)
{
	koopa->changeAnm(bck, btp, rate);
}

void TKoopaFlame::attack_(THitActor* actor)
{
	if (actor->receiveMessage(this, 0xA)) {
		if (actor == (THitActor*)gpMarioAddress) {
			f32 jump
			    = ((TKoopaParams*)((TEnemyManager*)mOwner->mManager)->unk38)
			          ->flameJump.value;
			JGeometry::TVec3<f32> throwVec(0.0f, 1.0f, 0.0f);
			SMS_ThrowMario(throwVec, jump);
			mOwner->unk155 = 1;
			changeKoopaAnm(
			    mOwner, 3, 0,
			    ((TKoopaParams*)((TEnemyManager*)mOwner->mManager)->unk38)
			        ->fireSpeed.value);
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

void TKoopaFlame::fire(const JGeometry::TVec3<f32>& position,
                      const JGeometry::TVec3<f32>& direction, f32 velocity,
                      f32 lifetime, f32 radius, f32 height)
{
	mPosition.set(position);
	unk78.set(direction);
	unk6C.set(position);
	unk84 = velocity;
	unk88 = lifetime;
	unk8C = 0.0f;
	unk90 = radius;
	unk94 = height;
}

void TKoopaFlame::control()
{
	if (!(getCurrentTime() < unk88)) {
		onHitFlag(0x2);
		onHitFlag(0x4);
		onHitFlag(0x1);
	} else {
		unk8C += unk84;

		JGeometry::TVec3<f32> position;
		position.scaleAdd(unk8C, unk6C, unk78);
		set(position, unk90, unk94);
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

	TKoopaParams* params
	    = (TKoopaParams*)((TEnemyManager*)mManager)->unk38;
	f32 waitSpeed = params->waitSpeed.get();
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
		mFlameHitActors[i] = new TKoopaFlame("クッパの吐く炎", 0x08000029,
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
	mBodyRadius = 800.0f;
	mHeadHeight = 2000.0f;

	TSpineEnemy::init(manager);
	onHitFlag(0x1);
	onHitFlag(0x4);
	offHitFlag(0x2);
	mSpine->initWith(&TNerveKoopaProvoke::theNerve());

	if (!mMActor->checkCurBckFromIndex(12)) {
		mMActor->setBckFromIndex(12);
		const char** bas = getBasNameTable();
		setAnmSound(!bas ? nullptr : bas[12]);
	}

	if (mMActor->getCurAnmIdx(3) != 1)
		mMActor->setBtpFromIndex(1);

	J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(0);
	ctrl->setRate(2.0f * SMSGetAnmFrameRate() * 0.5f);
	changeAnm(6, 0, 2.0f);

	if (mMActor->getAnmBck())
		mMActor->getAnmBck()->initSimpleMotionBlend(0x10);

	unk150 = getTargetDir(*gpMarioPos);
	initAnmSound();
	reset();

	J3DModelData* modelData = getModel()->getModelData();
	JUTNameTab* nameTab     = modelData->getJointName();
	mHeadJointIndex         = nameTab->getIndex("ago");
	mNeckJointIndex         = nameTab->getIndex("head");
	mJointIndex2            = nameTab->getIndex("neck");

	modelData = getModel()->getModelData();
	J3DNode* node = (J3DNode*)modelData->getJointNodePointer(mNeckJointIndex);
	node->setCallBack(&KoopaNeckCallBack);
	node->setCallBackUserData(this);

	unk1B8 = 1.0f;
	unk155 = 0;
}

void TKoopa::calcRootMatrix()
{
	TBathtub* bathtub = JDrama::TNameRefGen::search<TBathtub>("バスタブ");
	MtxPtr rootMtx    = *bathtub->getRootJointMtx();

	f32 offX = rootMtx[0][1] * -1500.0f;
	f32 offY = rootMtx[1][1] * -1500.0f;
	f32 offZ = rootMtx[2][1] * -1500.0f;

	TMtx34f mtx;
	MsMtxSetRotRPH(mtx, 0.0f, mRotation.y, 0.0f);
	mtx.mMtx[0][3] = 0.0f;
	mtx.mMtx[1][3] = 0.0f;
	mtx.mMtx[2][3] = 0.0f;

	mtx.concat(*(JGeometry::SMatrix34C<f32>*)rootMtx, mtx);

	mPosition.x = mtx.mMtx[0][3];
	mPosition.y = mtx.mMtx[1][3];
	mPosition.z = mtx.mMtx[2][3];
	mPosition.x += offX;
	mPosition.y += offY;
	mPosition.z += offZ;

	mtx.mMtx[0][3] = mPosition.x;
	mtx.mMtx[1][3] = mPosition.y;
	mtx.mMtx[2][3] = mPosition.z;
	PSMTXCopy(mtx, getModel()->getBaseTRMtx());

	JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
	getModel()->setBaseScale(scale);

	mScaling.x = 1.0f;
	mScaling.y = 1.0f;
	mScaling.z = 1.0f;
}

BOOL TKoopa::updateAnmSound()
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

bool TKoopa::isBreathing() const
{
	if (mMActor->getCurAnmIdx(0) == 4)
		return true;
	if (mMActor->getCurAnmIdx(0) == 5
	    && mMActor->getFrameCtrl(0)->getFrame() >= 85.0f)
		return true;
	return false;
}

bool TKoopa::isProvoking() const
{
	if (mMActor->getCurAnmIdx(0) == 6) {
		f32 frame = mMActor->getFrameCtrl(0)->getFrame();
		if (68.0f <= frame && frame <= 164.0f)
			return true;
	}
	return false;
}

bool TKoopa::isTumbling() const
{
	J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(0);
	f32 frame         = ctrl->getFrame();
	bool started
	    = mSpine->getCurrentNerve() == &TNerveKoopaTumble::theNerve()
	      && frame >= ((TKoopaParams*)((TEnemyManager*)mManager)->unk38)
	                      ->tumbleStartFrame.get();
	return started
	       && frame <= ((TKoopaParams*)((TEnemyManager*)mManager)->unk38)
	                       ->tumbleEndFrame.get();
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
	mHandHitActors[0]->perform(flags, graphics);
	mHandHitActors[1]->perform(flags, graphics);
	mBodyHitActor->perform(flags, graphics);

	if (flags & 1) {

		if (isTumbling()) {
			TBathtub* bathtub = JDrama::TNameRefGen::search<TBathtub>("バスタブ");
			bathtub->tumble(mRotation.y,
			                ((TKoopaParams*)((TEnemyManager*)mManager)->unk38)
			                    ->tumbleWeight.get());
		}

		setUpHitActors();
	}

	if (flags & 2) {
		if (isBreathing() || isProvoking()) {
			mMActor->calc();

			f32 flameScale = getSaveParam2()->flameScale.get();
			JGeometry::TVec3<f32> scale;
			scale.x = flameScale;
			scale.y = flameScale;
			scale.z = flameScale;

			JPABaseEmitter* emitter
			    = gpMarioParticleManager->emitAndBindToMtxPtr(
			        0x1F3, mMActor->getModel()->getAnmMtx(mNeckJointIndex), 3,
			        this);
			if (emitter) {
				emitter->unk154.x = scale.x;
				emitter->unk154.y = scale.y;
				emitter->unk154.z = scale.z;
				emitter->unk174.x = scale.x;
				emitter->unk174.y = scale.y;
				emitter->unk174.z = scale.z;
			}

			emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
			    0x1C3, mMActor->getModel()->getAnmMtx(mNeckJointIndex), 1, this);
			if (emitter) {
				emitter->unk154.set(scale);
				emitter->unk174.set(scale);
			}

			emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
			    0x1C2, mMActor->getModel()->getAnmMtx(mNeckJointIndex), 1, this);
			if (emitter) {
				emitter->unk154.set(scale);
				emitter->unk174.set(scale);
			}

			emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
			    0x1C1, mMActor->getModel()->getAnmMtx(mNeckJointIndex), 1, this);
			if (emitter) {
				emitter->unk154.set(scale);
				emitter->unk174.set(scale);
			}

			emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
			    0x1C0, mMActor->getModel()->getAnmMtx(mNeckJointIndex), 1, this);
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
	TBathtub* bathtub = JDrama::TNameRefGen::search<TBathtub>("バスタブ");
	MtxPtr rootMtx    = *bathtub->getRootJointMtx();

	f32 y = pos.y - rootMtx[1][3];
	f32 x = pos.x - rootMtx[0][3];
	f32 z = pos.z - rootMtx[2][3];

	f32 targetZ = rootMtx[0][2] * x + rootMtx[1][2] * y
	              + rootMtx[2][2] * z;
	f32 targetX = rootMtx[0][0] * x + rootMtx[1][0] * y
	              + rootMtx[2][0] * z;

	return matan(targetZ, targetX) * (360.0f / 65536.0f);
}

void TKoopa::stagger(bool ignoreFlame)
{
	if (&TNerveKoopaFall::theNerve() == mSpine->getCurrentNerve())
		return;
	if (&TNerveKoopaProvoke::theNerve() == mSpine->getCurrentNerve())
		return;
	if (!ignoreFlame
	    && mSpine->getCurrentNerve() == &TNerveKoopaFlame::theNerve())
		return;
	if (&TNerveKoopaTumble::theNerve() == mSpine->getCurrentNerve())
		return;
	if (&TNerveKoopaGetDown::theNerve() == mSpine->getCurrentNerve())
		return;
	if (&TNerveKoopaGetShowered::theNerve() == mSpine->getCurrentNerve())
		return;

	mSpine->pushNerve(&TNerveKoopaStagger::theNerve());
}

BOOL TKoopa::getShowered()
{
	if (&TNerveKoopaFall::theNerve() == mSpine->getCurrentNerve())
		return FALSE;
	if (&TNerveKoopaProvoke::theNerve() == mSpine->getCurrentNerve())
		return FALSE;
	if (&TNerveKoopaTumble::theNerve() == mSpine->getCurrentNerve())
		return FALSE;
	if (&TNerveKoopaGetDown::theNerve() == mSpine->getCurrentNerve())
		return FALSE;
	if (&TNerveKoopaGetShowered::theNerve() == mSpine->getCurrentNerve())
		return TRUE;

	if (&TNerveKoopaStagger::theNerve() == mSpine->getCurrentNerve()) {
		mSpine->setNext(&TNerveKoopaGetShowered::theNerve());
		return TRUE;
	}

	if (&TNerveKoopaFlame::theNerve() == mSpine->getCurrentNerve()) {
		mSpine->setNext(&TNerveKoopaWait::theNerve());
		return FALSE;
	}

	mSpine->pushNerve(&TNerveKoopaGetShowered::theNerve());
	return TRUE;
}

bool TKoopa::effectsTumble() const
{
	if (&TNerveKoopaTumble::theNerve() == mSpine->getCurrentNerve()) {
		int time = mSpine->getTime();
		if (time < 900 && time > 190)
			return TRUE;
	}
	return FALSE;
}

void TKoopa::getDown()
{
	if (&TNerveKoopaFall::theNerve() == mSpine->getCurrentNerve())
		return;
	if (&TNerveKoopaProvoke::theNerve() == mSpine->getCurrentNerve())
		return;
	if (&TNerveKoopaTumble::theNerve() == mSpine->getCurrentNerve())
		return;

	if (&TNerveKoopaStagger::theNerve() == mSpine->getCurrentNerve())
		mSpine->setNext(&TNerveKoopaGetDown::theNerve());
	if (&TNerveKoopaGetShowered::theNerve() == mSpine->getCurrentNerve())
		mSpine->setNext(&TNerveKoopaGetDown::theNerve());

	mSpine->pushNerve(&TNerveKoopaGetDown::theNerve());
}

BOOL TKoopa::allowsLaunch() const
{
	if (&TNerveKoopaTumble::theNerve() == mSpine->getCurrentNerve())
		return FALSE;
	return TRUE;
}

f32 TKoopa::getNeckFocus() const
{
	int idx            = mMActor->getCurAnmIdx(0);
	MActor* actor      = mMActor;
	J3DFrameCtrl* ctrl = actor->getFrameCtrl(0);
	f32 end            = ctrl->getEnd();
	f32 frame          = ctrl->getFrame();
	f32 focus          = 1.0f;

	switch (idx) {
	case 6:
		focus = 0.0f;
		if (frame >= 164.0f)
			focus = (frame - 164.0f) / (end - 164.0f);
		break;

	case 2:
		focus = 0.0f;
		break;

	case 0:
		if (frame <= 40.0f)
			focus = 1.0f - frame / 40.0f;
		else
			focus = 0.0f;
		break;

	case 1:
		focus = 0.0f;
		break;

	case 7:
		if (frame <= 125.0f)
			focus = 0.0f;
		else
			focus = (frame - 125.0f) / (end - 125.0f);
		break;

	case 9:
		if (frame <= 30.0f)
			focus = 1.0f - frame / 30.0f;
		else if (frame <= 65.0f)
			focus = 0.0f;
		else
			focus = (frame - 65.0f) / (end - 65.0f);
		break;

	case 8:
		if (frame <= 30.0f)
			focus = 1.0f - frame / 30.0f;
		else if (frame <= 170.0f)
			focus = 0.0f;
		else
			focus = (frame - 170.0f) / (end - 170.0f);
		break;

	case 14:
		if (frame <= 20.0f)
			focus = 1.0f - frame / 20.0f;
		else if (frame <= 40.0f)
			focus = 0.0f;
		else
			focus = (frame - 40.0f) / (end - 40.0f);
		break;

	case 4:
		focus = 0.0f;
		break;

	case 5:
		if (frame <= 103.0f)
			focus = 1.0f - frame / 103.0f;
		else
			focus = 0.0f;
		break;

	case 3:
		focus = frame / end;
		break;

	case 12:
		if (frame <= 200.0f) {
			focus = 1.0f;
		} else if (frame <= 255.0f) {
			focus = 1.0f - (frame - 200.0f) / 55.0f;
		} else if (frame <= 330.0f) {
			focus = 0.0f;
		} else if (frame <= 390.0f) {
			focus = (frame - 330.0f) / 60.0f;
		} else if (frame <= 440.0f) {
			focus = 1.0f;
		} else if (frame <= 480.0f) {
			focus = 1.0f - (frame - 440.0f) / 40.0f;
		} else if (frame <= 555.0f) {
			focus = 0.0f;
		} else if (frame <= 615.0f) {
			focus = (frame - 555.0f) / 60.0f;
		} else {
			focus = 1.0f;
		}
		break;
	}

	return focus;
}

bool TKoopa::isFlaming() const
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
	MActor* endActor   = mMActor;
	f32 frame          = ctrl->getFrame();
	f32 end            = endActor->getFrameCtrl(0)->getEnd();
	TEnemyManager* manager = (TEnemyManager*)mManager;
	TKoopaParams* prm      = (TKoopaParams*)manager->unk38;
	s32 time           = mSpine->getTime();
	f32 overStart      = prm->flameOverStart.get();
	s32 focusStart     = prm->flameFocusStartStep.get();
	s32 focusEnd       = prm->flameFocusEndStep.get();
	int idx            = mMActor->getCurAnmIdx(0);

	switch (idx) {
	case 5:
		return -((frame * overStart) / end);

	case 3:
	case 4: {
		f32 rate;
		if (mSpine->getTime() <= focusStart) {
			rate = -overStart;
		} else if (time <= focusEnd) {
			rate = ((overStart + 1.0f) * (time - focusStart))
			           / (focusEnd - focusStart)
			       - overStart;
		} else {
			rate = 1.0f;
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
	PAD_STACK(0x8);

	f32 rate              = getFlameDirRate();
	TEnemyManager* manager = (TEnemyManager*)mManager;
	bool reverse           = unk154;
	f32 range = ((TKoopaParams*)manager->unk38)->flameNeckRange.get();
	f32 degree = rate * range;
	degree = reverse ? -degree : degree;

	return mRotation.y + degree;
}

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
	bool canEmitFlame;
	if (mMActor->getCurAnmIdx(0) == 4) {
		canEmitFlame = true;
	} else if (mMActor->getCurAnmIdx(0) == 5
	           && mMActor->getFrameCtrl(0)->getFrame() >= 85.0f) {
		canEmitFlame = true;
	} else {
		canEmitFlame = false;
	}

	if (canEmitFlame) {
		int available = -1;
		bool waiting  = false;
		for (int i = 0; i < 10; ++i) {
			TKoopaFlame* flame = mFlameHitActors[i];
			if (!(flame->unk8C < flame->unk88)) {
				available = i;
			} else if (flame->unk8C
			           < 2.0f
			                 * ((TKoopaParams*)((TEnemyManager*)mManager)->unk38)
			                       ->flameRadius.get()) {
				waiting = true;
			}
		}

		if (!waiting && available >= 0) {
			MtxPtr mtx = mMActor->getModel()->getAnmMtx(mNeckJointIndex);
			JGeometry::TVec3<f32> position(mtx[0][3], mtx[1][3] - 500.0f,
			                                mtx[2][3]);
			JGeometry::TVec3<f32> direction(mtx[0][0], 0.0f, mtx[2][0]);
			direction.normalize();

			TKoopaParams* prm
			    = (TKoopaParams*)((TEnemyManager*)mManager)->unk38;
			mFlameHitActors[available]->fire(position, direction,
			                                prm->flameVelocity.get(), 4000.0f,
			                                prm->flameRadius.get(),
			                                prm->flameHeight.get());
		}
	} else {
		for (int i = 0; i < 10; ++i) {
			TKoopaFlame* flame = mFlameHitActors[i];
			flame->unk88 = 0.0f;
			flame->unk8C = 1.0f;
		}
	}

	TKoopaParams* prm = (TKoopaParams*)((TEnemyManager*)mManager)->unk38;
	MtxPtr headMtx = mMActor->getModel()->getAnmMtx(mHeadJointIndex);
	JGeometry::TVec3<f32> headPosition;
	((TPosition3f*)headMtx)->getTrans(headPosition);
	headPosition.y -= 200.0f;
	mHeadHitActor->set(headPosition, prm->headRadius.get(), 0.0f);
	mBodyHitActor->set(mPosition, 800.0f, 2000.0f);
}

inline const TNerveKoopaTurnR& TNerveKoopaTurnR::theNerve()
{
	static TNerveKoopaTurnR instance;
	return instance;
}

inline const TNerveKoopaTurnL& TNerveKoopaTurnL::theNerve()
{
	static TNerveKoopaTurnL instance;
	return instance;
}

inline const TNerveKoopaTumble& TNerveKoopaTumble::theNerve()
{
	static TNerveKoopaTumble instance;
	return instance;
}

const TNerveKoopaProvoke& TNerveKoopaProvoke::theNerve()
{
	static TNerveKoopaProvoke instance;
	return instance;
}

inline const TNerveKoopaWait& TNerveKoopaWait::theNerve()
{
	static TNerveKoopaWait instance;
	return instance;
}

inline const TNerveKoopaFlame& TNerveKoopaFlame::theNerve()
{
	static TNerveKoopaFlame instance;
	return instance;
}

const TNerveKoopaGetDown& TNerveKoopaGetDown::theNerve()
{
	static TNerveKoopaGetDown instance;
	return instance;
}

const TNerveKoopaStagger& TNerveKoopaStagger::theNerve()
{
	static TNerveKoopaStagger instance;
	return instance;
}

const TNerveKoopaFall& TNerveKoopaFall::theNerve()
{
	static TNerveKoopaFall instance;
	return instance;
}

const TNerveKoopaGetShowered& TNerveKoopaGetShowered::theNerve()
{
	static TNerveKoopaGetShowered instance;
	return instance;
}

// TODO: original helper name unknown.
static inline bool passesKoopaWaitFrame(TKoopa* koopa, f32 frame)
{
	bool passed = false;
	if (koopa->mMActor->getFrameCtrl(0)->getFrame() <= frame) {
		J3DFrameCtrl* ctrl = koopa->mMActor->getFrameCtrl(0);
		if (frame <= 0.005f + (ctrl->getFrame() + ctrl->getRate()))
			passed = true;
	}
	return passed;
}

BOOL TNerveKoopaWait::execute(TSpineBase<TLiveActor>* spine) const
{
	TKoopa* self = (TKoopa*)spine->getBody();

	if (self->unk19C > 0) {
		self->changeAnm(
		    12, 1, ((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)
		               ->waitSpeed.get());
		return FALSE;
	}

	TBathtub* bathtub = JDrama::TNameRefGen::search<TBathtub>("バスタブ");
	JGeometry::TVec3<f32> marioSpeed(*gpMarioSpeedX, *gpMarioSpeedY,
	                                 *gpMarioSpeedZ);
	JGeometry::TVec3<f32> predicted = marioSpeed;
	predicted.scale(self->getSaveParam2()->marioEstimationWait.get());

	u8 hasGrip = bathtub->getNextGrip(
	    *gpMarioPos, predicted,
	    ((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)->waitRange.get(),
	    &self->unk150);
	if (!hasGrip) {
		predicted.set(*gpMarioSpeedX, *gpMarioSpeedY, *gpMarioSpeedZ);
		predicted.scale(self->getSaveParam2()->marioEstimationFire.get());
		self->unk150 = bathtub->getNextJuncture(*gpMarioPos, predicted);
	}

	f32 turnDiff = std::fmodf(
	    360.0f + ((self->unk150 - self->mRotation.y) - -180.0f), 360.0f);
	turnDiff += -180.0f;

	s32 turn;
	if (turnDiff < -((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)
	                    ->focusRange.get())
		turn = -1;
	else if (turnDiff > ((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)
	                       ->focusRange.get())
		turn = 1;
	else
		turn = 0;

	if (hasGrip) {
		switch (turn) {
		case -1:
			spine->pushNerve(&TNerveKoopaTurnL::theNerve());
			break;
		case 1:
			spine->pushNerve(&TNerveKoopaTurnR::theNerve());
			break;
		default: {
			self->changeAnm(
			    12, 1, ((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)
			               ->waitSpeed.get());
			bool shouldTumble;
			if (self->mMActor->getCurAnmIdx(0) != 12) {
				shouldTumble = false;
			} else {
				if (self->mMActor->curAnmEndsNext(0, nullptr)) {
					shouldTumble = true;
				} else {
					if (passesKoopaWaitFrame(self, 2.0f))
						shouldTumble = true;
					else if (passesKoopaWaitFrame(self, 400.0f))
						shouldTumble = true;
					else if (passesKoopaWaitFrame(self, 700.0f))
						shouldTumble = true;
					else
						shouldTumble = false;
				}
			}
			if (shouldTumble
			    && JDrama::TNameRefGen::search<TBathtub>("バスタブ")
			           ->allowsTumble())
				spine->pushNerve(&TNerveKoopaTumble::theNerve());
			break;
		}
		}
		return FALSE;
	}

	switch (turn) {
	case -1:
		spine->pushNerve(&TNerveKoopaTurnL::theNerve());
		break;
	case 1:
		spine->pushNerve(&TNerveKoopaTurnR::theNerve());
		break;
	default: {
		f32 flameDiff = std::fmodf(
		    360.0f + ((self->getTargetDir(*gpMarioPos) - self->unk150) - -180.0f),
		    360.0f);
		flameDiff += -180.0f;
		self->unk154 = flameDiff < 0.0f;
		spine->setNext(&TNerveKoopaFlame::theNerve());
		break;
	}
	}
	return FALSE;
}

BOOL TNerveKoopaTumble::execute(TSpineBase<TLiveActor>* spine) const
{
	TKoopa* self = (TKoopa*)spine->getBody();
	TKoopaParams* params
	    = (TKoopaParams*)((TEnemyManager*)self->mManager)->unk38;
	self->changeAnm(8, 0, params->tumbleSpeed.get());
	self->mMActor->getFrameCtrl(0);
	if (spine->getTime() == 190) {
		gpCameraShake->startShake((EnumCamShakeMode)0x27, 1.0f);
		static TBathtub* bathtub
		    = JDrama::TNameRefGen::search<TBathtub>("バスタブ");
		gpMarioParticleManager->emitAndBindToMtx(
		    0xF5, *bathtub->getRootJointMtx(), 0, this);
		if (SMS_IsMarioTouchGround4cm())
			SMSRumbleMgr->start(1, (f32*)nullptr);
	}
	if (self->mMActor->curAnmEndsNext(0, nullptr))
		return TRUE;
	return FALSE;
}

BOOL TNerveKoopaFlame::execute(TSpineBase<TLiveActor>* spine) const
{
	TKoopa* self = (TKoopa*)spine->getBody();

	switch (self->mMActor->getCurAnmIdx(0)) {
	case 5:
		if (self->mMActor->curAnmEndsNext(0, nullptr)) {
			self->changeAnm(4, 0, 2.0f);
			spine->setNext(&TNerveKoopaFlame::theNerve());
		} else {
			self->unk155 = 0;
		}
		break;

	case 3:
		if (!self->mMActor->curAnmEndsNext(0, nullptr))
			break;

		if (self->unk155) {
			if (gpMSound->gateCheck(0x89AD)) {
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x89AD, &self->unk158, 0, nullptr, 0, 4);
			}
			self->unk155 = 0;
		}

		if (self->unk19C > 0) {
			spine->setNext(&TNerveKoopaWait::theNerve());
			return FALSE;
		} else {
			TBathtub* bathtub
			    = JDrama::TNameRefGen::search<TBathtub>("バスタブ");
			JGeometry::TVec3<f32> speed(*gpMarioSpeedX, *gpMarioSpeedY,
			                             *gpMarioSpeedZ);
			JGeometry::TVec3<f32> predicted;
			f32 scale = self->getSaveParam2()->marioEstimationWait.get();
			predicted.x = speed.x * scale;
			predicted.y = speed.y * scale;
			predicted.z = speed.z * scale;

			u8 hasGrip = bathtub->getNextGrip(
			    *gpMarioPos, predicted,
			    ((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)
			        ->waitRange.get(),
			    &self->unk150);
			if (!hasGrip) {
				JGeometry::TVec3<f32> speed2(
				    *gpMarioSpeedX, *gpMarioSpeedY, *gpMarioSpeedZ);
				f32 scale = self->getSaveParam2()->marioEstimationFire.get();
				predicted.x = speed2.x * scale;
				predicted.y = speed2.y * scale;
				predicted.z = speed2.z * scale;
				self->unk150
				    = bathtub->getNextJuncture(*gpMarioPos, predicted);
			}

			f32 turn = std::fmodf(
			    360.0f + ((self->unk150 - self->mRotation.y) - -180.0f),
			    360.0f);
			turn += -180.0f;
			int turnDir;
			if (turn < -((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)
			               ->focusRange.get())
				turnDir = -1;
			else if (turn > ((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)
			               ->focusRange.get())
				turnDir = 1;
			else
				turnDir = 0;

			if (hasGrip) {
				spine->setNext(&TNerveKoopaWait::theNerve());
			} else {
				switch (turnDir) {
				case -1:
					spine->pushNerve(&TNerveKoopaTurnL::theNerve());
					break;
				case 1:
					spine->pushNerve(&TNerveKoopaTurnR::theNerve());
					break;
				case 0: {
					f32 flameDiff = std::fmodf(
					    360.0f
					        + ((self->getTargetDir(*gpMarioPos) - self->unk150)
					           - -180.0f),
					    360.0f);
					flameDiff += -180.0f;
					self->unk154 = flameDiff < 0.0f;
					self->changeAnm(
					    5, 0, ((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)
			                   ->fireSpeed.get());
					spine->setNext(&TNerveKoopaFlame::theNerve());
					break;
				}
				}
			}
		}
		break;

	case 4:
		if (spine->getTime()
		    < ((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)
		          ->flameCount.get())
			break;

		if ((spine->getTime() & 7) == 0) {
			TBathtub* bathtub
			    = JDrama::TNameRefGen::search<TBathtub>("バスタブ");
			JGeometry::TVec3<f32> speed(*gpMarioSpeedX, *gpMarioSpeedY,
			                             *gpMarioSpeedZ);
			JGeometry::TVec3<f32> predicted;
			f32 scale = self->getSaveParam2()->marioEstimationWait.get();
			predicted.x = speed.x * scale;
			predicted.y = speed.y * scale;
			predicted.z = speed.z * scale;

			u8 hasGrip = bathtub->getNextGrip(
			    *gpMarioPos, predicted,
			    ((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)
			        ->waitRange.get(),
			    &self->unk150);
			if (!hasGrip) {
				JGeometry::TVec3<f32> speed2(
				    *gpMarioSpeedX, *gpMarioSpeedY, *gpMarioSpeedZ);
				f32 scale = self->getSaveParam2()->marioEstimationFire.get();
				predicted.x = speed2.x * scale;
				predicted.y = speed2.y * scale;
				predicted.z = speed2.z * scale;
				self->unk150
				    = bathtub->getNextJuncture(*gpMarioPos, predicted);
			}

			f32 turn = std::fmodf(
			    360.0f + ((self->unk150 - self->mRotation.y) - -180.0f),
			    360.0f);
			turn += -180.0f;
			int turnDir;
			if (turn < -((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)
			               ->focusRange.get())
				turnDir = -1;
			else if (turn > ((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)
			               ->focusRange.get())
				turnDir = 1;
			else
				turnDir = 0;

			if (hasGrip || turnDir != 0)
				self->changeAnm(
				    3, 0, ((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)
		                   ->fireSpeed.get());
		} else if (self->mMActor->curAnmEndsNext(0, nullptr)
		           && spine->getTime()
		                  >= ((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)
		                         ->flameFocusEndStep.get()) {
			self->changeAnm(
			    3, 0, ((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)
		                   ->fireSpeed.get());
		}
		break;

	default: {
		self->changeAnm(
		    5, 0, ((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)
		                   ->fireSpeed.get());
		break;
	}
	}

	return FALSE;
}

static inline void turnKoopa(TKoopa* self, f32 turn)
{
	if (turn > 0.0f)
		self->changeAnm(11, 0, turn * self->getSaveParam2()->turnAnim.get());
	else
		self->changeAnm(10, 0, -turn * self->getSaveParam2()->turnAnim.get());
	self->mRotation.y
	    = -180.0f
	      + JGeometry::TUtil<f32>::mod(
	          360.0f + ((self->mRotation.y + turn) - -180.0f), 360.0f);
}

BOOL TNerveKoopaTurnL::execute(TSpineBase<TLiveActor>* spine) const
{
	TKoopa* self = (TKoopa*)spine->getBody();

	f32 turn = -180.0f
	           + std::fmodf(
	               360.0f + ((self->unk150 - self->mRotation.y) - -180.0f),
	               360.0f);

	bool turning;
	if (turn < -((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)
	               ->turnSpeed.get()) {
		turn = -self->getSaveParam2()->turnSpeed.get();
		turnKoopa(self, turn);
		turning = true;
	} else if (turn < 0.0f) {
		turnKoopa(self, turn);
		turning = true;
	} else {
		turning = false;
	}

	if (turning)
		return FALSE;
	return TRUE;
}

BOOL TNerveKoopaTurnR::execute(TSpineBase<TLiveActor>* spine) const
{
	TKoopa* self = (TKoopa*)spine->getBody();

	f32 turn = -180.0f
	           + std::fmodf(
	               360.0f + ((self->unk150 - self->mRotation.y) - -180.0f),
	               360.0f);

	bool turning;
	if (turn > ((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)
	               ->turnSpeed.get()) {
		turn = self->getSaveParam2()->turnSpeed.get();
		turnKoopa(self, turn);
		turning = true;
	} else if (turn > 0.0f) {
		turnKoopa(self, turn);
		turning = true;
	} else {
		turning = false;
	}

	if (turning)
		return FALSE;
	return TRUE;
}

BOOL TNerveKoopaGetDown::execute(TSpineBase<TLiveActor>* spine) const
{
	TKoopa* self = (TKoopa*)spine->getBody();

	switch (self->mMActor->getCurAnmIdx(0)) {
	case 0:
		if (self->mMActor->curAnmEndsNext(0, nullptr))
			self->changeAnm(
			    1, 0, ((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)
			              ->downSpeed.get());
		break;

	case 1: {
		TBathtub* bathtub = JDrama::TNameRefGen::search<TBathtub>("バスタブ");
		f32 downStep
		    = ((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)
		          ->downStep.get();
		s32 step = spine->getTime() * (bathtub->getNumGripsDead() + 2);
		if ((f32)step < downStep)
			break;
		if (self->mMActor->curAnmEndsNext(0, nullptr))
			self->changeAnm(
			    7, 0, ((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)
			              ->downSpeed.get());
		break;
	}

	case 7:
		if (self->mMActor->curAnmEndsNext(0, nullptr))
			return TRUE;
		break;

	default: {
		self->changeAnm(
		    0, 0, ((TKoopaParams*)((TEnemyManager*)self->mManager)->unk38)
		              ->downSpeed.get());
		static TBathtub* bathtub
		    = JDrama::TNameRefGen::search<TBathtub>("バスタブ");
		gpMarioParticleManager->emitAndBindToMtx(
		    0xF5, *bathtub->getRootJointMtx(), 0, this);
		break;
	}
	}

	return FALSE;
}

BOOL TNerveKoopaGetShowered::execute(TSpineBase<TLiveActor>* spine) const
{
	TKoopa* self = (TKoopa*)spine->getBody();
	TKoopaParams* params
	    = (TKoopaParams*)((TEnemyManager*)self->mManager)->unk38;
	self->changeAnm(14, 0, params->waterhitSpeed.get());
	return self->mMActor->curAnmEndsNext(0, nullptr) ? TRUE : FALSE;
}

BOOL TNerveKoopaStagger::execute(TSpineBase<TLiveActor>* spine) const
{
	TKoopa* self = (TKoopa*)spine->getBody();
	TKoopaParams* params
	    = (TKoopaParams*)((TEnemyManager*)self->mManager)->unk38;
	self->changeAnm(9, 0, params->staggerSpeed.get());
	return self->mMActor->curAnmEndsNext(0, nullptr) ? TRUE : FALSE;
}

BOOL TNerveKoopaProvoke::execute(TSpineBase<TLiveActor>* spine) const
{
	TKoopa* self = (TKoopa*)spine->getBody();
	self->changeAnm(6, 0, 2.0f);
	if (self->mMActor->curAnmEndsNext(0, nullptr)) {
		spine->setNext(&TNerveKoopaWait::theNerve());
		return FALSE;
	}
	return FALSE;
}

#pragma dont_inline on
TSpineEnemyParams* TEnemyManager::getSaveParam() const { return unk38; }
#pragma dont_inline off

BOOL TNerveKoopaFall::execute(TSpineBase<TLiveActor>* spine) const
{
	TKoopa* self = (TKoopa*)spine->getBody();
	TKoopaParams* params
	    = (TKoopaParams*)((TEnemyManager*)self->mManager)->unk38;
	self->changeAnm(2, 0, params->fallSpeed.get());
	return FALSE;
}

TKoopaManager::TKoopaManager(const char* name)
    : TEnemyManager(name)
{
}

TSpineEnemy* TKoopaManager::createEnemyInstance() { return nullptr; }

void TKoopaManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
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
