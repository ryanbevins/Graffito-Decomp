#include <Enemy/Bombhei.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/EffectObj.hpp>
#include <Enemy/Walker.hpp>
#include <Player/MarioAccess.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <MoveBG/MapObjBlock.hpp>
#include <MoveBG/ItemManager.hpp>
#include <Map/MapData.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <Camera/CameraShake.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <System/MarDirector.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/Particles.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/ObjModel.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>

// rogue includes needed for matching sinit & rodata
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

bool TBombHei::mSerialBomb = true;

const char* bombhei_bastable[] = {
	"/scene/bombhei/bas/downnejibomb_down1.bas",
	nullptr,
	nullptr,
	"/scene/bombhei/bas/nejibomb_land1.bas",
	nullptr,
	nullptr,
	"/scene/bombhei/bas/nejibomb_stop_down1.bas",
};

// ============= nerves =============

DEFINE_NERVE(TNerveBombHeiExplosion, TLiveActor)
{
	TBombHei* self = (TBombHei*)spine->getBody();

	if (spine->getTime() == 0) {
		self->unk1A0 = self->getBombParam()->mSLBombRange.get()
		             * self->mBodyScale / self->mAttackRadius;
		self->setDeadAnm();
		self->mLiveFlag |= 0x8;

		if (self->mHolder == (TTakeActor*)gpMarioAddress)
			self->sendAttackMsgToMario();

		self->unk164 = 0;

		u16 type = self->mGroundPlane->mBGType;
		if (type == 0x100 || (type >= 0x101 && type <= 0x105) || type == 0x4104) {
			if (TEffectBombColumWater* water
			    = (TEffectBombColumWater*)gpConductor->makeOneEnemyAppear(
			        self->mPosition, "エフェクト爆発水柱マネージャー", 1)) {
				JGeometry::TVec3<f32> scale(2.0f, 2.0f, 2.0f);
				water->generate(self->mPosition, scale);
			}
		}

		if (type == 0x701 || type == 0x4701 || type == 0x8701 || type == 0xc701) {
			if (TEffectColumSand* sand
			    = (TEffectColumSand*)gpConductor->makeOneEnemyAppear(
			        self->mPosition, "エフェクト砂柱マネージャー", 1)) {
				JGeometry::TVec3<f32> scale(0.7f, 0.7f, 0.7f);
				sand->generate(self->mPosition, scale);
			}
		}
	}

	if (self->unk190 < self->unk1A0) {
		self->unk190 *= 1.2f;
	} else {
		if (self->checkCurAnmEnd(0)) {
			self->unk64 |= 0x1;
			self->mLiveFlag |= 0x1;
			self->mLiveFlag |= 0x8;
			self->mLiveFlag &= ~0x10000;
			self->mHolder = nullptr;
			self->stopAnmSound();
			spine->reset();
			spine->setDefaultNext();
			spine->pushAfterCurrent(spine->getDefault());
			return true;
		}
	}

	self->expandCollision();
	return false;
}

DEFINE_NERVE(TNerveBombHeiThrown, TLiveActor)
{
	TBombHei* self = (TBombHei*)spine->getBody();

	if (spine->getTime() == 0) {
		TBombHeiSaveLoadParams* p = self->getBombParam();
		s16 angle                 = *gpMarioAngleY;
		f32 power                 = *gpMarioThrowPower;
		f32 rateXZ                = p->mSLThrownRateXZ.get();

		JGeometry::TVec3<f32> vel;
		vel.x = rateXZ * (power * JMASin(angle));
		vel.y = p->mSLThrownVY.get();
		vel.z = rateXZ * (power * JMACos(angle));
		self->mVelocity = vel;

		self->mPosition.y += 2.0f;
		self->mLiveFlag |= 0x80;
	}

	if (spine->getTime() == 0x78)
		self->unk64 &= ~0x1;

	if (!(self->mLiveFlag & 0x80)) {
		self->genEventCoin();
		self->mSpine->pushNerve(&TNerveBombHeiExplosion::theNerve());
		return true;
	}

	return false;
}

DEFINE_NERVE(TNerveBombHeiPickUp, TLiveActor)
{
	TBombHei* self = (TBombHei*)spine->getBody();
	if (spine->getTime() == 0 && self->unk164 == 0)
		return true;
	return false;
}

DEFINE_NERVE(TNerveBombHeiWaitExplosion, TLiveActor)
{
	TBombHei* self = (TBombHei*)spine->getBody();

	if (spine->getTime() == 0) {
		self->setBckAnm(6);
		self->unk164 = 1;
	}

	if (self->unk164 != 0) {
		if (self->getMActor()->getFrameCtrl(0)->checkPass(60.0f))
			self->getMActor()->setFrameRate(0.0f, 0);

		if (self->getMActor()->getFrameCtrl(0)->checkPass(10.0f)) {
			self->getMActor()->setFrameRate(SMSGetAnmFrameRate(), 3);
			if (self->getCurAnmFrameNo(3) >= 1.0f)
				self->getMActor()->setFrameRate(0.0f, 3);
		}
		return false;
	}

	self->getMActor()->setFrameRate(SMSGetAnmFrameRate(), 0);
	if (!self->getMActor()->checkCurAnmFromIndex(0, 3))
		self->getMActor()->setBtpFromIndex(0);

	if (self->checkCurAnmEnd(0)) {
		if (spine->getTime() > 0x96) {
			self->mSpine->pushNerve(&TNerveBombHeiExplosion::theNerve());
			return true;
		}
	}

	int frame = (int)self->getMActor()->getFrameCtrl(3)->getFrame();
	if (frame % 40 == 0) {
		if (gpMSound->gateCheck(0x2859))
			MSoundSESystem::MSoundSE::startSoundActor(0x2859, &self->mPosition, 0,
			                                          nullptr, 0, 4);
	}
	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x17f, self->getMActor()->getModel()->getAnmMtx(1), 1, self);

	return false;
}

DEFINE_NERVE(TNerveBombHeiWalkExplosion, TLiveActor)
{
	TBombHei* self = (TBombHei*)spine->getBody();

	if (spine->getTime() == 0) {
		self->setBckAnm(5);
		self->getMActor()->setBtpFromIndex(0);
	} else {
		if (self->checkCurAnmEnd(0)) {
			self->mSpine->pushNerve(&TNerveBombHeiExplosion::theNerve());
			return true;
		}
	}

	int frame = (int)self->getMActor()->getFrameCtrl(3)->getFrame();
	if (frame % 40 == 0) {
		if (gpMSound->gateCheck(0x2859))
			MSoundSESystem::MSoundSE::startSoundActor(0x2859, &self->mPosition, 0,
			                                          nullptr, 0, 4);
	}

	self->walkBehavior(2, 0.6f);
	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x17f, self->getMActor()->getModel()->getAnmMtx(1), 1, self);

	return false;
}

DEFINE_NERVE(TNerveBombHeiAttack, TLiveActor)
{
	TBombHei* self = (TBombHei*)spine->getBody();

	if (spine->getTime() == 0) {
		self->setWalkAnm();
		self->unk164 = 0;
		self->setGoalPathMario();
	}

	self->walkBehavior(2, 1.0f);
	return false;
}

DEFINE_NERVE(TNerveBombHeiGenerate, TLiveActor)
{
	TBombHei* self = (TBombHei*)spine->getBody();

	if (spine->getTime() == 0) {
		self->mMActor = self->mMActorKeeper->getMActor("nejibomb_model1.bmd");
		self->setBckAnm(2);
		self->getMActor()->setBtpFromIndex(1);
		self->getMActor()->setFrameRate(0.0f, 3);
	}

	if (self->mHolder != nullptr)
		self->getMActor()->setFrameRate(0.0f, 0);

	if (!(self->mLiveFlag & 0x80) && self->mHolder == nullptr) {
		if (self->mCurrentBckAnm == 2) {
			self->setBckAnm(3);
		} else if (self->checkCurAnmEnd(0)) {
			self->mSpine->pushNerve(&TNerveBombHeiAttack::theNerve());
			return true;
		}
	} else {
		if (self->mCurrentBckAnm != 2) {
			self->mMActor = self->mMActorKeeper->getMActor("nejibomb_model1.bmd");
			self->setBckAnm(2);
			self->getMActor()->setBtpFromIndex(1);
			self->getMActor()->setFrameRate(0.0f, 3);
		}
	}

	return false;
}

// ============= params =============

TBombHeiSaveLoadParams::TBombHeiSaveLoadParams(const char* path)
    : TWalkerEnemyParams(path)
    , PARAM_INIT(mSLBombTime, 1000)
    , PARAM_INIT(mSLBombRange, 300.0f)
    , PARAM_INIT(mSLThrownVY, 50.0f)
    , PARAM_INIT(mSLThrownRateXZ, 0.5f)
    , PARAM_INIT(mSLThrownGravityY, 1.5f)
    , PARAM_INIT(mSLShootVelocity, 12.0f)
{
	TParams::load(mPrmPath);
}

// ============= manager =============

TBombHeiManager::TBombHeiManager(const char* name)
    : TSmallEnemyManager(name)
    , unk60(0)
{
}

void TBombHeiManager::load(JSUMemoryInputStream& stream)
{
	TSmallEnemyManager::load(stream);
	unk38 = new TBombHeiSaveLoadParams("/enemy/bombhei.prm");
}

void TBombHeiManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "nejibomb_model1.bmd", 0x10230000, 0 },
		{ "downnejibomb_model1.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TSmallEnemy* TBombHeiManager::createEnemyInstance() { return new TBombHei; }

// ============= instance =============

TBombHei::TBombHei(const char* name)
    : TWalkerEnemy(name)
    , mParams(nullptr)
    , unk198(0)
    , unk19C(1)
    , unk1A4(0)
{
}

void TBombHei::init(TLiveManager* manager)
{
	TWalkerEnemy::init(manager);
	mActorType = 0x1000001e;
	unk150     = 0x11;
	mParams    = (TBombHeiSaveLoadParams*)getSaveParam();

	mSpine->initWith(&TNerveBombHeiGenerate::theNerve());

	if (mInstanceIndex == 0) {
		for (u8 i = 0; i < getModel()->getModelData()->getJointNum(); ++i) {
			;
		}
	}
}

const char** TBombHei::getBasNameTable() const { return bombhei_bastable; }

bool TBombHei::isDamageToCannon()
{
	if (mSpine->getCurrentNerve() == &TNerveBombHeiThrown::theNerve()
	    || mSpine->getCurrentNerve() == &TNerveBombHeiExplosion::theNerve()) {
		unk64 |= 0x1;
		return true;
	}
	return false;
}

void TBombHei::forceKill()
{
	if (mGroundPlane->mFlags & 0x10)
		return;

	u16 type = mGroundPlane->mBGType;
	if (!(type == 0x800 || (type >= 0x100 && type <= 0x105) || type == 0x4104))
		return;

	if (mLiveFlag & 0x80)
		return;
	if (mLiveFlag & 0x10)
		return;

	if (mSpine->getCurrentNerve() != &TNerveBombHeiExplosion::theNerve()) {
		mSpine->reset();
		mSpine->setNext(&TNerveBombHeiExplosion::theNerve());
		mSpine->pushAfterCurrent(mSpine->getDefault());
	}
	mLiveFlag |= 0x20000;
	mHitPoints = 1;
}

bool TBombHei::isCollidMove(THitActor* other)
{
	if (mSerialBomb) {
		if (other->mActorType == 0x1000001e) {
			if (((TBombHei*)other)->mSpine->getCurrentNerve()
			        == &TNerveBombHeiExplosion::theNerve()
			    && mSpine->getCurrentNerve()
			           != &TNerveBombHeiExplosion::theNerve()) {
				mSpine->pushNerve(&TNerveBombHeiExplosion::theNerve());
			}
		}
	}

	if (other->mActorType == 0x80000013) {
		if (mSpine->getCurrentNerve() == &TNerveBombHeiExplosion::theNerve())
			other->receiveMessage((THitActor*)this, 0);

		if (mSpine->getCurrentNerve() == &TNerveBombHeiThrown::theNerve())
			mSpine->pushNerve(&TNerveBombHeiExplosion::theNerve());
	}

	return true;
}

void TBombHei::moveObject()
{
	TWalkerEnemy::moveObject();

	if (mSpine->getCurrentNerve() == &TNerveBombHeiThrown::theNerve())
		return;
	if (mSpine->getCurrentNerve() == &TNerveBombHeiExplosion::theNerve())
		return;
	if (mSpine->getCurrentNerve() == &TNerveBombHeiGenerate::theNerve())
		return;
	if (mSpine->getCurrentNerve() == &TNerveSmallEnemyChange::theNerve())
		return;

	unk198++;
	if (unk198 > mParams->mSLBombTime.get()) {
		unk164 = 0;
		unk198 = 0;
		if (mSpine->getCurrentNerve() != &TNerveBombHeiWaitExplosion::theNerve())
			mSpine->pushNerve(&TNerveBombHeiWalkExplosion::theNerve());
	}
}

void TBombHei::walkBehavior(int param_1, f32 param_2)
{
	if (SMSGetMSound()->gateCheck(0x2068))
		MSoundSESystem::MSoundSE::startSoundActor(0x2068, &mPosition, 0, nullptr,
		                                          0, 4);
	TWalkerEnemy::walkBehavior(param_1, param_2);
}

f32 TBombHei::getGravityY() const
{
	if (mSpine->getCurrentNerve() == &TNerveBombHeiThrown::theNerve())
		return mParams->mSLThrownGravityY.get();
	return mGravity;
}

void TBombHei::reset()
{
	TWalkerEnemy::reset();
	unk198  = 0;
	unk1A4  = 0;
	unk164  = 0;
	unk19C  = 1;
	mMActor = mMActorKeeper->getMActor("nejibomb_model1.bmd");
}

void TBombHei::behaveToRelease()
{
	if (unk164 == 0
	    && mSpine->getCurrentNerve() != &TNerveBombHeiWalkExplosion::theNerve())
		return;
	if (mSpine->getCurrentNerve() != &TNerveBombHeiThrown::theNerve())
		mSpine->pushNerve(&TNerveBombHeiThrown::theNerve());
}

void TBombHei::behaveToTaken(THitActor* sender)
{
	if (mSpine->getCurrentNerve() == &TNerveBombHeiPickUp::theNerve())
		return;
	if (sender->mActorType == 0x80000001)
		unk1A4 = 1;
	mSpine->pushNerve(&TNerveBombHeiPickUp::theNerve());
}

void TBombHei::attackToMario()
{
	if (mSpine->getCurrentNerve() == &TNerveBombHeiExplosion::theNerve())
		SMS_SendMessageToMario(this, 0xa);
}

void TBombHei::calcRootMatrix()
{
	TSpineEnemy::calcRootMatrix();

	if (gpMarDirector->unk4C & 0xf) {
		mLiveFlag |= 0x1;
		unk64 |= 0x1;
	}

	if (mCurrentBckAnm == 0) {
		if (getMActor()->getFrameCtrl(0)->checkPass(2.0f)) {
			if (TEffectExplosion* effect
			    = (TEffectExplosion*)gpConductor->makeOneEnemyAppear(
			        mPosition, "エフェクト爆発マネージャー", 1)) {
				effect->generate(mPosition, mScaling);
			}
		}
	}
}

void TBombHei::setDeadAnm()
{
	mMActor         = mMActorKeeper->getMActor("downnejibomb_model1.bmd");
	volatile f32 mn = 0.0f;
	volatile f32 mx = 360.0f;
	f32 range       = mx - mn;
	f32 rot         = range * MsRandF();
	mRotation.y     = mn + rot;
	unk19C      = 0;
	setBckAnm(0);

	gpCameraShake->startShake((EnumCamShakeMode)6, 1.0f);
	SMSRumbleMgr->start(0x15, 5, (f32*)nullptr);
}

void TBombHei::setFreezeAnm() { setBckAnm(1); }

void TBombHei::setWalkAnm() { setBckAnm(4); }

void TBombHei::genEventCoin()
{
	if (!unk1A4)
		return;

	TBombHeiManager* mgr = (TBombHeiManager*)mManager;
	if (mgr->unk60 >= 0x14)
		return;
	mgr->unk60++;

	if (TMapObjBase* obj = gpItemManager->makeObjAppear(
	        mPosition.x, mPosition.y, mPosition.z, 0x2000000e, true)) {
		TLiveActor* coin  = (TLiveActor*)obj;
		coin->mPosition.y = mPosition.y;

		JGeometry::TVec3<f32> dir = *gpMarioPos;
		dir.sub(mPosition);
		MsVECNormalize(&dir, &dir);

		coin->mVelocity.x = 20.0f * dir.x;
		coin->mVelocity.z = 20.0f * dir.z;
		coin->mLiveFlag &= ~0x10;
	}
}

void TBombHei::kill()
{
	if (mLiveFlag & 0x1)
		return;

	mHitPoints = 1;
	if (mSpine->getCurrentNerve() != &TNerveBombHeiExplosion::theNerve()) {
		mSpine->reset();
		mSpine->setNext(&TNerveBombHeiExplosion::theNerve());
		mSpine->pushAfterCurrent(&TNerveBombHeiExplosion::theNerve());
	}
	mLiveFlag |= 0x40;
}

bool TBombHei::isHitValid(u32 message)
{
	if (message == 0xb) {
		mLiveFlag |= 0x1;
		unk64 |= 0x1;
		genEventCoin();
		return false;
	}
	return false;
}

void TBombHei::changeOut()
{
	if (gpMSound->gateCheck(0x293d))
		MSoundSESystem::MSoundSE::startSoundActor(0x293d, &mPosition, 0, nullptr,
		                                          0, 4);

	mLiveFlag |= 0x1;
	genEventCoin();
	unk64 |= 0x1;

	mPosition = ((TLiveActor*)mJuiceBlock)->mPosition;
	gpMarioParticleManager->emitAndBindToPosPtr(0xcd, &mPosition, 0, nullptr);

	mMActor->setFrameRate(SMSGetAnmFrameRate(), 0);
	mJuiceBlock->kill();
	mJuiceBlock = nullptr;
}

void TBombHei::behaveToWater(THitActor*)
{
	if (mCurrentBckAnm == 4 || mCurrentBckAnm == 3) {
		if (mHitPoints == 0)
			mSpine->pushNerve(&TNerveBombHeiWaitExplosion::theNerve());
		mSprayedByWaterCooldown = 0x14;
	}
}

void TBombHei::setMActorAndKeeper()
{
	mMActorKeeper = new TMActorKeeper(mManager, 2);
	mMActor       = mMActorKeeper->createMActor("nejibomb_model1.bmd", 0);
	mMActorKeeper->createMActor("downnejibomb_model1.bmd", 3);
}

void TBombHei::setAfterDeadEffect() { }

bool TBombHei::doKeepDistance() { return unk19C; }
