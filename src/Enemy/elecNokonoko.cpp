#include <Enemy/ElecNokonoko.hpp>
#include <Enemy/Conductor.hpp>
#include <Camera/Camera.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/ObjModel.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/MarDirector.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionData.hpp>
#include <Map/MapData.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MoveBG/MapObjBase.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/MarioMain.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <MarioUtil/ShadowUtil.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <Strategic/Strategy.hpp>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

// ------------------------------------------------------------------ nerves

DEFINE_NERVE(TNerveElecCarapaceReturn, TLiveActor)
{
	TElecCarapace* self = (TElecCarapace*)spine->getBody();
	TElecNokonoko* host = (TElecNokonoko*)self->unk16C;

	if (spine->getTime() == 0) {
		JGeometry::TVec3<f32> d = host->getPosition();
		self->unk18C            = (d.x - self->mPosition.x) * 0.015625f;
		self->unk190            = (d.y - self->mPosition.y) * 0.015625f;
		self->unk194            = (d.z - self->mPosition.z) * 0.015625f;

		if (host->mCurrentBckAnm == 8)
			host->setBckAnm(0);
	}

	if (spine->getTime() < 20) {
		if (host->mCurrentBckAnm == 8)
			host->setBckAnm(0);
	}

	bool nearHost = false;
	if (host->mSpine->getCurrentNerve()
	    == &TNerveElecNokonokoFreeze::theNerve()) {
		JGeometry::TVec3<f32> v
		    = host->getPosition() - host->mCarapace->getPosition();
		if (PSVECMag((Vec*)&v) < 200.0f)
			nearHost = true;
	}

	if (nearHost) {
		host->onLiveFlag(0x10000);
		host->kill();
		gpMarioParticleManager->emitAndBindToPosPtr(
		    0xCD, (const JGeometry::TVec3<f32>*)&self->mPosition, 0, nullptr);
	}

	self->unk188 += host->mSaveParams->mSLCarapaceSpinSpeed.get();
	if (self->unk188 > 360.0f)
		self->unk188 -= 360.0f;

	self->mPosition.x += self->unk18C;
	self->mPosition.y += self->unk190;
	self->mPosition.z += self->unk194;

	JGeometry::TVec3<f32> hp = host->mPosition;

	if (self->unk18C > 0.0f) {
		if (self->mPosition.x > hp.x)
			self->mPosition.x = hp.x;
	} else {
		if (self->mPosition.x < hp.x)
			self->mPosition.x = hp.x;
	}

	if (self->unk190 > 0.0f) {
		if (self->mPosition.y > hp.y)
			self->mPosition.y = hp.y;
	} else {
		if (self->mPosition.y < hp.y)
			self->mPosition.y = hp.y;
	}

	if (self->unk194 > 0.0f) {
		if (self->mPosition.z > hp.z)
			self->mPosition.z = hp.z;
	} else {
		if (self->mPosition.z < hp.z)
			self->mPosition.z = hp.z;
	}

	if (self->mLiveFlag & 1) {
		self->mScaling.y *= 0.8f;
		if (self->mScaling.y < 0.01f)
			self->kill();
	}

	return false;
}

DEFINE_NERVE(TNerveElecCarapaceWait, TLiveActor)
{
	return spine->getTime() > 60 ? TRUE : FALSE;
}

DEFINE_NERVE(TNerveElecCarapaceMove, TLiveActor)
{
	TElecCarapace* self = (TElecCarapace*)spine->getBody();
	TElecNokonoko* host = (TElecNokonoko*)self->unk16C;

	TElecNokonokoSaveLoadParams* params = host->mSaveParams;
	f32 spinSpeed                       = params->mSLCarapaceSpinSpeed.get();
	f32 speed                           = params->mSLCarapaceSpeed.get();
	f32 turnSpeed                       = params->mSLCarapaceTurnSpeed.get();

	if (self->unk175) {
		self->walkToCurPathNode(speed, turnSpeed, 0.0f);
	} else {
		self->zigzagToCurPathNode(speed, turnSpeed, self->unk178,
		                          self->unk17C);
	}

	self->unk188 += spinSpeed;
	if (self->unk188 > 360.0f)
		self->unk188 -= 360.0f;

	if (self->unk184) {
		f32 collectRange = 64.0f * host->mSaveParams->mSLCarapaceSpeed.get();

		JGeometry::TVec3<f32> diff = self->unk104.getPoint();
		diff.sub(self->mPosition);
		f32 dist = JGeometry::TUtil<f32>::sqrt(
		    diff.z * diff.z + (diff.x * diff.x + diff.y * diff.y));

		if (dist < collectRange) {
			if (host->mSpine->getCurrentNerve()
			        != &TNerveElecNokonokoCollect::theNerve()
			    && host->mSpine->getCurrentNerve()
			           != &TNerveSmallEnemyDie::theNerve()
			    && host->mSpine->getCurrentNerve()
			           != &TNerveElecNokonokoFreeze::theNerve()
			    && host->mSpine->getCurrentNerve()
			           != &TNerveElecNokonokoCollect::theNerve()) {
				host->mSpine->setNext(
				    &TNerveElecNokonokoCollect::theNerve());
			}

			spine->pushAfterCurrent(&TNerveElecCarapaceReturn::theNerve());
			return true;
		}
	}

	JGeometry::TVec3<f32> diff = self->unk104.getPoint();
	diff.x -= self->mPosition.x;
	diff.y -= self->mPosition.y;
	diff.z -= self->mPosition.z;
	diff.y = 0.0f;

	if (self->unk176 == 0) {
		if (MsVECMag2((Vec*)&diff) < 100.0f) {
			if (self->unk184) {
				spine->pushAfterCurrent(
				    &TNerveElecCarapaceReturn::theNerve());
				return true;
			}

			self->unk184 = 1;
			self->setGoalPath(TPathNode(self->unk16C->getPosition()));
		}
	}

	return false;
}

DEFINE_NERVE(TNerveElecNokonokoFreeze, TLiveActor)
{
	TElecNokonoko* self = (TElecNokonoko*)spine->getBody();

	if (spine->getTime() == 0) {
		bool noCarapace = self->unk1A4 == 0 ? true : false;
		if (noCarapace) {
			self->setBckAnm(3);
			gpMarioParticleManager->emitAndBindToMtxPtr(
			    0xCA, (MtxPtr)self->mMActor->unk4->mNodeMatrices, 0, nullptr);
		} else {
			self->setBckAnm(7);
		}
	}

	if (self->isBckAnm(3)) {
		if (self->getCurAnmFrameNo(0) < 25.0f) {
			Mtx* nodeMtx
			    = (Mtx*)((u8*)self->mMActor->unk4->mNodeMatrices + 0x180);
			((JGeometry::TVec3<f32>*)&self->unk1A8)
			    ->set((*nodeMtx)[0][3], (*nodeMtx)[1][3], (*nodeMtx)[2][3]);
			JPABaseEmitter* emitter
			    = gpMarioParticleManager->emitAndBindToPosPtr(
			        0x17E, (JGeometry::TVec3<f32>*)&self->unk1A8, 1, self);
			if (emitter) {
				emitter->unk154.x = self->mScaling.x;
				emitter->unk154.y = self->mScaling.y;
				emitter->unk154.z = self->mScaling.z;
				emitter->unk174.x = self->mScaling.x;
				emitter->unk174.y = self->mScaling.y;
				emitter->unk174.z = self->mScaling.z;
			}
		}
	}

	if (self->checkCurAnmEnd(0)) {
		if (self->isBckAnm(7)) {
			self->setBckAnm(6);
		} else if (self->isBckAnm(6)) {
			bool wasReady = self->unk165;
			if (self->unk165) {
				self->unk165 = false;
			}
			if (!wasReady) {
				bool noCarapace = self->unk1A4 == 0 ? true : false;
				if (noCarapace) {
					self->setBckAnm(5);
				} else {
					self->setBckAnm(6);
				}
			} else {
				self->setBckAnm(6);
			}
		} else {
			return true;
		}
	}

	if (spine->getTime() > 0x190) {
		spine->reset();
		spine->setDefaultNext();
		spine->pushAfterCurrent(&TNerveElecNokonokoRebirth::theNerve());
		return true;
	}

	return false;
}

DEFINE_NERVE(TNerveElecNokonokoCollect, TLiveActor)
{
	TElecNokonoko* self = (TElecNokonoko*)spine->getBody();

	if (spine->getTime() == 0) {
		if (!self->isBckAnm(0)) {
			self->setBckAnm(8);
		}

		TPathNode node((THitActor*)self->mCarapace);
		if (self->mCarapace) {
			node.unk4.set(self->mCarapace->mPosition.x,
			              self->mCarapace->mPosition.y,
			              self->mCarapace->mPosition.z);
		}
		self->unkF4  = node;
		self->unk104 = node;
		self->unk114.clear();
	}

	if ((self->mCarapace->mSpine->getCurrentNerve()
	     != &TNerveElecCarapaceWait::theNerve())
	    ? true
	    : false) {
		self->getMActor()->setFrameRate(SMSGetAnmFrameRate(), 0);
	} else {
		self->getMActor()->setFrameRate(0.0f, 0);
	}

	if (self->isBckAnm(0)) {
		int frame = (int)self->getCurAnmFrameNo(0);
		if (frame > 20) {
			self->mCarapace->onHitFlag(1);
		}
		if (frame > 32) {
			self->unk1A4 = 0;
			self->mCarapace->kill();
		}
		if (self->checkCurAnmEnd(0)) {
			return true;
		}
	} else {
		if (spine->getTime() > 800 || self->mCarapace->checkLiveFlag(1)) {
			spine->pushAfterCurrent(&TNerveElecNokonokoRebirth::theNerve());
			return true;
		}
	}

	self->walkToCurPathNode(0.0f, self->mTurnSpeed, 0.0f);
	return false;
}

DEFINE_NERVE(TNerveElecNokonokoAttack, TLiveActor)
{
	TElecNokonoko* self = (TElecNokonoko*)spine->getBody();

	if (spine->getTime() == 0 || !self->isBckAnm(3)) {
		self->setBckAnm(3);
	}

	return self->checkCurAnmEnd(0) ? TRUE : FALSE;
}

DEFINE_NERVE(TNerveElecNokonokoRebirth, TLiveActor)
{
	TElecNokonoko* self = (TElecNokonoko*)spine->getBody();

	if (spine->getTime() == 0) {
		self->setBckAnm(13);
		self->unk1A4 = 0;
		gpMarioParticleManager->emitAndBindToPosPtr(
		    0xCD, &self->mCarapace->mPosition, 0, nullptr);
		self->mCarapace->kill();
	}

	if (self->mMActor->getFrameCtrl(0)->checkPass(88.0f)) {
		gpMarioParticleManager->emitAndBindToPosPtr(
		    0xCD, &self->mPosition, 0, nullptr);
	}

	if (self->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveWalkerGraphWander::theNerve());
		return true;
	}

	return false;
}

DEFINE_NERVE(TNerveElecNokonokoTurn, TLiveActor)
{
	TElecNokonoko* self = (TElecNokonoko*)spine->getBody();

	if (spine->getTime() == 0) {
		self->setBckAnm(16);
		self->setGoalPathMario();
	}

	if (self->isBckAnm(15)) {
		if (MsIsInSight(self->mPosition, self->mRotation.y, *gpMarioPos,
		                ((TSmallEnemyParams*)self->getSaveParam())
		                    ->getSLSearchLength(),
		                60.0f, 0.0f)) {
			self->setBckAnm(14);
		}
	}

	if (self->checkCurAnmEnd(0)) {
		if (self->isBckAnm(16)) {
			self->setBckAnm(15);
		} else if (self->isBckAnm(14)) {
			return true;
		}
	}

	f32 zero = 0.0f;
	if (zero == self->mPosition.x - self->mCarapace->mPosition.x
	    && zero == self->mPosition.z - self->mCarapace->mPosition.z) {
		self->mPosition.x += 1.0f;
	}

	self->walkToCurPathNode(0.0f, self->mTurnSpeed, 0.0f);

	return spine->getTime() > 500 ? TRUE : FALSE;
}

DEFINE_NERVE(TNerveElecNokonokoShoot, TLiveActor)
{
	TElecNokonoko* self = (TElecNokonoko*)spine->getBody();

	if (spine->getTime() == 0) {
		self->setBckAnm(9);
	}

	if (self->isBckAnm(9)) {
		if (self->checkCurAnmEnd(0)) {
			self->setBckAnm(12);
			self->unk198 = 0;
		}
	} else if (self->isBckAnm(12)) {
		if (self->mMActor->getFrameCtrl(0)->checkPass(60.0f)) {
			self->mCarapace->appear();
		}

		if (self->getCurAnmFrameNo(0) < 62.0f) {
			self->walkToCurPathNode(0.0f, self->mTurnSpeed, 0.0f);
		}

		if (self->mMActor->getFrameCtrl(0)->checkPass(62.0f)) {
			self->mCarapace->shoot();
			self->unk1A4 = 1;
		}

		if (self->checkCurAnmEnd(0)) {
			spine->pushAfterCurrent(&TNerveElecNokonokoCollect::theNerve());
			return true;
		}
	}

	return false;
}

// --------------------------------------------------------------- bas table

static const char* dennoko_bastable[] = {
	"/scene/dennoko/bas/dennoko_catch1.bas",        // 0
	"/scene/dennoko/bas/dennoko_down1.bas",         // 1
	"/scene/dennoko/bas/dennoko_elec_down1.bas",    // 2
	"/scene/dennoko/bas/dennoko_hit1.bas",          // 3
	nullptr,                                        // 4
	nullptr,                                        // 5
	"/scene/dennoko/bas/dennoko_mogaki1_loop.bas",  // 6
	"/scene/dennoko/bas/dennoko_mogaki1_start.bas", // 7
	nullptr,                                        // 8
	nullptr,                                        // 9
	"/scene/dennoko/bas/dennoko_run1_loop.bas",     // 10
	nullptr,                                        // 11
	"/scene/dennoko/bas/dennoko_shoot1.bas",        // 12
	"/scene/dennoko/bas/dennoko_supply1.bas",       // 13
	nullptr,                                        // 14
	"/scene/dennoko/bas/dennoko_turn1_loop.bas",    // 15
	nullptr,                                        // 16
	nullptr,                                        // 17
};

const char** TElecNokonoko::getBasNameTable() const { return dennoko_bastable; }

u8 TElecNokonoko::mReflectSw = 1;

// ------------------------------------------------------ TElecNokonoko: anims

void TElecNokonoko::setDeadAnm() { setBckAnm(1); }

void TElecNokonoko::setWaitAnm()
{
	unk198 = 0;
	setBckAnm(0x11);
}

void TElecNokonoko::setWalkAnm()
{
	if (!isBckAnm(0xa)) {
		setBckAnm(0xb);
	}
}

void TElecNokonoko::setRunAnm()
{
	if (!isBckAnm(0xa)) {
		setBckAnm(0xb);
	}
}

void TElecNokonoko::setMeltAnm()
{
	setBckAnm(2);
	mLiveFlag |= 8;

	JGeometry::TVec3<f32> zeroVec(0.0f, 0.0f, 0.0f);
	unk18C = 3;
	mCarapace->mVelocity = zeroVec;

	JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0xCA, (MtxPtr)mMActor->getModel()->mNodeMatrices, 0, nullptr);
	if (emitter) {
		emitter->unk154.x = mScaling.x;
		emitter->unk154.y = mScaling.y;
		emitter->unk154.z = mScaling.z;
		emitter->unk174.x = mScaling.x;
		emitter->unk174.y = mScaling.y;
		emitter->unk174.z = mScaling.z;
	}
}

// ----------------------------------------------------- TElecNokonoko: stubs

void TElecNokonoko::load(JSUMemoryInputStream& stream)
{
	TSmallEnemy::load(stream);
	reset();
}

BOOL TElecNokonoko::receiveMessage(THitActor* sender, u32 message)
{
	if (message == HIT_MESSAGE_UNKD || message == HIT_MESSAGE_UNKB) {
		onLiveFlag(LIVE_FLAG_DEAD);
		kill();
	}

	if (message == HIT_MESSAGE_TAKE && mHolder == nullptr) {
		onHitFlag(HIT_FLAG_NO_COLLISION);
		mHolder = (TTakeActor*)sender;
		return TRUE;
	}

	if ((message == HIT_MESSAGE_UNK6 || message == HIT_MESSAGE_UNK7)
	    && mHolder == sender) {
		mHolder = nullptr;
		return TRUE;
	}

	if (message == HIT_MESSAGE_TRAMPLE) {
		if (unk1A4 == 1) {
			mHitPoints = 1;
			kill();
			return TRUE;
		}
		SMS_SendMessageToMario(this, 9);
		return FALSE;
	}

	if (message == HIT_MESSAGE_SPRAYED_BY_WATER) {
		if (!changeByJuice()) {
			behaveToWater(sender);
		} else {
			mCarapace->kill();
		}
		return TRUE;
	}

	return FALSE;
}

void TElecNokonoko::init(TLiveManager* manager)
{
	TWalkerEnemy::init(manager);

	mActorType = 0x1000000A;
	unk150     = 0x11;

	mSaveParams = (TElecNokonokoSaveLoadParams*)getSaveParam();

	mCarapace = new TElecCarapace(
	    "\x83\x6D\x83\x52\x83\x6D\x83\x52\x8D\x62\x97\x85");

	mSpine->initWith(&TNerveWalkerGraphWander::theNerve());

	mCarapace->loadInit(this, "koura_model1.bmd");

	MActor* carapaceMActor = mCarapace->mMActor;
	carapaceMActor->unk4->getModelData()->setMaterialTable(
	    ((TElecNokonokoManager*)getManager())->getMaterialTable(),
	    (J3DMaterialCopyFlag)3);
	carapaceMActor->initDL();
	carapaceMActor->unk4->lock();

	TMsRange<s32> readyTimerRange(0, 300);
	mReadyTimer = readyTimerRange.rand();

	offHitFlag(HIT_FLAG_NO_COLLISION);
}

void TElecNokonoko::calcRootMatrix()
{
	TSpineEnemy::calcRootMatrix();

	bool isReady = unk1A4 == 0;
	if (isReady
	    && mSpine->getCurrentNerve() != &TNerveElecNokonokoFreeze::theNerve()
	    && mSpine->getCurrentNerve() != &TNerveSmallEnemyDie::theNerve()
	    && mSpine->getCurrentNerve()
	           != &TNerveElecNokonokoCollect::theNerve()) {
		if (gpMSound->gateCheck(0x20BB)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x20BB, (const Vec*)&mPosition, 0, nullptr, 0, 4);
		}

		JPABaseEmitter* emitter1 = gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x17A, (MtxPtr)((u8*)mMActor->unk4->mNodeMatrices + 0x150), 1, this);
		if (emitter1) {
			emitter1->setScale(getScaling());
		}

		JPABaseEmitter* emitter2 = gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x17B, (MtxPtr)((u8*)mMActor->unk4->mNodeMatrices + 0x150), 1, this);
		if (emitter2) {
			emitter2->setScale(getScaling());
		}

		JPABaseEmitter* emitter3 = gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x17C, (MtxPtr)((u8*)mMActor->unk4->mNodeMatrices + 0x150), 1, this);
		if (emitter3) {
			emitter3->setScale(getScaling());
		}
	}

	if (mCurrentBckAnm == 2) {
		JPABaseEmitter* emitter4 = gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x17D, (MtxPtr)mMActor->unk4->mNodeMatrices, 1, this);
		if (emitter4) {
			emitter4->setScale(getScaling());
		}

		Mtx* nodeMtx
		    = (Mtx*)((u8*)mMActor->unk4->mNodeMatrices + 0x180);
		((JGeometry::TVec3<f32>*)&unk1A8)
		    ->set((*nodeMtx)[0][3], (*nodeMtx)[1][3], (*nodeMtx)[2][3]);
		JPABaseEmitter* emitter5 = gpMarioParticleManager->emitAndBindToPosPtr(
		    0x17E, (JGeometry::TVec3<f32>*)&unk1A8, 1, this);
		if (emitter5) {
			emitter5->setScale(getScaling());
		}

		if (mMActor->getFrameCtrl(0)->checkPass(72.0f)) {
			JPABaseEmitter* emitter6
			    = gpMarioParticleManager->emitAndBindToPosPtr(
			        0x17F, (JGeometry::TVec3<f32>*)&unk1A8, 1, this);
			if (emitter6) {
				emitter6->setScale(getScaling());
			}
		}
	}
}

void TElecNokonoko::moveObject()
{
	TWalkerEnemy::moveObject();

	if (isBckAnm(0xb) && checkCurAnmEnd(0)) {
		setBckAnm(0xa);
	}
}

void TElecNokonoko::genRandomItem()
{
	mCarapace->kill();
	gpMarioParticleManager->emitAndBindToPosPtr(0xCD, &mPosition, 0, nullptr);
	TSmallEnemy::genRandomItem();

	if (mLiveFlag & LIVE_FLAG_UNK10000) {
		if (TMapObjBase* mapObj = gpItemManager->makeObjAppear(
		        mCarapace->mPosition.x, mCarapace->mPosition.y,
		        mCarapace->mPosition.z, 0x2000000e, true)) {
			mapObj->mVelocity.set(0.0f, 20.0f, 0.0f);
			mapObj->offLiveFlag(LIVE_FLAG_UNK10);
		}
	}
}

void TElecNokonoko::behaveToWater(THitActor* water)
{
	if (isBckAnm(0xc) && getCurAnmFrameNo(0) > 58.0f) {
		return;
	}

	if (mSpine->getCurrentNerve() == &TNerveSmallEnemyDie::theNerve()) {
		return;
	}

	if (isBckAnm(0) && unk1A4 == 0) {
		return;
	}

	unk165                  = 1;
	mSprayedByWaterCooldown = 0;
	if (mSpine->getCurrentNerve() == &TNerveElecNokonokoFreeze::theNerve()) {
		return;
	}
	mSpine->pushNerve(&TNerveElecNokonokoFreeze::theNerve());
}

void TElecNokonoko::attackToMario()
{
	if (mSpine->getCurrentNerve() == &TNerveSmallEnemyDie::theNerve()) {
		return;
	}

	TSmallEnemy::attackToMario();

	if (mSpine->getCurrentNerve() == &TNerveElecNokonokoAttack::theNerve()) {
		return;
	}
	if (mSpine->getCurrentNerve() == &TNerveElecNokonokoCollect::theNerve()) {
		return;
	}
	if (mSpine->getCurrentNerve() == &TNerveElecNokonokoShoot::theNerve()) {
		return;
	}

	bool noCarapace = unk1A4 == 0 ? true : false;
	if (!noCarapace) {
		return;
	}

	mSpine->pushNerve(&TNerveElecNokonokoAttack::theNerve());
}

void TElecNokonoko::setMActorAndKeeper()
{
	mMActorKeeper = new TMActorKeeper(getManager(), 1);
	mMActor       = getActorKeeper()->createMActor("dennoko_model1.bmd", 3);
	MActor* actor = mMActor;
	actor->unk4->getModelData()->setMaterialTable(
	    ((TElecNokonokoManager*)getManager())->getMaterialTable(),
	    (J3DMaterialCopyFlag)3);
	actor->initDL();
	actor->unk4->lock();
}

void TElecNokonoko::sendAttackMsgToMario()
{
	if (unk1A4 == 0) {
		SMS_SendMessageToMario(this, 9);
	} else {
		SMS_SendMessageToMario(this, 0xe);
	}
}

void TElecNokonoko::behaveToFindMario()
{
	mSpine->pushAfterCurrent(&TNerveWalkerGraphWander::theNerve());
	mSpine->pushAfterCurrent(&TNerveWalkerAttack::theNerve());
	mSpine->pushAfterCurrent(&TNerveElecNokonokoTurn::theNerve());

	setGoalPathMario();
}

static f32 calcDist(const JGeometry::TVec3<f32>& a,
                    const JGeometry::TVec3<f32>& b)
{
	JGeometry::TVec3<f32> diff = a;
	diff.sub(b);
	return JGeometry::TUtil<f32>::sqrt(
	    diff.z * diff.z + (diff.x * diff.x + diff.y * diff.y));
}

bool TElecNokonoko::isResignationAttack()
{
	f32 range = mSaveParams->mSLCarapaceShootRange.get();

	if (!checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)) {
		if (calcDist(unk104.getPoint(), mPosition) < range) {
			mSpine->pushAfterCurrent(&TNerveElecNokonokoShoot::theNerve());
			return true;
		}
	}

	return false;
}

void TElecNokonoko::rest()
{
	TWalkerEnemy::reset();
	mScaledBodyRadius = 140.0f;
}

TElecNokonoko::TElecNokonoko(const char* name)
    : TWalkerEnemy(name)
    , mCarapace(nullptr)
    , unk198(0)
    , unk1A4(0)
{
}

// --------------------------------------------------------- TElecCarapace

void TElecCarapace::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TEnemyAttachment::perform(flags, graphics);

	if (flags & 1) {
		if (unk180 != 0) {
			unk180 = unk180 + 1;
			if (unk180 > 5)
				unk180 = 0;
		}
	}

	if (flags & 0x200) {
		bool dead = isUnk150Zero() ? true : false;
		if (dead)
			return;
		if (unk16C->checkLiveFlag(LIVE_FLAG_DEAD))
			return;
		if (checkLiveFlag(LIVE_FLAG_DEAD | LIVE_FLAG_HIDDEN))
			return;

		TCircleShadowRequest request;

		request.unk0 = mPosition;
		if (!isAirborne()) {
			request.unk0.y = mGroundHeight;
			request.unk1D  = 0;
		}

		request.unk10 = unk16C->mScaledBodyRadius;
		request.unkC  = request.unk10;
		request.unk14 = mRotation.y;
		gpBindShadowManager->request(request, getActorType());
	}
}
BOOL TElecCarapace::receiveMessage(THitActor* sender, u32 message)
{
	if (message == 0xd) {
		gpMarioParticleManager->emitAndBindToPosPtr(0xCD, &mPosition, 0,
		                                            nullptr);
		kill();
	}

	if (message == 0) {
		SMS_SendMessageToMario(this, 9);
	}

	if (message == 0xf) {
		return TRUE;
	}

	return FALSE;
}
void TElecCarapace::calcRootMatrix()
{
	MsMtxSetXYZRPH(mMActor->unk4->getBaseTRMtx(), mPosition.x, mPosition.y,
	               mPosition.z, mRotation.x, mRotation.y + unk188, mRotation.z);
	mMActor->unk4->setBaseScale(mScaling);

	if (gpMSound->gateCheck(0x20bc)) {
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x20bc, (const Vec*)&mPosition, 0, nullptr, 0, 4);
	}

	JPABaseEmitter* emitter0 = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x17a, (MtxPtr)((u8*)mMActor->unk4->mNodeMatrices + 0x60), 1, this);
	if (emitter0) {
		emitter0->setScale(unk16C->getScaling());
	}

	JPABaseEmitter* emitter1 = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x17b, (MtxPtr)((u8*)mMActor->unk4->mNodeMatrices + 0x60), 1, this);
	if (emitter1) {
		emitter1->setScale(unk16C->getScaling());
	}

	JPABaseEmitter* emitter2 = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x17c, (MtxPtr)((u8*)mMActor->unk4->mNodeMatrices + 0x60), 1, this);
	if (emitter2) {
		emitter2->setScale(unk16C->getScaling());
	}
}
void TElecCarapace::bind()
{
	control();
	TEnemyAttachment::bind();
}
void TElecCarapace::kill() { TEnemyAttachment::kill(); }
void TElecCarapace::loadInit(TSpineEnemy* host, const char* name)
{
	TEnemyAttachment::loadInit(host, name);

	unk16C = unk160;
	JDrama::TNameRefGen::search<TIdxGroupObj>(
	    "\x93\x47\x83\x4F\x83\x8B\x81\x5B\x83\x76")
	    ->getChildren()
	    .push_back(this);

	initHitActor(0x1000000B, 3, 0x98000000, 80.0f, 80.0f, 60.0f, 60.0f);
	offHitFlag(HIT_FLAG_NO_COLLISION);
	unk150 = 0;

	mSpine->initWith(&TNerveElecCarapaceMove::theNerve());

	volatile int low  = 0;
	volatile int high = 300;
	int range         = high - low;
	if (low + (int)(MsRandF() * range) < 150) {
		unk174 = 0;
	}

	host->mHeadHeight = 80.0f;
}
void TElecCarapace::appear()
{
	if (unk150 != 0)
		return;

	unk150 = 1;

	mPosition = unk16C->getPosition();

	f32 scale  = unk16C->mScaling.x;
	unk164     = scale;
	mScaling.z = scale;
	mScaling.y = scale;
	mScaling.x = scale;

	mBodyRadius = 50.0f;
	unk170      = 0;
	unk64 |= 1;
}
void TElecCarapace::rebirth() { }
void TElecCarapace::sendMessage()
{
	for (int i = 0; i < mColCount; ++i) {
		THitActor* col = mCollisions[i];
		if (col->isActorTypeOf(0x80000000)) {
			if (SMS_SendMessageToMario(this, 9)) {
				unk64 |= 1;
				if (mSpine->getCurrentNerve()
				    != &TNerveElecCarapaceWait::theNerve()) {
					mSpine->pushNerve(&TNerveElecCarapaceWait::theNerve());
				}
			}
			continue;
		}

		if (col == unk16C) {
			unk64 &= ~1;
			continue;
		}

		if (col->isActorTypeOf(0x1000000)) {
			// TODO: fabricated stack slots standing in for an inlined helper
			// (probably an electric-spark effect) that was DCE'd down to its
			// rand() side effects; remove once the real inline is identified.
			volatile s32 sparkData[2];
			sparkData[0] = 0;
			sparkData[1] = 0x168;
			for (int j = 0; j < 5; ++j) {
				rand();
				rand();
				rand();
			}
		} else if (TElecNokonoko::mReflectSw) {
			reflect(col);
		}
	}
}
void TElecCarapace::behaveToHitGround()
{
	if (unk176)
		unk184 = 1;

	u16 type = mGroundPlane->mBGType;
	bool isSlope;
	if (type == 0x100 || type == 0x101
	    || (u16)(type - 0x102) <= 3 || type == 0x4104) {
		isSlope = true;
	} else {
		isSlope = false;
	}

	if (isSlope)
		kill();

	unk176       = 0;
	unk168       = 1;
	mLiveFlag &= ~0x80;
	mVelocity.x = 0.0f;
	mVelocity.y = 0.0f;
	mVelocity.z = 0.0f;
}
void TElecCarapace::behaveToHitWall(const TBGCheckData* wall)
{
	if (unk180 > 0)
		return;
	if (!TElecNokonoko::mReflectSw)
		return;

	unk180 = 1;
	unk184 = 0;
	unk176 = 1;
	unk175 = 1;

	f32 dot = mLinearVelocity.dot(wall->getNormal());
	dot *= -1.5f;
	mVelocity.x = dot * wall->mNormal.x;
	mVelocity.y = 3.0f;
	mVelocity.z = dot * wall->mNormal.z;
	mPosition.y = 2.0f + mGroundHeight;

	setGoalPath(TPathNode(unk16C->getPosition()));
}
void TElecCarapace::setBehavior()
{
	if (unk16C->checkLiveFlag(LIVE_FLAG_DEAD))
		kill();

	if (unk168)
		mPosition.y = mGroundHeight;

	unk168 = 0;
}
void TElecCarapace::recoverScale() { }
f32 TElecCarapace::getNowGravity()
{
	return ((TElecNokonoko*)unk16C)->getSaveParams2()->mSLCarapaceGravity.get();
}
f32 TElecCarapace::getPhaseShift() const
{
	if (unk174)
		return 0.0f;
	return 180.0f;
}
void TElecCarapace::shoot()
{
	unk180 = 0;
	if (unk150 == 2)
		return;

	JGeometry::TVec3<f32> dir = *gpMarioPos;
	dir.sub(mPosition);

	JGeometry::TVec3<f32> target = dir;
	MsVECNormalize((Vec*)&target, (Vec*)&target);

	f32 flyDist
	    = ((TElecNokonoko*)unk16C)->mSaveParams->mSLCarapaceFlyDist.get();
	target.x *= flyDist;
	target.y *= mPosition.y;
	target.z *= flyDist;
	target.x += mPosition.x;
	target.y += mPosition.y;
	target.z += mPosition.z;

	unk174 = !unk174;
	unk188 = 0.0f;
	unk150 = 2;
	unk176 = 0;
	unk168 = 0;
	unk184 = 0;
	unk175 = 0;
	offHitFlag(1);

	mSpine->initWith(&TNerveElecCarapaceMove::theNerve());
	setGoalPath(TPathNode(target));

	TMsRange<f32> speedRange(3.0f, 5.0f);
	f32 speed = speedRange.rand();

	JGeometry::TVec3<f32> toGoal = unk104.getPoint();
	toGoal.sub(mPosition);
	unk178 = speed * JGeometry::TUtil<f32>::sqrt(toGoal.dot(toGoal));

	TMsRange<f32> lifeRange(20.0f, 30.0f);
	unk17C = lifeRange.rand();
}
void TElecCarapace::reflect(THitActor* other)
{
	if ((THitActor*)unk170 == other)
		return;

	unk184 = 0;
	unk170 = (int)other;
	unk176 = 1;
	unk175 = 0;

	JGeometry::TVec3<f32> dir(other->mPosition.x - mPosition.x, 0.0f,
	                          other->mPosition.z - mPosition.z);

	if (dir.x == 0.0f && dir.y == 0.0f && dir.z == 0.0f)
		dir.x = dir.x + 1.0f;

	MsVECNormalize((Vec*)&dir, (Vec*)&dir);

	f32 xDir = 0.0f;
	f32 yDir = 0.0f;
	f32 zDir = 0.0f;
	if (__fabsf(dir.z / dir.x) > 1.0f) {
		if (other->mPosition.z > mPosition.z)
			zDir = 1.0f;
		else
			zDir = -1.0f;
	} else {
		if (other->mPosition.x > mPosition.x)
			xDir = 1.0f;
		else
			xDir = -1.0f;
	}

	f32 dot = dir.y * yDir + dir.x * xDir + dir.z * zDir;
	f32 force = -7.0f * dot;
	mVelocity.x = dir.x * force;
	mVelocity.y = 2.0f;
	mVelocity.z = dir.z * force;
	mPosition.y = 2.0f + mGroundHeight;

	unk174 = 0;
	if ((mVelocity.x > 0.0f && mVelocity.z > 0.0f)
	    || (mVelocity.x < 0.0f && mVelocity.z < 0.0f))
		unk174 = 1;

	setGoalPath(TPathNode(unk16C->getPosition()));
}

#pragma dont_inline on
TElecCarapace::TElecCarapace(const char* name)
    : TEnemyAttachment(name)
    , unk16C(0)
    , unk170(0)
    , unk174(1)
    , unk175(0)
    , unk176(0)
{
	unk178 = 0.0f;
	unk17C = 0.0f;
	unk180 = 0;
	unk184 = 0;
	unk188 = 0.0f;
	unk198 = 0.0f;
}
#pragma dont_inline off

// ------------------------------------------------------------ params

TElecNokonokoSaveLoadParams::TElecNokonokoSaveLoadParams(const char* path)
    : TWalkerEnemyParams(path)
    , PARAM_INIT(mSLReadyTime, 300)
    , PARAM_INIT(mSLCarapaceGravity, 0.01f)
    , PARAM_INIT(mSLCarapaceSpeed, 5.0f)
    , PARAM_INIT(mSLCarapaceTurnSpeed, 2.0f)
    , PARAM_INIT(mSLCarapaceSpinSpeed, 2.0f)
    , PARAM_INIT(mSLCarapaceShootRange, 500.0f)
    , PARAM_INIT(mSLCarapaceFlyDist, 100.0f)
{
	load(mPrmPath);
}

// ------------------------------------------------------------ manager

TSpineEnemy* TElecNokonokoManager::createEnemyInstance()
{
	return new TElecNokonoko(
	    "\x93\x64\x8B\x43\x83\x6D\x83\x52\x83\x6D\x83\x52");
}

void TElecNokonokoManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "dennoko_model1.bmd", 0x10220000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TElecNokonokoManager::load(JSUMemoryInputStream& stream)
{
	TSmallEnemyManager::load(stream);
	unk38 = new TElecNokonokoSaveLoadParams("/enemy/elecNokonoko.prm");
	mMaterialTable = J3DModelLoaderDataBase::loadMaterialTable(
	    JKRFileLoader::getGlbResource("/scene/dennoko/dennoko_model1.bmt"));
}

void TElecNokonokoManager::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TEnemyManager::perform(flags, graphics);
	for (int i = 0; i < getActiveObjNum(); ++i)
		((TElecNokonoko*)getObj(i))->mCarapace->perform(flags, graphics);
}

void TElecNokonokoManager::clipEnemies(JDrama::TGraphics* graphics)
{
	f32 radius;
	f32 far;
	if (unk38 == nullptr) {
		far    = gpConductor->getCondParams().mEnemyFarClip.get();
		radius = 300.0f;
	} else {
		far    = unk38->mSLFarClip.get();
		radius = unk38->mSLClipRadius.get();
	}

	SetViewFrustumClipCheckPerspective(
	    gpCamera->getFovy(), gpCamera->getAspect(), graphics->mNearPlane, far);

	for (int i = 0; i < mObjNum; ++i) {
		TElecNokonoko* enemy = (TElecNokonoko*)unk18[i];

		if (ViewFrustumClipCheck(graphics, &enemy->mPosition, radius))
			enemy->offLiveFlag(LIVE_FLAG_CLIPPED_OUT);
		else
			enemy->onLiveFlag(LIVE_FLAG_CLIPPED_OUT);

		if (!enemy->mCarapace->isUnk150Zero()) {
			if (ViewFrustumClipCheck(
			        graphics, &enemy->mCarapace->mPosition, radius))
				enemy->mCarapace->offLiveFlag(LIVE_FLAG_CLIPPED_OUT);
			else
				enemy->mCarapace->onLiveFlag(LIVE_FLAG_CLIPPED_OUT);
		}
	}
}

void TElecNokonokoManager::initSetEnemies() { }

TElecNokonokoManager::TElecNokonokoManager(const char* name)
    : TSmallEnemyManager(name)
{
}
