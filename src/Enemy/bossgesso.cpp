#define J3DMTXCALC_BASIC_INIT_OUT_OF_LINE
#define J3DMTXCALC_MAYA_INIT_OUT_OF_LINE
#include <Enemy/BossGesso.hpp>
#include <Enemy/BossGessoTentacle.hpp>
#include <Enemy/BossGessoPolDrop.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/EffectObj.hpp>
#include <Enemy/NameKuri.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorAnm.hpp>
#include <Map/Map.hpp>
#include <MoveBG/ItemManager.hpp>
#include <GC2D/GCConsole2.hpp>
#include <Player/MarioMain.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/ModelWaterManager.hpp>
#include <Camera/CameraShake.hpp>
#include <MarioUtil/TexUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/ShadowUtil.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <Map/PollutionManager.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>
#include <System/MSoundMainSide.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Strategy.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DCluster.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DMaterial.hpp>
#include <stdlib.h>
#undef J3DMTXCALC_MAYA_INIT_OUT_OF_LINE
#undef J3DMTXCALC_BASIC_INIT_OUT_OF_LINE

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

const char* bgeso_bastable[] = {
	nullptr,
	"/scene/bgeso/bas/bgeso_cannon.bas",
	nullptr,
	"/scene/bgeso/bas/bgeso_damage.bas",
	nullptr,
	"/scene/bgeso/bas/bgeso_eyedamage.bas",
	"/scene/bgeso/bas/bgeso_fly.bas",
	"/scene/bgeso/bas/bgeso_hit.bas",
	nullptr,
	"/scene/bgeso/bas/bgeso_limit.bas",
	"/scene/bgeso/bas/bgeso_osen.bas",
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	"/scene/bgeso/bas/bgeso_roll_all_start.bas",
	nullptr,
	"/scene/bgeso/bas/bgeso_roll_start.bas",
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	"/scene/bgeso/bas/bgeso_wait.bas",
	"/scene/bgeso/bas/bgeso_wait2.bas",
	"/scene/bgeso/bas/bgeso_wait3.bas",
};

static void getAttackModeStr(int) { }

static BOOL isNozzleWater(THitActor* param_1)
{
	if (!param_1->isActorType(0x1000001))
		return FALSE;

	return gpModelWaterManager->checkParticleFlag((TWaterHitActor*)param_1,
	                                              0x40);
}

TBossGessoParams::TBossGessoParams(const char* path)
    : TSpineEnemyParams(path)
    , PARAM_INIT(mSLUnisonInter, 2000)
    , PARAM_INIT(mSLUnisonHoming, 200)
    , PARAM_INIT(mSLSingleHoming, 200)
    , PARAM_INIT(mSLBeakHoming, 200)
    , PARAM_INIT(mSLRegenFoot, 4800)
    , PARAM_INIT(mSLAmputeeTime, 1200)
    , PARAM_INIT(mSLRestTime, 360)
    , PARAM_INIT(mSLStunTime, 1200)
    , PARAM_INIT(mSLBeakDamageHeight, 150.0f)
    , PARAM_INIT(mSLBeakDamageRadius, 150.0f)
    , PARAM_INIT(mSLBeakLengthLimit, 1200.0f)
    , PARAM_INIT(mSLBeakLengthDamage, 600.0f)
    , PARAM_INIT(mSLBeakLengthPollute, 500.0f)
    , PARAM_INIT(mSLUnisonAttackSpeed, 2.0f)
    , PARAM_INIT(mSLDoubleAttackSpeed, 1.2f)
    , PARAM_INIT(mSLSkipRopeAttackSpeed, 1.0f)
    , PARAM_INIT(mSLSingleAttackLen, 2200.0f)
    , PARAM_INIT(mSLDoubleAttackLen, 1900.0f)
    , PARAM_INIT(mSLUnisonAttackLen, 1900.0f)
    , PARAM_INIT(mSLForceUnisonLen, 800.0f)
    , PARAM_INIT(mSLGuardLen, 500.0f)
    , PARAM_INIT(mSLTentacleStretch, 0.1f)
    , PARAM_INIT(mSLBeakStretch, 0.07f)
    , PARAM_INIT(mSLEyeDamageRadius, 100.0f)
    , PARAM_INIT(mSLEyeDamageHeight, 150.0f)
    , PARAM_INIT(mSLShootRadius, 8000.0f)
    , PARAM_INIT(mSLBlurJoint, 10)
    , PARAM_INIT(mSLBlurScale, 1.0f)
    , PARAM_INIT(mSLColumnScale, 3.0f)
    , PARAM_INIT(mSLSightAngle, 120.0f)
    , PARAM_INIT(mSLAmputeeWait, 720)
{
	TParams::load(mPrmPath);
}

TBGBeakHit::TBGBeakHit(TBossGesso* owner, const char* name)
    : TTakeActor(name)
    , mOwner(owner)
{
	JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ")
	    ->getChildren()
	    .push_back(this);

	initHitActor(0x8000008, 1, -0x80000000, 0.0f, 0.0f,
	             mOwner->getSaveParam2()->mSLBeakDamageRadius.get(),
	             mOwner->getSaveParam2()->mSLBeakDamageHeight.get());
	offHitFlag(HIT_FLAG_NO_COLLISION);
	unkA4.zero();
}

MtxPtr TBGBeakHit::getTakingMtx() { return unk74; }

// TODO: fake
static inline JGeometry::TVec3<f32> fromPolar(f32 theta, f32 radius)
{
	return JGeometry::TVec3<f32>(radius * JMASSin(theta * (65536.0f / 360.0f)),
	                             0.0f,
	                             radius * JMASCos(theta * (65536.0f / 360.0f)));
}

BOOL TBGBeakHit::moveRequest(const JGeometry::TVec3<f32>& where_to)
{
	TBossGessoParams* params = mOwner->getSaveParam2();

	unkA4 = fromPolar(gpMarioOriginal->getIntendedYaw(),
	                  gpMarioOriginal->getIntendedMag()
	                      * params->mSLBeakStretch.get());

	JGeometry::TVec3<f32> delta = mOwner->mPosition;
	delta -= where_to;
	delta.scale(0.001f);
	unkA4 += delta;

	mPosition = where_to;
	return FALSE;
}

BOOL TBGBeakHit::receiveMessage(THitActor* sender, u32 message)
{
	if (sender->isActorType(0x1000001)) {
		if (!isNozzleWater(sender))
			return true;

		mOwner->gotEyeDamage();

		gpMarioParticleManager->emit(0xE7, &mPosition, 0, nullptr);
		return true;
	}

	if (mOwner->mAttackMode == 3)
		return false;

	if (mOwner->getLatestNerve() == &TNerveBGPollute::theNerve()
	    || mOwner->getLatestNerve() == &TNerveBGPolDrop::theNerve()
	    || mOwner->getLatestNerve() == &TNerveBGBeakDamage::theNerve())
		return false;

	if (sender->getActorType() == 0x80000001) {
		if (message == HIT_MESSAGE_TAKE) {
			TTakeActor* actor = (TTakeActor*)sender;
			if (actor->mHeldObject != nullptr && actor->mHeldObject != this)
				return false;

			mHolder = actor;

			TBossGesso* owner = mOwner;
			if (owner->unk190.color.a != 0) {
				if ((owner->unk198 & 8) == 0)
					gpMarDirector->getConsole()->startAppearBalloon(0xE0028,
					                                               true);
				owner->unk198 |= 8;
			}

			return true;
		}

		if (message == HIT_MESSAGE_UNK7 || message == HIT_MESSAGE_UNK8) {
			// TODO: inlined from TBossGesso?
			JGeometry::TVec3<f32> delta = mPosition;
			TBossGesso* gesso           = mOwner;
			delta -= gesso->mPosition;
			f32 length = gesso->getSaveParam2()->mSLBeakLengthDamage.get();

			if (delta.length() >= length)
				mOwner->gotBeakDamage();

			mHolder = nullptr;
			return true;
		}
	}

	return false;
}

void TBGBeakHit::perform(u32 param_1, JDrama::TGraphics* param_2)
{
	if (param_1 & 1) {
		mOwner->getJointTransByIndex(26, &mPosition);
		mPosition.y -= mDamageHeight * 0.5f;

		ensureTakeSituation();
		if (mHolder != nullptr && !unkA4.isZero()) {
			mPosition += unkA4;
			JGeometry::TVec3<f32> delta = mHolder->mPosition;
			delta += unkA4;
			mHolder->moveRequest(delta);
			unkA4.zero();
		}

		if (mHolder != nullptr) {
			JGeometry::TVec3<f32> us2mario = SMS_GetMarioPos();
			us2mario -= mOwner->mPosition;
			if (!us2mario.isZero())
				VECNormalize(&us2mario, &us2mario);
			else
				us2mario.set(0.0f, 0.0f, 1.0f);

			JGeometry::TVec3<f32> offset = us2mario;
			JGeometry::TVec3<f32> perp;
			JGeometry::TVec3<f32> up(0.0f, 1.0f, 0.0f);
			perp.cross(offset, up);
			if (!perp.isZero())
				VECNormalize(&perp, &perp);
			else
				perp.set(1.0f, 0.0f, 0.0f);

			unk74.mMtx[0][0] = perp.x;
			unk74.mMtx[1][0] = perp.y;
			unk74.mMtx[2][0] = perp.z;
			unk74.mMtx[0][1] = 0.0;
			unk74.mMtx[1][1] = 1.0;
			unk74.mMtx[2][1] = 0.0;
			unk74.mMtx[0][2] = us2mario.x;
			unk74.mMtx[1][2] = us2mario.y;
			unk74.mMtx[2][2] = us2mario.z;

			offset.scale(mDamageRadius);
			offset.add(mPosition);
			unk74.mMtx[0][3] = offset.x;
			unk74.mMtx[1][3] = offset.y;
			unk74.mMtx[2][3] = offset.z;

			JGeometry::TVec3<f32> ownerToUs = mPosition;
			ownerToUs -= mOwner->mPosition;
			f32 beakPullDist = ownerToUs.length();

			f32 lenPollute = mOwner->getSaveParam2()->mSLBeakLengthPollute.get();
			if (mOwner->unk190.color.a != 0 && beakPullDist >= lenPollute) {
				mHolder->receiveMessage(this, HIT_MESSAGE_UNK8);
			}

			f32 lenLimit = mOwner->getSaveParam2()->mSLBeakLengthLimit.get();
			if (beakPullDist >= lenLimit) {
				mHolder->receiveMessage(this, HIT_MESSAGE_UNK8);
				mOwner->gotBeakDamage();
			}
		}
	}
}

TBGEyeHit::TBGEyeHit(TBossGesso* owner, int joint_index, const char* name)
    : THitActor(name)
    , mOwner(owner)
    , mJointIndex(joint_index)
{
	JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ")
	    ->getChildren()
	    .push_back(this);

	initHitActor(0x8000009, 1, 0x1000000, 0.0f, 0.0f,
	             mOwner->getSaveParam2()->mSLEyeDamageRadius.get(),
	             mOwner->getSaveParam2()->mSLEyeDamageHeight.get());
	offHitFlag(HIT_FLAG_NO_COLLISION);
}

BOOL TBGEyeHit::receiveMessage(THitActor* sender, u32 message)
{
	if (mOwner->getAttackMode() == 3)
		return false;

	if (sender->getActorType() == 0x1000001
	    && message == HIT_MESSAGE_SPRAYED_BY_WATER) {
		mOwner->gotEyeDamage();
		gpMarioParticleManager->emit(0xE7, &sender->mPosition, 0, nullptr);
		return true;
	}

	return mOwner->receiveMessage(sender, message);
}

void TBGEyeHit::perform(u32 param_1, JDrama::TGraphics* param_2)
{
	THitActor::perform(param_1, param_2);
	if (param_1 & 2)
		mOwner->getJointTransByIndex(mJointIndex, &mPosition);
}

TBGBodyHit::TBGBodyHit(TBossGesso* owner, int joint_index, const char* name)
    : mOwner(owner)
    , mJointIndex(joint_index)
{
	JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ")
	    ->getChildren()
	    .push_back(this);

	initHitActor(0x8000005, 1, -0x7f000000, 300.0f, 300.0f, 300.0f, 300.0f);
	offHitFlag(HIT_FLAG_NO_COLLISION);
}

BOOL TBGBodyHit::receiveMessage(THitActor* sender, u32 message)
{
	if (sender->getActorType() == 0x1000001
	    && message == HIT_MESSAGE_SPRAYED_BY_WATER) {
		gpMarioParticleManager->emit(0xE7, &sender->mPosition, 0, nullptr);
		return true;
	}

	return mOwner->receiveMessage(sender, message);
}

void TBGBodyHit::perform(u32 param_1, JDrama::TGraphics* param_2)
{
	if (param_1 & 2)
		mOwner->getJointTransByIndex(mJointIndex, &mPosition);
	THitActor::perform(param_1, param_2);
}

TBossGessoMtxCalc::TBossGessoMtxCalc(TBossGesso* owner)
    : M3UMtxCalcSIAnmBlendQuat(true)
    , mOwner(owner)
{
	unk50 = 0.0f;
}

void TBossGessoMtxCalc::joinAnm(int param_1)
{
	J3DAnmTransformKey* anm
	    = mOwner->getActorKeeper()->getMActorAnmData()->getUnk2C()->getAnmPtr(
	        param_1);

	if (unk54 == anm)
		return;

	unk58 = unk54;
	unk54 = anm;
	unk50 = 1.0f;
}

void TBossGessoMtxCalc::setAnm(int param_1) { }

void TBossGessoMtxCalc::calc(u16 param_1)
{
	M3UMtxCalcSIAnmBlendQuat::calc(param_1);
	if (param_1 != 26)
		return;

	if (mOwner->mBeak != nullptr && mOwner->mBeak->isTaken()) {
		TBGBeakHit* beak = mOwner->mBeak;
		MtxPtr mtx26     = mOwner->getModel()->getAnmMtx(param_1);
		mtx26[0][3]      = beak->mPosition.x;
		mtx26[1][3]      = beak->mPosition.y + 50.0f;
		mtx26[2][3]      = beak->mPosition.z;

		JGeometry::TVec3<f32> local_28 = beak->mPosition;
		local_28 -= mOwner->mPosition;

		f32 fVar4 = VECMag(&local_28);
		if (fVar4 > 0.0f)
			fVar4 = 300.0f / fVar4;
		else
			fVar4 = 1.0f;

		if (fVar4 > 1.0f)
			fVar4 = 1.0f;

		MTXCopy(mtx26, J3DSys::mCurrentMtx);
		mtx26[0][0] *= fVar4;
		mtx26[1][0] *= fVar4;
		mtx26[2][0] *= fVar4;
		mtx26[0][2] *= fVar4;
		mtx26[1][2] *= fVar4;
		mtx26[2][2] *= fVar4;
	} else {
		if (mOwner->mAttackMode == 3) {
			MtxPtr mtx26 = mOwner->getModel()->getAnmMtx(param_1);
			mtx26[0][0] *= 0.02f;
			mtx26[1][0] *= 0.02f;
			mtx26[2][0] *= 0.02f;
			MTXCopy(mtx26, J3DSys::mCurrentMtx);
		}
	}
}

TBGBinder::TBGBinder() { }

void TBGBinder::bind(TLiveActor* param_1)
{
	TBossGesso* gesso = (TBossGesso*)param_1;

	JGeometry::TVec3<f32> local_3c = gesso->mLinearVelocity;
	local_3c += gesso->mPosition;

	if (gesso->isAirborne()) {
		JGeometry::TVec3<f32> local_48 = gesso->mVelocity;
		local_3c += local_48;
		local_48.y -= gesso->getGravityY();
		if (local_48.y < TLiveActor::mVelocityMinY)
			local_48.y = TLiveActor::mVelocityMinY;
		gesso->mVelocity = local_48;
	}

	if (gesso->getLatestNerve() == &TNerveBGDie::theNerve()
	    && gesso->getMActor()->checkCurBckFromIndex(6)) {

		// TODO: defo an inline
		JGeometry::TVec3<f32> local_b4 = local_3c;
		local_b4 -= gesso->mPosition;
		gesso->mLinearVelocity = local_b4;

		if (gpMarDirector->mMap != 9
		    && gesso->mPosition.y - local_3c.y > 0.0f) {

			// TODO: this is likely an inline where xyz are passed as separate
			// args
			f32 someZ = local_3c.z;
			const TBGCheckData* pTStack_4c;
			f32 dVar7 = gpMap->checkGround(local_3c.x,
			                               local_3c.y + gesso->getHeadHeight(),
			                               someZ, &pTStack_4c);
			dVar7 += 1.0f;
			const TBGCheckData* pTStack_50;
			f32 dVar8 = gpMap->checkGround(
			    local_3c.x, gesso->mPosition.y + gesso->getHeadHeight(), someZ,
			    &pTStack_50);
			dVar8 += 1.0f;

			if (dVar7 < dVar8) {
				TEffectColumWater* enemy
				    = (TEffectColumWater*)gpConductor->makeOneEnemyAppear(
				        gesso->mPosition, "エフェクト水柱マネージャー", 1);
				if (enemy) {
					f32 scale = gesso->getSaveParam2()->mSLColumnScale.get();
					JGeometry::TVec3<f32> local_5c;
					local_5c.x = scale;
					local_5c.y = scale;
					local_5c.z = scale;
					enemy->generate(gesso->mPosition, local_5c);
				}

				if (gpMSound->gateCheck(0x286A))
					MSoundSESystem::MSoundSE::startSoundActor(
					    0x286A, &gesso->mPosition, 0, nullptr, 0, 4);

				gesso->unk1A4 = 1.0f;
				SMSRumbleMgr->start(8, &gesso->unk1A4);
			}
		}
	} else {
		// TODO: this is likely an inline where xyz are passed as separate args
		f32 someY = local_3c.y;
		f32 someZ = local_3c.z;

		const TBGCheckData* local_60;
		f32 fVar1 = gpMap->checkGround(
		    local_3c.x, someY + gesso->getHeadHeight(), someZ, &local_60);
		fVar1 += 1.0f;

		f32 gessoY = gesso->mPosition.y;
		if (gessoY - someY > 0.0f) {
			const TBGCheckData* local_64;
			f32 dVar8 = gpMap->checkGround(
			    local_3c.x, gessoY + gesso->getHeadHeight(), someZ, &local_64);
			dVar8 += 1.0f;

			if (dVar8 > fVar1) {
				local_60 = local_64;
				fVar1    = dVar8;
			}
		}

		if (someY <= fVar1) {
			local_3c.y       = fVar1;
			gesso->mVelocity = JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f);

			gesso->offLiveFlag(LIVE_FLAG_AIRBORNE);
		} else {
			gesso->onLiveFlag(LIVE_FLAG_AIRBORNE);
		}

		gesso->mGroundHeight = fVar1;
		gesso->mGroundPlane  = local_60;

		// TODO: defo an inline
		JGeometry::TVec3<f32> local_c0 = local_3c;
		local_c0 -= gesso->mPosition;
		gesso->mLinearVelocity = local_c0;
	}
}

TBGCork::TBGCork(TBossGesso* owner)
    : mOwner(owner)
    , unk4(nullptr)
    , unk8(nullptr)
    , unkC(0)
{
	unk4 = mOwner->getActorKeeper()->createMActor("bgeso_kolk.bmd", 0);
	unk8 = mOwner->getActorKeeper()->createMActor("bgeso_kolk_break.bmd", 0);
}

void TBGCork::crush()
{
	if (unkC)
		return;

	unk8->setBckFromIndex(8);
	unk4->getModel()->setBaseTRMtx(unk8->getModel()->getBaseTRMtx());
	unkC = 1;
}

inline void TBGCork::perform(u32 param_1, JDrama::TGraphics* param_2)
{
	MActor* model;
	if (unkC == 0) {
		model = unk4;
		if (param_1 & 2) {
			MtxPtr src = mOwner->getModel()->getAnmMtx(27);
			PSMTXCopy(src, model->getModel()->getBaseTRMtx());
		}
	} else {
		model = unk8;
	}
	model->perform(param_1, param_2);
}

TBossGesso::TBossGesso(const char* name)
    : TSpineEnemy(name)
    , mBeak(nullptr)
    , mMtxCalc(nullptr)
    , mAttackMode(0)
    , mTimeInCurrentAttackMode(0)
    , unk178(nullptr)
    , unk17C(0)
    , mPolDrop(nullptr)
    , unk188(0.0f)
    , mCork(nullptr)
    , unk194(0)
    , unk195(0)
    , unk196(0)
    , unk198(0)
    , unk19C(0)
    , unk1A0(0)
    , unk1A1(0)
    , unk1A4(0.0f)
    , unk1A8(0)
    , unk1AC(0)
    , unk1AE(0)
{
	mBinder    = new TBGBinder;
	mTurnSpeed = 0.2f;
}

void TBossGesso::init(TLiveManager* param_1)
{
	mManager = param_1;
	mManager->manageActor(this);
	mMActorKeeper = new TMActorKeeper(mManager, 0xf);
	mMActor       = mMActorKeeper->createMActor("bgeso_body.bmd", 0);
	for (int i = 0; i < 4; ++i)
		mTentacles[i] = new TBGTentacle(this, 6, i);
	mBeak     = new TBGBeakHit(this);
	mLeftEye  = new TBGEyeHit(this, 7);
	mRightEye = new TBGEyeHit(this, 4);
	mBody     = new TBGBodyHit(this, 0);

	initHitActor(0x8000005, 5, -0x7f000000, 300.0f, 300.0f, 300.0f, 300.0f);
	onHitFlag(HIT_FLAG_NO_COLLISION);

	mSpine->initWith(&TNerveBGWait::theNerve());
	mMtxCalc = new TBossGessoMtxCalc(this);

	getMActor()->setCalcForBck(mMtxCalc);

	getMActor()->calc();
	getMActor()->setLightType(1);

	unk178   = getActorKeeper()->createMActor("bgeso_dirty_white.bmd", 0);
	mPolDrop = new TBGPolDrop;

	mPolDrop->setMActors(
	    getActorKeeper()->createMActor("bgeso_osenball_white.bmd", 0),
	    getActorKeeper()->createMActor("bgeso_osenball.bmd", 0));

	mCork = new TBGCork(this);

	J3DModel* model = getMActor()->getModel();
	if (!model->getSkinDeform()) {
		J3DSkinDeform* skinDeform = new J3DSkinDeform;
		model->setSkinDeform(skinDeform, J3D_DEFORM_ATTACH_FLAG_UNK_1);
	}

	reset();

	mHitPoints = getSaveParam2() ? getSaveParam2()->mSLHitPointMax.get() : 1;

	offLiveFlag(LIVE_FLAG_UNK100);
	getMActor()->offMakeDL();
	onLiveFlag(LIVE_FLAG_UNK8);
	mScaledBodyRadius = 330.0f;
	initAnmSound();
	unk190.color.r = 0;
	unk190.color.g = 0;
	unk190.color.b = 0;
	unk190.color.a = 0xE6;

	getMActor()
	    ->getModel()
	    ->getModelData()
	    ->getMaterialNodePointer(0)
	    ->getTevBlock()
	    ->setTevKColor(0, &unk190);

	const ResTIMG* image
	    = (const ResTIMG*)JKRGetResource("/scene/map/pollution/H_ma_rak.bti");

	if (image)
		SMS_ChangeTextureAll(getMActor()->getModel()->getModelData(),
		                     "H_ma_rak_dummy", *image);
}

void TBossGesso::rumblePad(int param_1, const JGeometry::TVec3<f32>& param_2)
{
	if (!SMS_IsMarioTouchGround4cm())
		return;

	JGeometry::TVec3<f32> delta = SMS_GetMarioPos();
	delta -= param_2;
	f32 fVar2 = delta.length();
	f32 fVar1 = (3000.0f - fVar2) / 1000.0f;

	if (fVar1 < 0.0f)
		return;

	if (fVar1 > 1.0f)
		fVar1 = 1.0f;

	switch (param_1) {
	case 0:
		fVar1 *= 0.4f;
		break;
	case 1:
		fVar1 *= 0.7f;
		break;
	case 2:
		break;
	}

	unk1A4 = fVar1;
	SMSRumbleMgr->start(8, &unk1A4);
}

void TBossGesso::definiteRumble() { }

void TBossGesso::continuousRumble() { }

#pragma dont_inline on
f32 TBossGesso::lenFromToeToMario()
{
	f32 min = 100000.0f;

	for (int i = 0; i < 4; ++i) {
		if (mTentacles[i]->isThing2())
			continue;

		JGeometry::TVec3<f32> tipPos
		    = mTentacles[i]->getLastNode()->getPosition();

		f32 len = tipPos.length();
		if (len < min)
			min = len;
	}

	return min;
}
#pragma dont_inline off

#pragma dont_inline on
void TBossGesso::showMessage(u32 param_1)
{
	u32 idx  = param_1 == 0xE0028 ? 3 : param_1 - 0xE0003;
	u32 flag = param_1 == 0xE0003 ? 0 : 1 << idx;

	if ((unk198 & flag) == 0)
		gpMarDirector->getConsole()->startAppearBalloon(param_1, true);

	unk198 |= flag;
}
#pragma dont_inline off

void TBossGesso::checkTakeMsg() { }

void TBossGesso::changeBck(int param_1)
{
	mMtxCalc->joinAnm(param_1);
	getMActor()->setFrameCtrlForBck(param_1);

	J3DFrameCtrl* ctrl = getMActor()->getFrameCtrl(0);
	if (ctrl != nullptr)
		unk188 = 10.0f / ctrl->getEnd();

	const char** table = getBasNameTable();
	setAnmSound(!table ? nullptr : table[param_1]);
}

// TODO: this inline is 99% incorrect, need to try harder =(
bool TBossGesso::inSightAngle(f32 a)
{
	return inSight() < a * 0.5f ? TRUE : FALSE;
}

// TODO: this inline is 99% incorrect, need to try harder =(
f32 TBossGesso::inSight()
{
	JGeometry::TVec3<f32> local_90 = SMS_GetMarioPos();
	local_90 -= mPosition;
	f32 dVar9  = MsGetRotFromZaxisY(local_90);
	f32 dVar10 = MsWrap(mRotation.y, dVar9 - 180.0f, dVar9 + 180.0f);
	return fabsf(dVar9 - dVar10);
}

BOOL TBossGesso::is2ndFightNow() const
{
	if (gpMarDirector->unk7D == 4)
		return TRUE;
	return FALSE;
}

void TBossGesso::stopIfRoll()
{
	if (mAttackMode != 7)
		return;

	changeAttackMode(ASTATE_SINGLE);

	mSpine->reset();
	mSpine->setNext(&TNerveBGWait::theNerve());

	for (int i = 0; i < TENTACLE_NUM; ++i) {
		TBGTentacle* tentacle = mTentacles[i];
		if (tentacle->mState != 5) {
			BOOL isThing;
			if (tentacle->mState == 6
			    || (u32)(tentacle->mState - 3) <= 1)
				isThing = TRUE;
			else
				isThing = FALSE;
			if (!isThing)
				tentacle->changeStateAndFixNodes(0);
		}
	}
}

void TBossGesso::changeAttackMode(int new_mode)
{
	mAttackMode              = new_mode;
	mTimeInCurrentAttackMode = 0;
	switch (mAttackMode) {
	case ASTATE_SINGLE:
	case ASTATE_DOUBLE:
	case ASTATE_SKIP_ROPE:
		break;

	case ASTATE_GUARD: {
		static int idx[2] = { 1, 3 };
		for (int i = 0; i < 2; ++i) {
			TBGTentacle* tentacle = mTentacles[idx[i]];
			BOOL isThing;
			if (tentacle->mState == 4 || tentacle->mState == 6
			    || tentacle->mState == 3)
				isThing = TRUE;
			else
				isThing = FALSE;
			if (!isThing)
				tentacle->changeStateAndFixNodes(10);
		}
		break;
	}

	case ASTATE_UNISON:
		for (int i = 0; i < TENTACLE_NUM; ++i) {
			TBGTentacle* tentacle = mTentacles[i];
			if (tentacle->mState != 5) {
				BOOL isThing;
				if (tentacle->mState == 6
				    || (u32)(tentacle->mState - 3) <= 1)
					isThing = TRUE;
				else
					isThing = FALSE;
				if (!isThing)
					tentacle->changeStateAndFixNodes(0);
			}
		}
		break;

	case ASTATE_ROLL:
		if ((int)(MsRandF() * 100.0f) < 0x28)
			unk1A1 = true;
		else
			unk1A1 = false;
		changeAllTentacleState(0x8);
		mSpine->reset();
		mSpine->setNext(&TNerveBGRoll::theNerve());
		mSpine->pushAfterCurrent(&TNerveBGWait::theNerve());
		break;

	case ASTATE_SHOOT:
		if (mSpine->getLatestNerve() != &TNerveBGPolDrop::theNerve()
		    && mSpine->getLatestNerve() != &TNerveBGPollute::theNerve()
		    && mSpine->getLatestNerve() != &TNerveBGTentacleDamage::theNerve()
		    && mSpine->getLatestNerve() != &TNerveBGEyeDamage::theNerve()
		    && mSpine->getLatestNerve() != &TNerveBGBeakDamage::theNerve()) {
			mSpine->reset();
			mSpine->setNext(&TNerveBGPolDrop::theNerve());
			mSpine->pushAfterCurrent(&TNerveBGWait::theNerve());
			changeAllTentacleState(0x8);
		}
		break;

	case ASTATE_UNK6:
		mTentacles[0]->changeStateAndFixNodes(0x9);
		break;
	}
}

void TBossGesso::gotTentacleDamage()
{
	if (mSpine->getLatestNerve() == &TNerveBGBeakDamage::theNerve())
		return;

	mSpine->reset();
	mSpine->setNext(&TNerveBGTentacleDamage::theNerve());
	mSpine->pushAfterCurrent(&TNerveBGWait::theNerve());
}

void TBossGesso::gotEyeDamage()
{
	if (mSpine->getLatestNerve() == &TNerveBGEyeDamage::theNerve()
	    || mSpine->getLatestNerve() == &TNerveBGBeakDamage::theNerve()
	    || mSpine->getLatestNerve() == &TNerveBGPollute::theNerve())
		return;

	mSpine->reset();
	mSpine->setNext(&TNerveBGEyeDamage::theNerve());
	mSpine->pushAfterCurrent(&TNerveBGWait::theNerve());
}

void TBossGesso::gotBeakDamage()
{
	if (mSpine->getLatestNerve() == &TNerveBGTentacleDamage::theNerve())
		return;

	if (mHitPoints > 0)
		--mHitPoints;

	if (mHitPoints == 0) {
		mSpine->setNext(&TNerveBGDie::theNerve());
	} else {
		mSpine->setNext(&TNerveBGBeakDamage::theNerve());
		mSpine->pushAfterCurrent(&TNerveBGWait::theNerve());
	}
}

void TBossGesso::changeAllTentacleState(int param_1)
{
	for (int i = 0; i < TENTACLE_NUM; ++i)
		if (mTentacles[i]->mState != 5 && !mTentacles[i]->isThing())
			mTentacles[i]->changeStateAndFixNodes(param_1);
}

void TBossGesso::forceAllTentacleState(int param_1)
{
	for (int i = 0; i < TENTACLE_NUM; ++i)
		mTentacles[i]->changeStateAndFixNodes(param_1);
}

void TBossGesso::startPollute() { }

void TBossGesso::stopPollute() { }

void TBossGesso::launchPolDrop()
{
	if (mPolDrop->getUnk58())
		return;

	JGeometry::TVec3<f32> local_14;
	if (checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)) {
		local_14 = getPosition();
		local_14.x += 1.0f;
	} else {
		getJointTransByIndex(26, &local_14);
	}

	JGeometry::TVec3<f32> VStack_20;
	SMSCalcJumpVelocityXZ(SMS_GetMarioPos(), local_14, 32.0f, 0.2f, &VStack_20);
	mPolDrop->launch(local_14, VStack_20);

	MtxPtr ptr = getModel()->getAnmMtx(27);
	gpMarioParticleManager->emitAndBindToMtxPtr(0x94, ptr, 0, nullptr);
	gpMarioParticleManager->emitAndBindToMtxPtr(0x93, ptr, 0, nullptr);

	unk195 += 1;
}

void TBossGesso::setEyeDamageBtp(int) { }

BOOL TBossGesso::tentacleHeld() const
{
	for (int i = 0; i < TENTACLE_NUM; ++i)
		if (mTentacles[i]->mState == 3)
			return true;

	return false;
}

void TBossGesso::tentacleAttack() { }

BOOL TBossGesso::beakHeld() const { return !!mBeak->mHolder; }

void TBossGesso::tentacleWait() { }

const char** TBossGesso::getBasNameTable() const { return bgeso_bastable; }

BOOL TBossGesso::receiveMessage(THitActor* sender, u32 message)
{
	if (sender->getActorType() == 0x1000001
	    && message == HIT_MESSAGE_SPRAYED_BY_WATER) {
		gpMarioParticleManager->emit(0xE7, &sender->mPosition, 0, nullptr);
		return true;
	}

	return false;
}

#pragma dont_inline on
void TBossGesso::doAttackSingle()
{
	if (getLatestNerve() != &TNerveBGPollute::theNerve()) {
		unk178->setBckFromIndex(-1);
		unk17C = 0;
	}

	if (gpMarDirector->unk58 < 0x1E0)
		return;

	{
		TMarDirector* dir = gpMarDirector;
		bool blocked = true;
		if (!dir->isTalkModeNow() && !dir->checkUnk124Thing2())
			blocked = false;
		if (blocked)
			return;
	}

	if (unk1A8 > 0) {
		unk1A8 -= 1;
		return;
	}

	if (gpMarDirector->unk7D == 4 ? 1 : 0) {

		JGeometry::TVec3<f32> delta = SMS_GetMarioPos();

		if (SMS_GetMarioPos().y + 20.0f < mPosition.y)
			return;

		delta -= mPosition;

		if (delta.squared() > 3610000.0f)
			return;
	}

	for (int i = 0; i < 2; ++i) {
		static const int idxarray[] = { 1, 3 };
		TBGTentacle* tentacle       = mTentacles[idxarray[i]];

		if (inSightAngle(getSaveParam2()->mSLSightAngle.get())
		    && tentacle->mState == 0) {
			JGeometry::TVec3<f32> delta = SMS_GetMarioPos();
			delta -= tentacle->getFirstNode()->getPosition();

			f32 singleAttackLen = getSaveParam2()->mSLSingleAttackLen.get();
			if (delta.squared() < singleAttackLen * singleAttackLen) {
				tentacle->changeStateAndFixNodes(1);
				break;
			}
		}
	}

	if (mTentacles[3]->isThing2() && mTentacles[1]->isThing2()
	    && !(mTentacles[2]->isThing2() && mTentacles[0]->isThing2())) {
		if (mTimeInCurrentAttackMode <= getSaveParam2()->mSLUnisonInter.get())
			return;

		if (!(gpMarDirector->unk7D == 4 ? 1 : 0))
			return;

		changeAttackMode(ASTATE_ROLL);
		return;
	}

	if (tentacleHeld())
		return;

	JGeometry::TVec3<f32> delta = SMS_GetMarioPos();

	f32 unisonAttackLen2 = getSaveParam2()->mSLUnisonAttackLen.get();
	unisonAttackLen2 *= unisonAttackLen2;

	f32 forceUnisonLen2 = getSaveParam2()->mSLForceUnisonLen.get();
	forceUnisonLen2 *= forceUnisonLen2;

	delta -= mPosition;

	if (mBeak->getHolder() != nullptr
	    && !(mTentacles[3]->isThing2() && mTentacles[1]->isThing2())) {
		for (int i = 0; i < 2; ++i) {
			static const int idxarray[] = { 1, 3 };
			TBGTentacle* tentacle       = mTentacles[idxarray[i]];
			if (tentacle->mState != 1 && tentacle->mState != 4
			    && tentacle->mState != 5 && tentacle->mState != 3
			    && tentacle->mState != 6) {
				tentacle->changeStateAndFixNodes(1);
			}
		}
		return;
	}

	f32 dist2 = delta.squared();

	if (dist2 < forceUnisonLen2) {
		if (gpMarioOriginal->isTouchGround4cm())
			changeAttackMode(ASTATE_UNISON);
		return;
	}

	if (dist2 < unisonAttackLen2) {
		if (gpMarioOriginal->isTouchGround4cm()) {
			if (mTimeInCurrentAttackMode > getSaveParam2()->mSLUnisonInter.get()) {
				if (gpMarDirector->unk7D == 4 ? 1 : 0)
					changeAttackMode(ASTATE_ROLL);
				else
					changeAttackMode(ASTATE_UNISON);
			}
			return;
		}
	}

	if (mTentacles[0]->mState == 1 || mTentacles[1]->mState == 1
	    || mTentacles[2]->mState == 1 || mTentacles[3]->mState == 1)
		return;

	if (mCork->unkC != 0 && unk195 < 3) {
		f32 shootRadius2 = getSaveParam2()->mSLShootRadius.get();
		shootRadius2 *= shootRadius2;

		if (mTimeInCurrentAttackMode > getSaveParam2()->mSLUnisonInter.get()) {
			if (dist2 < shootRadius2) {
				JGeometry::TVec3<f32> shootDelta = SMS_GetMarioPos();
				shootDelta -= mPosition;
				f32 dVar9  = MsGetRotFromZaxisY(shootDelta);
				f32 dVar10 = MsWrap(mRotation.y, dVar9 - 180.0f,
				                    dVar9 + 180.0f);
				if (fabsf(dVar9 - dVar10) < 30.0f)
					changeAttackMode(ASTATE_SHOOT);
			}
			return;
		}
	}

	if (gpMarDirector->unk7D == 4 ? 1 : 0) {
		if (inSightAngle(getSaveParam2()->mSLSightAngle.get())) {
			if (mTimeInCurrentAttackMode
			    > getSaveParam2()->mSLUnisonInter.get()) {
				changeAttackMode(ASTATE_ROLL);
				unk195 = 0;
			}
		}
	}
}
#pragma dont_inline off

void TBossGesso::doAttackDouble()
{
	if (mBeak->getHolder() != nullptr || tentacleHeld()) {
		changeAttackMode(ASTATE_SKIP_ROPE);
		return;
	}

	JGeometry::TVec3<f32> delta = mPosition;
	delta -= SMS_GetMarioPos();

	f32 doubleAttackLen2 = getSaveParam2()->mSLDoubleAttackLen.value;
	doubleAttackLen2 *= doubleAttackLen2;

	if (inSightAngle(getSaveParam2()->mSLSightAngle.get())
	    && delta.squared() < doubleAttackLen2) {

		for (int i = 0; i < 2; ++i) {
			static const int idxarray[] = { 0, 2 };
			TBGTentacle* tentacle       = mTentacles[idxarray[i]];
			if (tentacle->mState == 0) {
				tentacle->changeStateAndFixNodes(1);
			}
		}
	}

	if (mBeak->getHolder() != nullptr)
		return;

	if (!mTentacles[3]->isThing2() || !mTentacles[1]->isThing2())
		changeAttackMode(ASTATE_SINGLE);
}

void TBossGesso::doAttackSkipRope()
{
	if (mBeak->getHolder() != nullptr) {
		changeAttackMode(ASTATE_SKIP_ROPE);
		return;
	}

	if (mBeak->getHolder() == nullptr && !tentacleHeld()) {
		changeAttackMode(ASTATE_SINGLE);
		return;
	}

	if (inSightAngle(getSaveParam2()->mSLSightAngle.get())) {
		for (int i = 0; i < 2; ++i) {
			static const int idxarray[] = { 0, 2 };
			TBGTentacle* tentacle       = mTentacles[idxarray[i]];
			if (tentacle->mState != 2 && tentacle->mState != 1
			    && tentacle->mState != 4 && tentacle->mState != 5
			    && tentacle->mState != 3 && tentacle->mState != 6) {
				tentacle->changeStateAndFixNodes(1);
			}
		}
	}
}

void TBossGesso::doAttackUnison()
{
	if (mBeak->getHolder() != nullptr) {
		changeAttackMode(ASTATE_SKIP_ROPE);
		return;
	}
	JGeometry::TVec3<f32> delta = SMS_GetMarioPos();
	delta -= mPosition;

	f32 unisonAttackLen2 = getSaveParam2()->mSLUnisonAttackLen.value;
	unisonAttackLen2 *= unisonAttackLen2;

	if (inSightAngle(getSaveParam2()->mSLSightAngle.get())
	    && gpMarioOriginal->isTouchGround4cm()
	    && delta.squared() < unisonAttackLen2) {

		BOOL bVar3 = true;
		for (int i = 0; i < TENTACLE_NUM; ++i) {
			TBGTentacle* tentacle = mTentacles[i];
			if (i == mTimeInCurrentAttackMode / 8 && tentacle->mState == 0) {
				tentacle->changeStateAndFixNodes(1);
				bVar3 = false;
			}
		}

		if (bVar3 && mTimeInCurrentAttackMode > 300)
			changeAttackMode(ASTATE_SINGLE);
	} else {
		for (int i = 0; i < TENTACLE_NUM; ++i) {
			TBGTentacle* tentacle = mTentacles[i];
			if (tentacle->mState == 1)
				return;

			if (!tentacle->isThing2() && tentacle->mState != 0
			    && tentacle->mState != 2 && tentacle->mState != 5)
				tentacle->changeStateAndFixNodes(2);
		}

		changeAttackMode(ASTATE_SINGLE);
	}
}

#pragma dont_inline on
void TBossGesso::doAttackShoot()
{
	if (mBeak->getHolder() != nullptr) {
		changeAttackMode(ASTATE_SINGLE);
		return;
	}

	if (mTimeInCurrentAttackMode > 120
	    && mSpine->getLatestNerve() != &TNerveBGPolDrop::theNerve()) {
		changeAttackMode(ASTATE_SINGLE);
		return;
	}

	if (inSightAngle(getSaveParam2()->mSLSightAngle.get())) {
		JGeometry::TVec3<f32> delta = SMS_GetMarioPos();
		delta -= mPosition;

		f32 singleAttackLen = getSaveParam2()->mSLSingleAttackLen.get();
		if (delta.squared() < singleAttackLen * singleAttackLen) {
			changeAttackMode(ASTATE_SINGLE);
		}
	}
}
#pragma dont_inline off

inline void TBossGesso::doAttackGuard()
{
	if (mBeak->getHolder() != nullptr) {
		changeAttackMode(ASTATE_SKIP_ROPE);
		return;
	}

	// TODO: inSight inline is definitely wrong...
	if (inSightAngle(getSaveParam2()->mSLSightAngle.get())) {
		JGeometry::TVec3<f32> delta = SMS_GetMarioPos();
		delta -= mPosition;

		f32 guardLen = getSaveParam2()->mSLGuardLen.get();
		if (!(guardLen * guardLen < delta.squared())) {
			if (!mTentacles[3]->isThing2())
				return;

			if (!mTentacles[1]->isThing2())
				return;
		}

		changeAllTentacleState(0);
		changeAttackMode(ASTATE_SINGLE);
	}
}

void TBossGesso::doAttackRoll()
{
	if (getLatestNerve() == &TNerveBGRoll::theNerve())
		return;

	changeAllTentacleState(0);
	changeAttackMode(ASTATE_SINGLE);
}

void TBossGesso::moveObject()
{
	TLiveActor::moveObject();

	if (mSpine->getLatestNerve() == &TNerveBGDie::theNerve())
		return;

	if (mSpine->getLatestNerve() == &TNerveBGPolDrop::theNerve())
		return;

	if (mSpine->getLatestNerve() == &TNerveBGPollute::theNerve())
		return;

	if (mSpine->getLatestNerve() == &TNerveBGBeakDamage::theNerve())
		return;

	switch (mAttackMode) {
	case ASTATE_SINGLE:
		doAttackSingle();
		break;

	case ASTATE_DOUBLE:
		doAttackDouble();
		break;

	case ASTATE_SKIP_ROPE:
		doAttackSkipRope();
		break;

	case ASTATE_UNISON:
		doAttackUnison();
		break;

	case ASTATE_SHOOT:
		doAttackShoot();
		break;

	case ASTATE_GUARD:
		doAttackGuard();
		break;

	case ASTATE_ROLL:
		doAttackRoll();
		break;
	}

	if (mSpine->getLatestNerve() != &TNerveBGTentacleDamage::theNerve()
	    && mSpine->getLatestNerve() != &TNerveBGBeakDamage::theNerve()
	    && mSpine->getLatestNerve() != &TNerveBGTug::theNerve()) {

		// TODO: inline?
		BOOL bVar4;
		if (mTentacles[0]->mState == 3)
			bVar4 = true;
		else if (mTentacles[1]->mState == 3)
			bVar4 = true;
		else if (mTentacles[2]->mState == 3)
			bVar4 = true;
		else if (mTentacles[3]->mState == 3)
			bVar4 = true;
		else
			bVar4 = false;

		if (bVar4 || mBeak->mHolder != nullptr)
			mSpine->pushNerve(&TNerveBGTug::theNerve());
	}

	mTimeInCurrentAttackMode += 1;
}

void TBossGesso::reset()
{
	for (int i = 0; i < TENTACLE_NUM; ++i) {
		mTentacles[i]->resetAllNodes(mPosition);
		mTentacles[i]->getFirstNode()->onUnk24();
	}

	if (gpMarDirector->getCurrentMap() == 3
	    && gpMarDirector->getCurrentStage() == 0)
		changeAttackMode(ASTATE_UNK6);
	else
		changeAttackMode(ASTATE_SINGLE);

	calcRootMatrix();
	getMActor()->getModel()->calc();
}

void TBossGesso::calcRootMatrix()
{
	if (getLatestNerve() == &TNerveBGDie::theNerve()
	    && getMActor()->checkCurBckFromIndex(6)) {
		mRotation = MsGetRotFromZaxis(mVelocity);
		MtxPtr mA = getModel()->getBaseTRMtx();

		MsMtxSetXYZRPH(mA, mPosition.x, mPosition.y, mPosition.z, mRotation.x,
		               mRotation.y, mRotation.z);

		Mtx local_50;

		f32 s = JMASSin(0x4000);
		f32 c = JMASCos(0x4000);

		local_50[0][0] = 1.0;
		local_50[0][1] = 0.0;
		local_50[0][2] = 0.0;
		local_50[0][3] = 0.0;

		local_50[1][0] = 0.0;
		local_50[1][1] = c;
		local_50[1][2] = -s;
		local_50[1][3] = 0.0;

		local_50[2][0] = 0.0;
		local_50[2][1] = s;
		local_50[2][2] = c;
		local_50[2][3] = 0.0;

		MTXConcat(mA, local_50, mA);

		getModel()->setBaseScale(mScaling);
		return;
	}

	TSpineEnemy::calcRootMatrix();
}

void TBossGesso::perform(u32 param_1, JDrama::TGraphics* param_2)
{
	if (param_1 & 2) {
		if (beakHeld() || tentacleHeld()) {
			if (gpMarioOriginal->getIntendedMag() > 0.1f) {
				if (gpMSound->gateCheck(0x208D))
					MSoundSESystem::MSoundSE::startSoundActor(
					    0x208D, &mPosition, 0, nullptr, 0, 4);
			}
		}
	}

	if (param_1 & 1) {
		if (unk1A0 == 0 && !is2ndFightNow() && mAttackMode == 6) {
			JGeometry::TVec3<f32> delta = SMS_GetMarioPos();
			delta -= mPosition;
			if (delta.squared() < 4000000.0f) {
				unk19C += 1;
				if (unk19C >= 1200) {
					showMessage(0xE0004);
					unk1A0 = 1;
				}
			}
		}

		if (mBeak->mHolder != nullptr) {
			if (mTimeInCurrentAttackMode % 4 == 0)
				rumblePad(1, mBeak->mPosition);
		}

		if (unk1AC > 0)
			unk1AC -= 1;

		if (unk1AE > 0)
			unk1AE -= 1;
	}

	if (mAttackMode == 6) {
		if (param_1 & 2) {
			JDrama::TViewObj* container
			    = JDrama::TNameRefGen::search<JDrama::TViewObj>("container");
			if (container == nullptr) {
				changeAttackMode(ASTATE_SINGLE);
			} else if (mTentacles[0]->mState != 4) {
				mTentacles[0]->getFirstNode()->setPosition(
				    JGeometry::TVec3<f32>(11603.0f, 2114.3f, 2411.4f));
				mTentacles[0]->getFirstNode()[1].setPosition(
				    JGeometry::TVec3<f32>(11510.0f, 2114.3f, 2411.4f));
			}
		}
		mTentacles[0]->testPerform(param_1, param_2);
		return;
	}

	if (param_1 & 0x200) {
		if (getLatestNerve() == &TNerveBGBeakDamage::theNerve())
			SMS_AddDamageFogEffect(getMActor()->getModel()->getModelData(),
			                       mPosition, param_2);
		else
			SMS_ResetDamageFogEffect(getMActor()->getModel()->getModelData());

		getMActor()
		    ->getModel()
		    ->getModelData()
		    ->getMaterialNodePointer(0)
		    ->getTevBlock()
		    ->setTevKColor(0, &unk190);
	}

	TSpineEnemy::perform(param_1, param_2);
	mPolDrop->testPerform(param_1, param_2);

	if (checkLiveFlag(LIVE_FLAG_DEAD))
		return;

	if ((param_1 & 2) && unk194 > 0) {
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x13B, getModel()->getAnmMtx(0), 1, this);
		unk194 -= 1;
	}

	mCork->perform(param_1, param_2);

	if (param_1 & 4) {
		TCircleShadowRequest request;
		MtxPtr clusterMtx = getMActor()->getModel()->getAnmMtx(1);
		JGeometry::TVec3<f32> center;
		center.x = clusterMtx[0][3];
		center.y = mPosition.y;
		center.z = clusterMtx[2][3];
		request.unk0 = center;

		JGeometry::TVec3<f32> col0(clusterMtx[0][0], clusterMtx[1][0],
		                           clusterMtx[2][0]);
		JGeometry::TVec3<f32> col2(clusterMtx[0][2], clusterMtx[1][2],
		                           clusterMtx[2][2]);

		request.unkC  = PSVECMag((Vec*)&col0);
		request.unk10 = PSVECMag((Vec*)&col2);
		request.unkC *= mScaledBodyRadius;
		request.unk10 *= mScaledBodyRadius;
		request.unk1C = getShadowType();
		request.unk14 = mRotation.y;
		gpBindShadowManager->request(request, getActorType());
	}

	mBeak->testPerform(param_1, param_2);
	mLeftEye->testPerform(param_1, param_2);
	mRightEye->testPerform(param_1, param_2);
	mBody->testPerform(param_1, param_2);

	if (param_1 & 1) {
		f32 negStep               = -unk188;
		TBossGessoMtxCalc* mtxCalc = mMtxCalc;
		mtxCalc->unk50 = mtxCalc->unk50 + negStep;
		if (mtxCalc->unk50 < 0.0f)
			mtxCalc->unk50 = 0.0f;
		else if (mtxCalc->unk50 > 1.0f)
			mtxCalc->unk50 = 1.0f;
	}

	if (unk17C) {
		if (param_1 & 2) {
			MtxPtr bodyMtx = getMActor()->getModel()->getBaseTRMtx();
			PSMTXCopy(bodyMtx, unk178->getModel()->getBaseTRMtx());
			unk178->calcAnm();
		}
		if (param_1 & 0x200)
			gpPollution->stampModel(unk178->getModel());
	}

	if (param_1 & 2) {
		if (checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)) {
			for (int i = 0; i < TENTACLE_NUM; ++i) {
				if (mTentacles[i]->mState != 4)
					mTentacles[i]->getFirstNode()->setPosition(mPosition);
			}
		} else {
			static const int idxarray[] = { 2, 3, 5, 6 };
			for (int i = 0; i < TENTACLE_NUM; ++i) {
				if (mTentacles[i]->mState != 4) {
					JGeometry::TVec3<f32> jointTrans;
					if (getJointTransByIndex(idxarray[i], &jointTrans) >= 0)
						mTentacles[i]->getFirstNode()->setPosition(jointTrans);
				}
			}
		}
	}

	for (int i = 0; i < TENTACLE_NUM; ++i) {
		if (param_1 & 0x200) {
			if (getLatestNerve() == &TNerveBGBeakDamage::theNerve()) {
				mTentacles[i]->getUnk2C()->offMakeDL();
				SMS_AddDamageFogEffect(
				    mTentacles[i]->getUnk2C()->getModel()->getModelData(),
				    mPosition, param_2);
			} else {
				SMS_ResetDamageFogEffect(
				    mTentacles[i]->getUnk2C()->getModel()->getModelData());
			}
		}
		mTentacles[i]->testPerform(param_1, param_2);
	}

	if (param_1 & 2) {
		if (getMActor()->checkCurBckFromIndex(14)
		    || getMActor()->checkCurBckFromIndex(15)) {
			f32 len = lenFromToeToMario();
			if (gpMSound->gateCheck(0x215C))
				MSoundSESystem::MSoundSE::startSoundActorWithInfo(
				    0x215C, &mPosition, nullptr, len, 0, 0, nullptr, 0, 4);
		}
	}

	if (param_1 & 1) {
		if (mBeak->mHolder != nullptr && unk190.color.a == 0) {
			if (!mTentacles[1]->isThing() || !mTentacles[3]->isThing())
				gpMarDirector->getConsole()->startAppearBalloon(0xE0003, true);
		}
	}
}

TBossGessoManager::TBossGessoManager(const char* name)
    : TEnemyManager(name)
{
}

void TBossGessoManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "bgeso_body.bmd", 0x10300000, 0 },
		{ "bgeso_hand.bmd", 0x10240000, 0 },
		{ "bgeso_shand.bmd", 0x200000, 0 },
		{ "bgeso_dirty_white.bmd", 0x10220000, 0 },
		{ "bgeso_osenball.bmd", 0x10220000, 0 },
		{ "bgeso_osenball_white.bmd", 0x10220000, 0 },
		{ "bgeso_kolk.bmd", 0x10220000, 0 },
		{ "bgeso_kolk_break.bmd", 0x10220000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TBossGessoManager::initJParticle()
{
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_attack_a.jpa", 0x8c);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_attack_b.jpa", 0x8d);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_attack_c.jpa", 0x8e);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_attack_d.jpa", 0x8f);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_blur.jpa", 0x90);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_blur_f.jpa", 0x91);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_blur_j.jpa", 0x92);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_canon_a.jpa", 0x93);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_canon_b.jpa", 0x94);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_damage_a.jpa", 0x95);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_damage_b.jpa", 0x96);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_hit_a.jpa", 0x97);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_hit_b.jpa", 0x98);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_hit_c.jpa", 0x99);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_hitdown.jpa", 0x9a);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_jump.jpa", 0x9b);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_kizetsu.jpa", 0x9c);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_kizetsu_r.jpa", 0x9d);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_odanhit_a.jpa", 0x9e);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_odanhit_b.jpa", 0x9f);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_osen.jpa", 0xa0);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_ase.jpa", 0x138);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_namida.jpa", 0x139);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_kiseki.jpa", 0x13a);
	SMS_LoadParticle("/scene/bgeso/jpa/ms_boge_wash.jpa", 0x13b);
}

void TBossGessoManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TBossGessoParams("/enemy/bossgesso.prm");
	TEnemyManager::load(stream);
	initJParticle();
}

DEFINE_NERVE(TNerveBGWait, TLiveActor)
{
	TBossGesso* self = (TBossGesso*)spine->getBody();

	if (spine->getTime() == 0) {
		if (self->getHitPoints() == 1) {
			self->changeBck(27);
		} else if (self->getHitPoints() == 2) {
			self->changeBck(26);
		} else {
			self->changeBck(25);
		}

		self->setGoalPathMario();

		self->getMActor()->setBtpFromIndex(2);
		self->getMActor()->getFrameCtrl(3)->setFrame(0.0f);
		self->getMActor()->resetDL();
	}

	JGeometry::TVec3<f32> delta = SMS_GetMarioPos();
	delta -= self->mPosition;
	f32 len   = delta.length();
	f32 fVar2 = len > 800.0f ? 1.0f : 3000.0f / len;
	self->walkToCurPathNode(0.0f, fVar2 * self->getTurnSpeed(), 0.0f);

	return false;
}

DEFINE_NERVE(TNerveBGEyeDamage, TLiveActor)
{
	TBossGesso* self = (TBossGesso*)spine->getBody();

	if (spine->getTime() == 0) {
		self->changeBck(5);

		if (self->mBeak->mHolder == nullptr && self->mAttackMode != 2) {
			self->changeAttackMode(TBossGesso::ASTATE_UNISON);
			self->changeAllTentacleState(1);
		}

		self->getMActor()->setBtpFromIndex(1);
		J3DFrameCtrl* ctrl = self->getMActor()->getFrameCtrl(3);
		ctrl->setFrame(1.5f);
		ctrl->setRate(0.0f);
		self->getMActor()->resetDL();
	}

	if (self->getMActor()->curAnmEndsNext()) {
		self->getMActor()->setBtpFromIndex(2);
		J3DFrameCtrl* ctrl = self->getMActor()->getFrameCtrl(3);
		ctrl->setFrame(0.0f);
		self->getMActor()->resetDL();
		return true;
	}

	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x139, self->getModel()->getAnmMtx(7), 1, self);
	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x139, self->getModel()->getAnmMtx(4), 1, self);

	if (self->unk1AE == 0) {
		self->unk1AE = 0x78;
		if (gpMSound->gateCheck(0x2912))
			MSoundSESystem::MSoundSE::startSoundActor(0x2912, &self->mPosition,
			                                          0, nullptr, 0, 4);
	}

	if (self->unk190.color.a != 0) {
		self->unk190.color.a -= 1;
		self->unk194 = 5;
	}

	return false;
}

DEFINE_NERVE(TNerveBGBeakDamage, TLiveActor)
{
	TBossGesso* self = (TBossGesso*)spine->getBody();

	if (spine->getTime() == 0) {
		self->changeBck(7);
		self->getMActor()->setBtkFromIndex(0);
		for (int i = 0; i < 4; ++i) {
			TBGTentacle* tentacle = self->mTentacles[i];
			if (tentacle->mState != 5) {
				BOOL isThing;
				if (tentacle->mState == 6
				    || (u32)(tentacle->mState - 3) <= 1)
					isThing = TRUE;
				else
					isThing = FALSE;
				if (!isThing)
					tentacle->changeStateAndFixNodes(8);
			}
		}

		J3DFrameCtrl* ctrl4 = self->getMActor()->getFrameCtrl(4);
		ctrl4->setRate(1.0f);
		ctrl4->setFrame(0.0f);

		self->getMActor()->setBtpFromIndex(1);

		J3DFrameCtrl* ctrl3 = self->getMActor()->getFrameCtrl(3);
		ctrl3->setFrame(1.5f);
		ctrl3->setRate(0.0f);

		self->getMActor()->resetDL();

		if (gpMarDirector->mMap == 3 || gpMarDirector->mMap == 59) {
			MSBgm::stopBGM(0x8001000D, 10);
			MSMainProc::setBossNotDamagedFlag(false);
		}
	}

	if (spine->getTime() == 1)
		self->mCork->crush();

	if (spine->getTime() == 12) {
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x97, self->getModel()->getAnmMtx(27), 0, nullptr);
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x99, self->getModel()->getAnmMtx(27), 0, nullptr);
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x98, self->getModel()->getAnmMtx(27), 0, nullptr);
	}

	if (spine->getTime() == 18) {
		gpCameraShake->startShake(CAM_SHAKE_MODE_UNK12, 1.0f);
		self->rumblePad(1, self->mPosition);
	}

	if (spine->getTime() == 234) {
		gpCameraShake->startShake(CAM_SHAKE_MODE_UNK13, 1.0f);
		self->rumblePad(2, self->mPosition);
	}

	if (spine->getTime() == 510) {
		gpCameraShake->startShake(CAM_SHAKE_MODE_UNK14, 1.0f);
		self->rumblePad(1, self->mPosition);
	}

	if (spine->getTime() == 510 || spine->getTime() == 236) {
		JGeometry::TVec3<f32> local_28;
		self->getJointTransByIndex(47, &local_28);
		const TBGCheckData* data;
		local_28.y = gpMap->checkGround(local_28.x, local_28.y + 500.0f,
		                                local_28.z, &data);

		gpMarioParticleManager->emit(0x9A, &local_28, 0, nullptr);
	}

	if (spine->getTime() == 40) {
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x9C, self->getModel()->getAnmMtx(7), 0, nullptr);
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x9D, self->getModel()->getAnmMtx(4), 0, nullptr);
	}

	if (self->getMActor()->curAnmEndsNext()) {

		self->forceAllTentacleState(0);

		J3DFrameCtrl* ctrl4 = self->getMActor()->getFrameCtrl(4);
		ctrl4->setRate(0.0f);
		ctrl4->setFrame(0.0f);

		spine->pushAfterCurrent(&TNerveBGPollute::theNerve());
		if (gpMarDirector->mMap == 3 || gpMarDirector->mMap == 59)
			MSBgm::startBGM(0x8001002A);

		return true;
	}

	return false;
}

DEFINE_NERVE(TNerveBGTentacleDamage, TLiveActor)
{
	TBossGesso* self = (TBossGesso*)spine->getBody();

	if (spine->getTime() == 0) {
		self->changeBck(3);
	}

	if (spine->getTime() == 30 || spine->getTime() == 304) {
		JGeometry::TVec3<f32> local_28;
		self->getJointTransByIndex(47, &local_28);
		const TBGCheckData* data;
		local_28.y = gpMap->checkGround(local_28.x, local_28.y + 500.0f,
		                                local_28.z, &data);

		gpMarioParticleManager->emit(0x9A, &local_28, 0, nullptr);
	}

	if (spine->getTime() == 10) {
		gpCameraShake->startShake(CAM_SHAKE_MODE_UNK15, 1.0f);
		self->rumblePad(1, self->mPosition);
	}

	if (spine->getTime() == 304) {
		gpCameraShake->startShake(CAM_SHAKE_MODE_UNK14, 1.0f);
		self->rumblePad(1, self->mPosition);
	}

	if (self->getMActor()->curAnmEndsNext()) {
		if (!self->tentacleHeld())
			return true;

		spine->pushAfterCurrent(&TNerveBGTentacleDamage::theNerve());
		return true;
	}

	return false;
}

DEFINE_NERVE(TNerveBGTug, TLiveActor)
{
	TBossGesso* self = (TBossGesso*)spine->getBody();

	if (spine->getTime() == 0) {
		self->changeBck(5);
	}

	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x138, self->getModel()->getAnmMtx(47), 1, self);
	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x139, self->getModel()->getAnmMtx(7), 1, self);
	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x139, self->getModel()->getAnmMtx(4), 1, self);

	if (self->mBeak->mHolder != nullptr) {
		JGeometry::TVec3<f32> delta = SMS_GetMarioPos();
		delta -= self->mPosition;
		f32 lim = self->getSaveParam2()->mSLBeakLengthDamage.get();

		if (delta.length() >= lim) {
			self->getMActor()->setBtpFromIndex(1);

			J3DFrameCtrl* ctrl3 = self->getMActor()->getFrameCtrl(3);
			ctrl3->setFrame(1.5f);
			ctrl3->setRate(0.0f);

			self->getMActor()->resetDL();
			if (!self->getMActor()->checkCurBckFromIndex(9))
				self->changeBck(9);
		}
	}

	if (self->getMActor()->curAnmEndsNext()) {
		if (self->tentacleHeld())
			self->changeBck(5);
		else
			return true;
	}

	self->walkToCurPathNode(0.0f, self->getTurnSpeed() * 10.0f, 0.0f);
	return false;
}

DEFINE_NERVE(TNerveBGDie, TLiveActor)
{
	TBossGesso* self = (TBossGesso*)spine->getBody();

	if (spine->getTime() == 0) {
		self->changeBck(5);

		self->getMActor()->setBtpFromIndex(1);

		J3DFrameCtrl* ctrl3 = self->getMActor()->getFrameCtrl(3);
		ctrl3->setFrame(1.5f);
		ctrl3->setRate(0.0f);

		self->getMActor()->resetDL();

		if (gpMarDirector->mMap == 3 || gpMarDirector->mMap == 59) {
			MSBgm::stopTrackBGMs(7, 10);
			MSMainProc::setBossLivesFlag(false);
		} else if (gpMarDirector->mMap == 9) {
			MSBgm::stopTrackBGM(1, 10);
			MSMainProc::setBossLivesFlagOnlyFlag(false);
		}

		if (gpMarDirector->mMap == 9) {
			gpMarDirector->fireStartDemoCamera("bgeso_fall_camera3", nullptr,
			                                   -1, 0.0f, true, nullptr, 0,
			                                   nullptr, JDrama::TFlagT<u16>(0));
		} else {
			int isStage4 = gpMarDirector->unk7D == 4 ? 1 : 0;
			if (!isStage4) {
				gpMarDirector->fireStartDemoCamera(
				    "bgeso_fall_camera2", nullptr, -1, 0.0f, true, nullptr, 0,
				    nullptr, JDrama::TFlagT<u16>(0));
			} else {
				gpMarDirector->fireStartDemoCamera(
				    "bgeso_fall_camera", nullptr, -1, 0.0f, true, nullptr, 0,
				    nullptr, JDrama::TFlagT<u16>(0));
			}
		}

		if (gpMarDirector->mMap == 3 || gpMarDirector->mMap == 59) {
			gpItemManager->makeShineAppearWithDemo(
			    "シャイン（ボス用）", "ボスシャインカメラ", self->mPosition.x,
			    self->mPosition.y + 6000.0f, self->mPosition.z);
		}

		TNameKuriManager* nameKuriMgr
		    = JDrama::TNameRefGen::search<TNameKuriManager>(
		        "ナメクリマネージャー");
		if (nameKuriMgr)
			nameKuriMgr->killChildren();

		if (gpMSound->gateCheck(0x2953))
			MSoundSESystem::MSoundSE::startSoundActor(0x2953, &self->mPosition,
			                                          0, nullptr, 0, 4);
	}

	if (gpMarDirector->mMap == 9 && spine->getTime() >= 740
	    && spine->getTime() <= 750) {
		if (gpMSound->gateCheck(0x3008))
			MSoundSESystem::MSoundSE::startSoundActor(0x3008, &self->mPosition,
			                                          0, nullptr, 0, 4);

		if (spine->getTime() == 745) {
			self->unk1A4 = 1.0f;
			SMSRumbleMgr->start(8, &self->unk1A4);
		}
	}

	if (self->getMActor()->checkCurBckFromIndex(2)
	    || self->getMActor()->curAnmEndsNext()) {

		self->changeBck(6);
		self->changeAllTentacleState(8);

		JGeometry::TVec3<f32> local_24;
		local_24.x = self->mPosition.x;
		local_24.y = -5000.0f;
		local_24.z = self->mPosition.z + 7000.0f;

		self->unkF4.unk0 = nullptr;
		self->unkF4.unk4 = local_24;

		self->unk104.unk0 = nullptr;
		self->unk104.unk4 = local_24;

		self->unk114.clear();

		self->mVelocity
		    = self->calcVelocityToJumpToY(local_24, 0.0f, self->getGravityY());
		self->onLiveFlag(LIVE_FLAG_AIRBORNE);
		return false;
	}

	if (self->isReachedToGoal()) {
		self->mLinearVelocity = self->mVelocity
		    = JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f);
		self->onLiveFlag(LIVE_FLAG_UNK10);
	}

	if (self->isReachedToGoal() && gpMarDirector->unk124 != 3) {

		self->changeAllTentacleState(0);
		self->kill();

		THitActor* block = JDrama::TNameRefGen::search<THitActor>(
		    "マーレボスゲッソー用ブロック");

		if (block != nullptr) {
			block->receiveMessage(self, HIT_MESSAGE_ATTACK);
			MSBgm::stopTrackBGM(1, 10);
			MSBgm::setTrackVolume(0, 1.0f, 5, 0);
		}

		return true;
	}

	return false;
}

DEFINE_NERVE(TNerveBGPollute, TLiveActor)
{
	TBossGesso* self = (TBossGesso*)spine->getBody();

	if (spine->getTime() == 0) {
		self->changeBck(10);
		self->unk178->setBckFromIndex(4);
		self->unk17C = 1;
		self->changeAllTentacleState(0);
		self->getMActor()->setBtpFromIndex(2);

		J3DFrameCtrl* ctrl3 = self->getMActor()->getFrameCtrl(3);
		ctrl3->setFrame(0.0f);
		self->getMActor()->resetDL();
	}

	if (spine->getTime() == 90) {
		gpMarioParticleManager->emitAndBindToSRTMtxPtr(
		    0xA0, self->getModel()->getAnmMtx(27), 0, nullptr);
	}

	if (spine->getTime() == 226) {
		JGeometry::TVec3<f32> TStack_24;
		self->getJointTransByIndex(1, &TStack_24);
		gpMarioParticleManager->emit(0x9B, &TStack_24, 0, nullptr);
		self->unk190.color.a = 230;
	}

	if (spine->getTime() == 230) {
		gpCameraShake->startShake(CAM_SHAKE_MODE_UNK14, 1.0f);
		self->rumblePad(2, self->mPosition);
	}

	if (self->getMActor()->curAnmEndsNext()) {
		self->unk178->setBckFromIndex(-1);
		self->unk17C = 0;
		return true;
	}

	return false;
}

DEFINE_NERVE(TNerveBGPolDrop, TLiveActor)
{
	TBossGesso* self = (TBossGesso*)spine->getBody();

	if (spine->getTime() == 0) {
		self->changeBck(1);
		for (int i = 0; i < 4; ++i) {
			TBGTentacle* tentacle = self->mTentacles[i];
			if (tentacle->mState != 5) {
				BOOL isThing;
				if (tentacle->mState == 6
				    || (u32)(tentacle->mState - 3) <= 1)
					isThing = TRUE;
				else
					isThing = FALSE;
				if (!isThing)
					tentacle->changeStateAndFixNodes(0);
			}
		}
		self->getMActor()->setBtpFromIndex(2);

		J3DFrameCtrl* ctrl3 = self->getMActor()->getFrameCtrl(3);
		ctrl3->setFrame(0.0f);
		self->getMActor()->resetDL();
	}

	J3DFrameCtrl* ctrl0 = self->getMActor()->getFrameCtrl(0);
	if (83.0f < ctrl0->getFrame() && ctrl0->getFrame() < 87.0f) {
		self->launchPolDrop();
	}

	if (self->getMActor()->curAnmEndsNext()) {
		JGeometry::TVec3<f32> delta = SMS_GetMarioPos();
		delta -= self->mPosition;

		f32 shootRadius2 = self->getSaveParam2()->mSLShootRadius.value;
		shootRadius2 *= shootRadius2;
		f32 singleAttackLen2 = self->getSaveParam2()->mSLSingleAttackLen.get();
		singleAttackLen2 *= singleAttackLen2;

		if (self->unk195 < 3) {
			f32 len = delta.squared();
			if (singleAttackLen2 <= len && len < shootRadius2) {
				spine->pushAfterCurrent(&TNerveBGPolDrop::theNerve());
			}
		}

		return true;
	}

	self->walkToCurPathNode(0.0f, self->getTurnSpeed(), 0.0f);
	return false;
}

DEFINE_NERVE(TNerveBGRoll, TLiveActor)
{
	TBossGesso* self = (TBossGesso*)spine->getBody();
	MActor* mactor   = self->getMActor();

	if (self->unk1A1 == 0) {
		if (spine->getTime() == 0)
			self->changeBck(19);

		if (mactor->curAnmEndsNext()) {
			if (mactor->checkCurBckFromIndex(13)) {
				if (self->unk196 < 3) {
					self->changeBck(13);
					self->unk196 += 1;
				} else {
					self->changeBck(18);
					self->unk196 = 0;
				}
			} else {
				if (mactor->checkCurBckFromIndex(19))
					self->changeBck(13);
				else
					return true;
			}

			return false;
		}
	} else {
		if (spine->getTime() == 0)
			self->changeBck(17);

		if (mactor->curAnmEndsNext()) {
			if (mactor->checkCurBckFromIndex(15)) {
				if (self->unk196 < 3) {
					self->changeBck(15);
					self->unk196 += 1;
				} else {
					self->changeBck(16);
					self->unk196 = 0;
				}
			} else {
				if (mactor->checkCurBckFromIndex(17))
					self->changeBck(15);
				else
					return true;
			}
		}
	}

	return false;
}
