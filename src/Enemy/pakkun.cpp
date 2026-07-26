#include <Enemy/Pakkun.hpp>
#include <Camera/Camera.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/EffectObj.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <M3DUtil/MActor.hpp>
#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <Map/PollutionManager.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <MarioUtil/TexUtil.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MoveBG/ItemManager.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/ModelWaterManager.hpp>
#include <Player/Watergun.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Strategy.hpp>
#include <System/Particles.hpp>
#include <math.h>

// rogue includes needed for matching sinit & bss
#include <M3DUtil/InfectiousStrings.hpp>
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

static const char* pakkun_bastable[] = {
	"/scene/pakkun/bas/pakun_crush_to_hide.bas",
	"/scene/pakkun/bas/pakun_damage.bas",
	"/scene/pakkun/bas/pakun_down.bas",
	"/scene/pakkun/bas/pakun_hide.bas",
	nullptr,
	nullptr,
	nullptr,
	"/scene/pakkun/bas/pakun_set.bas",
	"/scene/pakkun/bas/pakun_shoot.bas",
	nullptr,
};

static const TModelDataLoadEntry entry[] = {
	{ "pakun.bmd", 0x10300000, 0 },
	{ nullptr, 0, 0 },
};

static TPakkun* gpCurPakkun;
static TPakkunSeed* gpCurPakkunSeed;

static int PakkunRootCallback(J3DNode*, int);
static int PakkunRootCallback2(J3DNode*, int);
static int PakkunSeedCallback(J3DNode*, int);

u8 TPakkun::mHeadJntIndex = 0;

f32 TPakkunManager::mRootExplosionScaleRate = 2.0f;
f32 TPakkunManager::mTestFlyAngX            = 30.0f;
f32 TPakkunManager::mIgnoreHitWaterY        = 50.0f;

static void setMarioGoalPath(TPakkun* pakkun)
{
	TPathNode node((THitActor*)gpMarioAddress);
	if (gpMarioAddress) {
		node.unk4.set(*(f32*)(gpMarioAddress + 0x10),
		              *(f32*)(gpMarioAddress + 0x14),
		              *(f32*)(gpMarioAddress + 0x18));
	}

	pakkun->unkF4  = node;
	pakkun->unk104 = node;
	pakkun->unk114.clear();
}

DEFINE_NERVE(TNerveStayPakkunAppear, TLiveActor)
{
	TLiveActor* actor = spine->getBody();
	TStayPakkun* self = (TStayPakkun*)actor;
	if (spine->getTime() == 0) {
		self->offLiveFlag(LIVE_FLAG_HIDDEN);
		self->setBckAnm(7);
		self->unk1B1 = true;
		self->mSeed->rebirth();
	}

	f32 frame = self->getCurAnmFrameNo(0);
	if (frame > 0.0f && frame < 25.0f
	    && !self->checkLiveFlag(LIVE_FLAG_UNK400)
	    && gpPollution->isPolluted(self->mPosition.x, self->mPosition.y,
	                               self->mPosition.z)) {
		JPABaseEmitter* emitter = gpMarioParticleManager->emit(
		    0x12d, &self->mPosition, 1, self);
		if (emitter) {
			emitter->unk154.x = 1.5f;
			emitter->unk154.y = 1.5f;
			emitter->unk154.z = 1.5f;
			emitter->unk174.x = 1.5f;
			emitter->unk174.y = 1.5f;
			emitter->unk174.z = 1.5f;
			SMSSetEmitterPolColor(emitter, 6);
		}
	}

	if (self->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNervePakkunStay::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveStayPakkunHide, TLiveActor)
{
	TStayPakkun* self = (TStayPakkun*)spine->getBody();
	if (spine->getTime() == 0) {
		self->onHitFlag(HIT_FLAG_NO_COLLISION);
		if (self->mCurrentBckAnm != 0)
			self->setBckAnm(3);
		self->mSeed->kill();
	} else if (self->unk1BC
	           && spine->getTime()
	                  > self->mPakkunParams->mSLDamageHideTime.get()) {
		self->unk1BC = false;
	}

	if (self->checkCurAnmEnd(0)) {
		self->onLiveFlag(LIVE_FLAG_HIDDEN);
		if (!self->unk1BC
		    && gpPollution->isPolluted(self->mPosition.x, self->mPosition.y,
		                               self->mPosition.z)
		    && self->isFindMario(0.9f)) {
			TSpineEnemyParams* params = self->getSaveParam();
			self->mHitPoints          = params ? params->mSLHitPointMax.get() : 1;
			spine->pushAfterCurrent(&TNerveStayPakkunAppear::theNerve());
			return TRUE;
		}
	}

	f32 frame = self->getCurAnmFrameNo(0);
	if (frame > 47.0f && frame < 80.0f
	    && !self->checkLiveFlag(LIVE_FLAG_HIDDEN | LIVE_FLAG_CLIPPED_OUT)) {
		if (gpPollution->isPolluted(self->mPosition.x, self->mPosition.y,
		                            self->mPosition.z)) {
			JPABaseEmitter* emitter = gpMarioParticleManager->emit(
			    0x12d, &self->mPosition, 1, self);
			if (emitter)
				SMSSetEmitterPolColor(emitter, 6);
		} else {
			gpMarioParticleManager->emit(0x13e, &self->mPosition, 1,
			                             self);
			gpMarioParticleManager->emit(0x13f, &self->mPosition, 1,
			                             self);
		}
	}

	self->walkToCurPathNode(0.0f, 3.0f * self->mTurnSpeed, 0.0f);
	return FALSE;
}

DEFINE_NERVE(TNervePakkunFreeze, TLiveActor)
{
	TPakkun* self = (TPakkun*)spine->getBody();
	if (spine->getTime() == 0)
		self->setBckAnm(6);

	if (self->checkCurAnmEnd(0)) {
		if (self->isBckAnm(6)) {
			bool sprayed = self->unk165;
			if (sprayed)
				self->unk165 = false;
			if (sprayed)
				self->setBckAnm(4);
		} else {
			bool sprayed = self->unk165;
			if (sprayed)
				self->unk165 = false;
			if (sprayed) {
				self->setBckAnm(4);
			} else if (self->isBckAnm(4)) {
				self->setBckAnm(5);
			} else {
				return TRUE;
			}
		}
	}

	return FALSE;
}

DEFINE_NERVE(TNervePakkunShoot, TLiveActor)
{
	TPakkun* self = (TPakkun*)spine->getBody();
	if (spine->getTime() == 0)
		self->setBckAnm(8);

	if (self->getMActor()->getFrameCtrl(0)->checkPass(60.0f))
		self->shootIn();
	if (self->getMActor()->getFrameCtrl(0)->checkPass(70.0f))
		self->shoot();

	self->walkToCurPathNode(0.0f, self->mTurnSpeed, 0.0f);
	if (self->checkCurAnmEnd(0)) {
		setMarioGoalPath(self);
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNervePakkunHide, TLiveActor)
{
	TPakkun* self = (TPakkun*)spine->getBody();
	if (spine->getTime() == 0)
		self->setBckAnm(3);

	if (self->checkCurAnmEnd(0)) {
		self->onHitFlag(HIT_FLAG_NO_COLLISION);
		self->onLiveFlag(LIVE_FLAG_HIDDEN);
		if (self->mSeed->isUnk150Zero()) {
			self->mPosition = self->mSeed->mPosition;
			self->mPosition.y = self->mSeed->mGroundHeight;
			spine->pushAfterCurrent(&TNervePakkunAppear::theNerve());
			self->setBckAnm(7);
			return TRUE;
		}
	}

	return FALSE;
}

DEFINE_NERVE(TNervePakkunAppear, TLiveActor)
{
	TPakkun* self = (TPakkun*)spine->getBody();
	if (spine->getTime() == 0) {
		self->setBckAnm(7);
		self->offHitFlag(HIT_FLAG_NO_COLLISION);
	}

	self->getMActor()->getFrameCtrl(0)->checkPass(100.0f);
	if (self->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNervePakkunStay::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNervePakkunStay, TLiveActor)
{
	TPakkun* self = (TPakkun*)spine->getBody();
	if (spine->getTime() == 0)
		self->setWaitAnm();

	TSmallEnemyParams* params = self->getSaveParam2();
	int waitTime              = params->mSLWaitTime.get();

	if (self->mSeed->isUnk150Zero() && self->checkCurAnmEnd(0)) {
		if (spine->getTime() >= self->mPakkunParams->mSLReadyTime.get()
		    || spine->getTime() >= waitTime || self->unk1B1) {
			JGeometry::TVec3<f32> goal = self->unk104.getPoint();
			JGeometry::TVec3<f32> toGoal(goal);
			toGoal.sub(self->mPosition);
			f32 goalDist = JGeometry::TUtil<f32>::sqrt(toGoal.squared());

			f32 scale = 1.0f;
			if (self->mHasSubSeeds)
				scale = 3.0f;

			JGeometry::TVec3<f32> marioPos = *gpMarioPos;
			if (goalDist < self->mPakkunParams->mSLShootRange.get() * scale
			    || self->mHasSubSeeds) {
				if (fabsf(gpMarioPos->y - self->mPosition.y)
				    < params->mSLSearchHeight.get() * scale
				    && self->isInSight(
				        marioPos, params->mSLSearchLength.get() * scale,
				        params->mSLSearchAngle.get() * scale,
				        params->mSLSearchAware.get() * scale)) {
					spine->pushAfterCurrent(&TNervePakkunStay::theNerve());
					spine->pushAfterCurrent(&TNervePakkunShoot::theNerve());
					self->unk1B1 = false;

					if (self->unk1B0 && !self->mHasSubSeeds) {
						self->unk1B0 = false;
						JGeometry::TVec3<f32> target
						    = self->unk104.getPoint();
						self->setGoalPath(TPathNode(target));

						JGeometry::TVec3<f32> velocity
						    = self->calcVelocityToJumpToY(
						        target,
						        self->mPakkunParams->mSLSeedSpeedC.get(),
						        self->mPakkunParams->mSLSeedGravityC.get());
						self->mShootType       = 1;
						self->mSeed->mVelocity = velocity;
						self->mSeed->mRotation.x
						    = TPakkunManager::mTestFlyAngX;
						self->mSeed->mRotation.y = 0.0f;
						self->mSeed->mRotation.z = 0.0f;
					} else {
						JGeometry::TVec3<f32> dir;
						dir.x = gpMarioPos->x - self->mPosition.x;
						dir.y = gpMarioPos->y - self->mPosition.y;
						dir.z = gpMarioPos->z - self->mPosition.z;
						self->onShootLiner(dir);
					}

					return TRUE;
				}
			} else if (spine->getTime() >= waitTime) {
				spine->pushAfterCurrent(&TNervePakkunHide::theNerve());
				spine->pushAfterCurrent(&TNervePakkunShoot::theNerve());

				int angle = (int)MsRandF(0.0f, 36000.0f);
				JGeometry::TVec3<f32> target = self->unk104.getPoint();

				JGeometry::TVec3<f32> targetDiff(target);
				targetDiff.sub(self->mPosition);
				f32 targetDist
				    = JGeometry::TUtil<f32>::sqrt(targetDiff.squared());

				if (targetDist > self->mPakkunParams->mSLLimitMove.get()) {
					target.x = gpMarioPos->x - self->mPosition.x;
					target.y = 0.0f;
					target.z = gpMarioPos->z - self->mPosition.z;

					if (target.x == 0.0f && target.y == 0.0f
					    && target.z == 0.0f)
						target.x += 1.0f;

					MsVECNormalize((Vec*)&target, (Vec*)&target);
					target.x = self->mPosition.x
					           + target.x
					                 * self->mPakkunParams->mSLMoveDist.get();
					target.z = self->mPosition.z
					           + target.z
					                 * self->mPakkunParams->mSLMoveDist.get();
				} else {
					u16 angleShort = angle;
					target.x += self->mPakkunParams->mSLMarioCircle.get()
					            * jmaCosTable[angleShort >> jmaSinShift];
					target.z += self->mPakkunParams->mSLMarioCircle.get()
					            * jmaSinTable[angleShort >> jmaSinShift];
				}

				self->setGoalPath(TPathNode(target));
				JGeometry::TVec3<f32> velocity = self->calcVelocityToJumpToY(
				    target, self->mPakkunParams->mSLSeedSpeedC.get(),
				    self->mPakkunParams->mSLSeedGravityC.get());
				self->mShootType       = 1;
				self->mSeed->mVelocity = velocity;
				self->mSeed->mRotation.x = TPakkunManager::mTestFlyAngX;
				self->mSeed->mRotation.y = 0.0f;
				self->mSeed->mRotation.z = 0.0f;

				return TRUE;
			}
		}
	}

	self->walkToCurPathNode(0.0f, self->mTurnSpeed, 0.0f);
	if (self->mHasSubSeeds) {
		if (!self->isFindMario(1.0f)) {
			f32 giveUpLength = params->mSLGiveUpLength.get();
			JGeometry::TVec3<f32> goal = self->unk104.getPoint();
			goal.sub(self->mPosition);
			f32 goalDist = JGeometry::TUtil<f32>::sqrt(goal.squared());

			if (goalDist > giveUpLength) {
				spine->pushAfterCurrent(&TNerveStayPakkunHide::theNerve());
				return TRUE;
			}
		}
	}

	return FALSE;
}

DEFINE_NERVE(TNervePakkunGenerate, TLiveActor)
{
	TPakkun* self = (TPakkun*)spine->getBody();
	if (spine->getTime() == 0) {
		self->onHitFlag(HIT_FLAG_NO_COLLISION);
		self->onLiveFlag(LIVE_FLAG_HIDDEN);
		self->mSeed->appear();
	}

	if (self->mHolder)
		return FALSE;

	TPakkunSeed* seed = self->mSeed;
	if (seed->unk150 == 1) {
		seed->TEnemyAttachment::set();
		seed->mScaling.x = seed->unk164;
		seed->mScaling.y = seed->unk164;
		seed->mScaling.z = seed->unk164;

		if (spine->getTime() % 5 == 0) {
			self->updateSquareToMario();
			f32 dist = self->mPakkunParams->mSLGenerateSeedDist.get();
			if (self->mDistToMarioSquared < dist * dist)
				seed->unk150 = 2;
		}
	}

	if (seed->isUnk150Zero()) {
		self->mPosition = seed->mPosition;
		self->mPosition.y = seed->mGroundHeight;
		spine->pushAfterCurrent(&TNervePakkunAppear::theNerve());
		self->setBckAnm(7);
		return TRUE;
	}

	return FALSE;
}

TSpineEnemyParams* TStayPakkun::getSaveParam() const
{
	return ((TPakkunManager*)mManager)->mStayParams;
}

void TStayPakkun::shoot()
{
	mSeed->shoot();
}

void TStayPakkun::kill()
{
	TSmallEnemy::kill();
	mSeed->kill();
	for (int i = 0; i < 2; ++i)
		mSubSeeds[i]->kill();
}

void TStayPakkun::shootIn()
{
	mSeed->appear();
	mSeed->set();

	for (int i = 0; i < 2; ++i) {
		mSubSeeds[i]->appear();
		mSubSeeds[i]->set();

		JGeometry::TVec3<f32> vel = mSeed->mVelocity;
		Mtx mtx;
		f32 pitch = -10.0f;
		if (i != 0)
			pitch *= -1.0f;
		MsMtxSetRotRPH(mtx, 0.0f, pitch, 0.0f);
		PSMTXMultVec(mtx, (Vec*)&vel, (Vec*)&vel);
		mSubSeeds[i]->mVelocity = vel;
	}
}

bool TStayPakkun::isHitValid(u32 flag)
{
	if (flag == 0xb) {
		onLiveFlag(LIVE_FLAG_DEAD);
		onLiveFlag(LIVE_FLAG_UNK20000);
		return true;
	}

	if (mSpine->getCurrentNerve() == &TNerveStayPakkunAppear::theNerve())
		return false;
	if (mSpine->getCurrentNerve() == &TNerveStayPakkunHide::theNerve())
		return false;

	mSpine->pushNerve(&TNerveStayPakkunHide::theNerve());
	unk1BC = true;
	gpPollution->clean(mPosition.x, mGroundHeight, mPosition.z,
	                   32.0f * getSaveParam2()->mSLPolluteRange.get());
	if (mSeed->isUnk150Zero())
		mSeed->kill();
	setBckAnm(0);
	return false;
}

void TStayPakkun::setBehavior()
{
	if (isBckAnm(4))
		--mHitPoints;

	u8 maxHp = getSaveParam() ? getSaveParam()->mSLHitPointMax.get() : 1;
	unk1B2.a               = (mHitPoints * 255) / maxHp;
	mRootScale = 1.0f
	             + (TPakkunManager::mRootExplosionScaleRate
	                * (255.0f - unk1B2.a) / 255.0f);

	if (mHitPoints < 2) {
		if (gpMSound->gateCheck(0x287f))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x287f, &mPosition, 0, nullptr, 0, 4);
		mHitPoints = 1;
		kill();
	}
}

void TStayPakkun::calcRootMatrix()
{
	gpCurPakkun = this;
	TSpineEnemy::calcRootMatrix();
}

void TStayPakkun::genRandomItem()
{
	unk1A4 = mPosition;
	unk1A4.y += 100.0f;

	TWaterEmitInfo* info
	    = ((TPakkunManager*)mManager)->mWaterEmitInfo;
	info->mPos.value = unk1A4;
	gpModelWaterManager->emitRequest(*info);

	TWaterGun* waterGun = (TWaterGun*)SMS_GetMarioWaterGun();
	if ((waterGun->mCurrentWater << 2)
	    < waterGun->getCurrentNozzle()->mEmitParams.mAmountMax.get()) {
		gpItemManager->makeObjAppear(mPosition.x, mPosition.y, mPosition.z,
		                             0x20000002, true);
	} else {
		unk18C = 3;
		setDeadAnm();
	}

	JPABaseEmitter* emitter
	    = gpMarioParticleManager->emit(0xa1, &unk1A4, 0, nullptr);
	if (emitter)
		emitter->setScale(JGeometry::TVec3<f32>(1.5f, 1.5f, 1.5f));

	emitter = gpMarioParticleManager->emit(0xa2, &unk1A4, 0, nullptr);
	if (emitter)
		emitter->setScale(JGeometry::TVec3<f32>(1.5f, 1.5f, 1.5f));
}

void TStayPakkun::setDeadAnm()
{
	setBckAnm(2);
}

void TStayPakkun::reset()
{
	TPakkun::reset();
	unk1B2.a = 0xff;
	offLiveFlag(LIVE_FLAG_UNK800);
	onLiveFlag(LIVE_FLAG_HIDDEN);
}

void TStayPakkun::init(TLiveManager* manager)
{
	TPakkun::init(manager);
	mSubSeeds = new TPakkunSeed*[2];
	for (int i = 0; i < 2; ++i) {
		mSubSeeds[i] = new TPakkunSeed("パックン種");
		mSubSeeds[i]->loadInit(this, "seed.bmd");
	}
	mSpine->initWith(&TNerveStayPakkunHide::theNerve());
}

void TStayPakkun::load(JSUMemoryInputStream& stream)
{
	TSmallEnemy::load(stream);
	reset();
	setGoalPathMario();
	mHasSubSeeds = true;
}

void TPakkunSeed::forceKill()
{
	if (mGroundPlane->getBGType() == 0x104
	    || mGroundPlane->getBGType() == 0x105
	    || mGroundPlane->getBGType() == 0x4104
	    || mGroundPlane->checkFlag(0x10)
	    || !gpMap->isInArea(mPosition.x, mPosition.z)) {
		kill();
		if (!mHost->mHasSubSeeds && mHost->mSeed->checkLiveFlag(LIVE_FLAG_DEAD)) {
			mHost->mSeed->kill();
			mHost->mSeed->onLiveFlag(0x20000);
		}
	}
}

void TPakkunSeed::set()
{
	TEnemyAttachment::set();
	if (((TPakkun*)unk160)->checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)) {
		mPosition.x = ((TPakkun*)unk160)->mPosition.x;
		mPosition.y = ((TPakkun*)unk160)->mPosition.y + 80.0f;
		mPosition.z = ((TPakkun*)unk160)->mPosition.z;
	} else {
		MtxPtr mtx  = mHost->getModel()->getAnmMtx(TPakkun::mHeadJntIndex);
		mPosition.x = mtx[0][3];
		mPosition.y = mtx[1][3] - 50.0f;
		mPosition.z = mtx[2][3];
	}
}

void TPakkunSeed::rebirth()
{
	if (mHost->mHasSubSeeds) {
		unk150 = 0;
		unk158 = 0;
		onHitFlag(HIT_FLAG_NO_COLLISION);

		TSmallEnemyManager* manager = (TSmallEnemyManager*)mHost->mManager;
		gpPollution->stamp(manager->getUnk58(), mPosition.x, mPosition.y,
		                   mPosition.z,
		                   32.0f
		                       * manager->getSaveParam2()->getSLStampRange()
		                       * mHost->unk158);
		if (gpMSound->gateCheck(0x287e))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x287e, &mPosition, 0, nullptr, 0, 4);
		return;
	}

	unk158 += 1;
	if (unk158 > mHost->mPakkunParams->mSLGenerateSeedTime.get()
	    || mHost->unk1B1) {
		unk150 = 0;
		unk158 = 0;
		onHitFlag(HIT_FLAG_NO_COLLISION);
	}

	if (mPosition.y < mGroundHeight - 70.0f) {
		mVelocity.y = 0.0f;
		onHitFlag(HIT_FLAG_NO_COLLISION);
		return;
	}

	u16 bgType = mGroundPlane->getBGType();
	if (bgType == 0x100 || (u16)(bgType - 0x101) <= 4
	    || bgType == 0x4104) {
		TEffectColumWater* enemy
		    = (TEffectColumWater*)gpConductor->makeOneEnemyAppear(
		        mPosition, "エフェクト水柱マネージャー", 0);
		if (enemy)
			enemy->generate(mPosition, mVelocity);
	} else {
		if (gpMSound->gateCheck(0x287e))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x287e, &mPosition, 0, nullptr, 0, 4);
		gpMarioParticleManager->emit(0x13e, &mPosition, 1, mHost->mSeed);
		gpMarioParticleManager->emit(0x13f, &mPosition, 1, mHost->mSeed);
	}
}

void TPakkunSeed::shoot()
{
	switch (mHost->mShootType) {
	case 0:
		unk150 = 3;
		break;
	case 1:
		unk150 = 4;
		break;
	}

	mScaling.x = 0.1f;
	mScaling.y = 0.1f;
	mScaling.z = 0.1f;
	unk168     = 0;
	offHitFlag(HIT_FLAG_NO_COLLISION);
}

void TPakkunSeed::appear()
{
	unk150        = 1;
	mHost->unk1B0 = 0;
	mScaling.x = mScaling.y = mScaling.z = 0.5f;
}

f32 TPakkunSeed::getNowGravity()
{
	TPakkunSaveLoadParams* params = (TPakkunSaveLoadParams*)mHost->getSaveParam();
	f32 gravity                   = params->mSLSeedGravityS.get();
	if (unk150 == 4)
		gravity = params->mSLSeedGravityC.get();
	return gravity;
}

void TPakkunSeed::behaveToHitGround()
{
	if (fabsf(mVelocity.y) < 1.0f
	    || mGroundPlane->getBGType() == 0x100
	    || mGroundPlane->getBGType() == 0x101
	    || (mGroundPlane->getBGType() - 0x102U) <= 3
	    || mGroundPlane->getBGType() == 0x4104) {
		unk168 = 1;
		offLiveFlag(LIVE_FLAG_UNK100);
		mVelocity.set(0.0f, -0.3f, 0.0f);
		rebirth();
		return;
	}

	if (unk150 == 3) {
		mVelocity.x *= 0.8f;
		mVelocity.z *= 0.8f;
		mVelocity.y = 0.1f * fabsf(mVelocity.y);
	} else {
		mVelocity.x *= 0.4f;
		mVelocity.z *= 0.4f;
		mVelocity.y = 0.4f * fabsf(mVelocity.y);
	}
}

void TPakkunSeed::calcRootMatrix()
{
	MSound* sound = gpMSound;
	if (sound->gateCheck(0x2169))
		MSoundSESystem::MSoundSE::startSoundActor(0x2169, &mPosition, 0,
		                                          nullptr, 0, 4);
	TEnemyAttachment::calcRootMatrix();
	gpCurPakkunSeed = this;
}

void TPakkunSeed::behaveToHitWall(const TBGCheckData* plane)
{
	f32 dot = mVelocity.dot(plane->getNormal());
	f32 s   = 1.5f * dot;
	s       = -s;
	mVelocity.x += s * plane->getNormal().x;
	mVelocity.y += s * plane->getNormal().y;
	if (unk150 == 3)
		mVelocity.y = -5.0f;
	mVelocity.z += s * plane->getNormal().z;
	mHost->unk1B0 = 1;
}

void TPakkunSeed::behaveToHost()
{
	if (mHost->mHasSubSeeds)
		return;
	((TPakkun*)unk160)->offLiveFlag(LIVE_FLAG_HIDDEN);
}

void TPakkunSeed::moveObject()
{
	TEnemyAttachment::moveObject();
	if (!unk168) {
		f32 roll = mRollAngle + 5.0f;
		while (roll >= 360.0f)
			roll -= 360.0f;
		while (roll < 0.0f)
			roll += 360.0f;
		mRollAngle = roll;

		if (mPosition.y > mGroundHeight + 20.0f) {
			JGeometry::TVec3<f32> velocity = mVelocity;
			mRotation.x = MsGetRotFromZaxis(velocity).x;
		}
	} else {
		f32 roll = mRollAngle + 5.0f;
		if (roll > 360.0f)
			roll = 360.0f;
		else if (roll < 0.0f)
			roll = 0.0f;
		mRollAngle = roll;
		mRotation.x *= 0.8f;
	}
}

void TPakkunSeed::loadInit(TSpineEnemy* host, const char* model)
{
	unk160 = host;
	mMActorKeeper = new TMActorKeeper(unk160->mManager, 1);
	mMActorKeeper->mModelLoaderFlags = 0x10220000;
	mMActor = mMActorKeeper->createMActor(model, 3);
	mHost   = (TPakkun*)unk160;

	TIdxGroupObj* group
	    = JDrama::TNameRefGen::search<TIdxGroupObj>("オブジェクトグループ");
	group->getChildren().push_back(this);

	initHitActor(0x10000006, 1, 0x80000000, 20.0f, 20.0f, 20.0f, 20.0f);
	offHitFlag(HIT_FLAG_NO_COLLISION);
	unk150       = 0;
	mGroundPlane = TMap::getIllegalCheckData();
	mMActor->getModel()
	    ->getModelData()
	    ->getJointNodePointer(0)
	    ->setCallBack(PakkunSeedCallback);
}

const char** TPakkun::getBasNameTable() const
{
	return pakkun_bastable;
}

void TPakkun::shoot()
{
	mSeed->shoot();
}

void TPakkun::shootIn()
{
	mSeed->appear();
	mSeed->set();
}

void TPakkun::reset()
{
	gpCurPakkun = this;
	TSmallEnemy::reset();
	unk1B1     = true;
	unk1B2.a   = false;
	mRootScale = 1.0f;
}

void TPakkun::setMActorAndKeeper()
{
	mMActorKeeper = new TMActorKeeper(mManager, 1);
	mMActor       = mMActorKeeper->createMActor("pakun.bmd", 3);
}

void TPakkun::behaveToWater(THitActor* actor)
{
	mSprayedByWaterCooldown = 0;
	if (actor->mPosition.y <= mGroundHeight + TPakkunManager::mIgnoreHitWaterY)
		return;

	unk165 = true;
	if (mHitPoints > 5)
		mHitPoints -= 4;

	if (mSpine->getCurrentNerve() != &TNervePakkunFreeze::theNerve()
	    && mSpine->getCurrentNerve() != &TNerveStayPakkunAppear::theNerve()
	    && mSpine->getCurrentNerve() != &TNerveStayPakkunHide::theNerve()) {
		mSpine->pushNerve(&TNervePakkunFreeze::theNerve());
		if (mSeed->isUnk150Zero())
			mSeed->kill();
	}
}

void TPakkun::onShootLiner(JGeometry::TVec3<f32>& dir)
{
	JGeometry::TVec3<f32> pos = mPosition;
	pos.x += dir.x * 200.0f;
	pos.z += dir.z * 200.0f;
	unkF4  = TPathNode(nullptr);
	unk104 = TPathNode(nullptr);
	unkF4.unk4 = pos;
	unk104.unk4 = pos;
	unk114.clear();
	mShootType = 0;
	if (dir.x == 0.0f && dir.y == 0.0f && dir.z == 0.0f)
		dir.x += 1.0f;

	MsVECNormalize((Vec*)&dir, (Vec*)&dir);
	f32 speed = mPakkunParams->mSLSeedSpeedS.get();
	dir.x *= speed;
	dir.y = -5.0f;
	dir.z *= speed;
	mSeed->mVelocity = dir;
}

void TPakkun::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (checkLiveFlag(LIVE_FLAG_UNK10000))
		return;

	mSeed->perform(flags, graphics);
	if (checkLiveFlag(LIVE_FLAG_DEAD))
		return;

	if (flags & 1) {
		if (mHasSubSeeds)
			offLiveFlag(LIVE_FLAG_CLIPPED_OUT);
		if (!gpMap->isInArea(mPosition.x, mPosition.z))
			kill();
		moveObject();
	}

	if (checkLiveFlag(LIVE_FLAG_HIDDEN)) {
		if (flags & 2)
			mMActor->frameUpdate();
		return;
	}

	if (flags & 2) {
		calcRootMatrix();
		updateAnmSound();
		mMActor->calcAnm();
	}
	if (!checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)) {
		if (flags & 4)
			mMActor->viewCalc();
		if (flags & 0x200)
			drawObject(graphics);
	}
}

void TPakkun::setDeadAnm()
{
	if (!checkLiveFlag(LIVE_FLAG_HIDDEN))
		setBckAnm(2);
}

void TPakkun::setFreezeAnm()
{
	setBckAnm(1);
}

void TPakkun::setWaitAnm()
{
	setBckAnm(9);
}

void TPakkun::kill()
{
	TSmallEnemy::kill();
	mSeed->kill();
}

void TPakkun::init(TLiveManager* manager)
{
	TSmallEnemy::init(manager);
	mActorType = 0x10000004;
	unk150     = 17;
	mPakkunParams = (TPakkunSaveLoadParams*)getSaveParam();
	mSpine->initWith(&TNervePakkunGenerate::theNerve());
	setMarioGoalPath(this);
	if (mInstanceIndex == 0) {
		for (u8 i = 0; i < getModel()->getModelData()->getJointNum(); ++i) {
			const char* name
			    = getModel()->getModelData()->getJointName()->getName(i);
			if (strcmp(name, "null_seed") == 0)
				mHeadJntIndex = i;
		}
	}

	mSeed = new TPakkunSeed("パックン種");
	mSeed->loadInit(this, "seed.bmd");
	mSeed->unk164 = mBodyScale;

	ResTIMG* img = (ResTIMG*)JKRFileLoader::getGlbResource(
	    "/scene/map/pollution/H_ma_rak.bti");
	if (img)
		SMS_ChangeTextureAll(getMActor()->getModel()->getModelData(),
		                     "H_ma_rak_dummy", *img);

	for (u16 i = 0; i < getMActor()->getModel()->getModelData()->getMaterialNum();
	     ++i) {
		SMS_InitPacket_OneTevKColor(getMActor()->getModel(), i, GX_KCOLOR0,
		                            &unk1B2);
	}

	getMActor()->setJointCallback(1, PakkunRootCallback);
	getMActor()->setJointCallback(2, PakkunRootCallback2);
}

void TPakkun::load(JSUMemoryInputStream& stream)
{
	TSmallEnemy::load(stream);
	reset();
	setMarioGoalPath(this);
}

#pragma dont_inline on
TPakkun::TPakkun(const char* name)
    : TSmallEnemy(name)
    , mSeed(nullptr)
    , mShootType(0)
    , mHasSubSeeds(0)
    , mSubSeeds(nullptr)
    , mRootScale(1.0f)
    , unk1BC(0)
{
}
#pragma dont_inline off

void TSmallEnemy::initAttacker(THitActor*) { unk184 = 1; }

void TPakkunManager::clipEnemies(JDrama::TGraphics* graphics)
{
	f32 farClip;
	f32 radius;
	if (unk38) {
		farClip = unk38->mSLFarClip.get();
		radius  = unk38->mSLClipRadius.get();
	} else {
		farClip = gpConductor->getCondParams().getEnemyFarClip();
		radius  = 300.0f;
	}

	SetViewFrustumClipCheckPerspective(gpCamera->getFovy(),
	                                   gpCamera->getAspect(),
	                                   graphics->mNearPlane, farClip);
	farClip *= farClip;

	for (int i = 0; i < mObjNum; ++i) {
		TPakkun* pakkun = (TPakkun*)unk18[i];
		if (pakkun->mHasSubSeeds) {
			pakkun->updateSquareToMario();
			if (pakkun->mDistToMarioSquared < farClip)
				continue;
		}

		if (ViewFrustumClipCheck(graphics, (Vec*)&pakkun->mPosition,
		                         radius))
			pakkun->offLiveFlag(LIVE_FLAG_CLIPPED_OUT);
		else
			pakkun->onLiveFlag(LIVE_FLAG_CLIPPED_OUT);

		if (!pakkun->mSeed->isUnk150Zero()) {
			if (ViewFrustumClipCheck(graphics,
			                         (Vec*)&pakkun->mSeed->mPosition,
			                         radius))
				pakkun->mSeed->offLiveFlag(LIVE_FLAG_CLIPPED_OUT);
			else
				pakkun->mSeed->onLiveFlag(LIVE_FLAG_CLIPPED_OUT);
		}

		if (pakkun->mHasSubSeeds) {
			for (int j = 0; j < 2; ++j) {
				TPakkunSeed* seed = pakkun->mSubSeeds[j];
				if (!seed->isUnk150Zero()) {
					if (ViewFrustumClipCheck(
					        graphics, (Vec*)&seed->mPosition, radius))
						seed->offLiveFlag(LIVE_FLAG_CLIPPED_OUT);
					else
						seed->onLiveFlag(LIVE_FLAG_CLIPPED_OUT);
				}
			}
		}
	}
}

TSmallEnemy* TPakkunManager::createEnemyInstance()
{
	return new TPakkun("パックン");
}

void TPakkunManager::createModelData()
{
	createModelDataArray(entry);
}

void TPakkunManager::loadAfter()
{
	TSmallEnemyManager::loadAfter();
}

void TPakkunManager::load(JSUMemoryInputStream& stream)
{
	TSmallEnemyManager::load(stream);
	unk38     = new TPakkunSaveLoadParams("/enemy/pakkun.prm");
	mStayParams = new TPakkunSaveLoadParams("/enemy/staypakkun.prm");
	mWaterEmitInfo     = new TWaterEmitInfo("/enemy/pakkunwater.prm");
	mHideWaterEmitInfo = new TWaterEmitInfo("/enemy/pakkunhide.prm");
}

TPakkunManager::TPakkunManager(const char* name)
    : TSmallEnemyManager(name)
    , mWaterEmitInfo(nullptr)
    , mHideWaterEmitInfo(nullptr)
{
	gpCurPakkun     = nullptr;
	gpCurPakkunSeed = nullptr;
	unk5C           = 0;
}

static int PakkunRootCallback2(J3DNode* node, int flag)
{
	if (flag == 0) {
		TPakkun* pakkun = gpCurPakkun;
		if (!pakkun)
			return 1;

		MtxPtr src = pakkun->mMActor->getModel()->mNodeMatrices
		    [((J3DJoint*)node)->getJntNo()];
		Mtx scale;
		f32 zero    = 0.0f;
		scale[0][3] = zero;
		scale[1][3] = zero;
		scale[2][3] = zero;
		f32 inv     = 1.0f / pakkun->mRootScale;
		scale[0][0] = inv;
		scale[0][1] = zero;
		scale[0][2] = zero;
		scale[1][0] = zero;
		scale[1][1] = inv;
		scale[1][2] = zero;
		scale[2][0] = zero;
		scale[2][1] = zero;
		scale[2][2] = inv;
		PSMTXConcat(scale, src, src);
		PSMTXConcat(j3dSys.mCurrentMtx, scale, j3dSys.mCurrentMtx);
	}
	return 1;
}

static int PakkunRootCallback(J3DNode* node, int flag)
{
	if (flag == 0) {
		TPakkun* pakkun = gpCurPakkun;
		if (!pakkun)
			return 1;

		u8 maxHp = pakkun->getSaveParam()
		               ? pakkun->getSaveParam()->mSLHitPointMax.get()
		               : 1;
		if (pakkun->mHitPoints == maxHp)
			return 1;

		MtxPtr src = pakkun->mMActor->getModel()->mNodeMatrices
		    [((J3DJoint*)node)->getJntNo()];
		Mtx scale;
		f32 zero    = 0.0f;
		scale[0][3] = zero;
		scale[1][3] = zero;
		scale[2][3] = zero;

		f32 root   = pakkun->mRootScale;
		f32 yScale = root;
		f32 zScale = root;
		if (root > 1.0f) {
			yScale *= 1.5f;
			zScale *= 1.5f;
		}

		scale[0][0] = root;
		scale[0][1] = zero;
		scale[0][2] = zero;
		scale[1][0] = zero;
		scale[1][1] = yScale;
		scale[1][2] = zero;
		scale[2][0] = zero;
		scale[2][1] = zero;
		scale[2][2] = zScale;
		PSMTXConcat(scale, src, src);

		scale[0][0] = root;
		scale[0][1] = zero;
		scale[0][2] = zero;
		scale[1][0] = zero;
		scale[1][1] = root;
		scale[1][2] = zero;
		scale[2][0] = zero;
		scale[2][1] = zero;
		scale[2][2] = root;
		PSMTXConcat(j3dSys.mCurrentMtx, scale, j3dSys.mCurrentMtx);
	}
	return 1;
}

static int PakkunSeedCallback(J3DNode* node, int flag)
{
	if (flag == 0) {
		TPakkunSeed* seed = gpCurPakkunSeed;
		if (!seed || seed->unk168 || seed->mHost->unk1B1)
			return 1;

		Mtx rot;
		MtxPtr rotPtr = rot;
		f32 angle = 182.04445f * seed->mRollAngle;
		MtxPtr src = seed->mMActor->getModel()->mNodeMatrices
		    [((J3DJoint*)node)->getJntNo()];
		u16 idx = (u16)(s16)angle;
		f32 s  = jmaSinTable[idx >> jmaSinShift];
		f32 c  = jmaCosTable[idx >> jmaSinShift];
		f32 zero = 0.0f;
		rotPtr[0][0] = c;
		rotPtr[0][1] = -s;
		rotPtr[0][2] = zero;
		rotPtr[0][3] = zero;
		rotPtr[1][0] = s;
		rotPtr[1][1] = c;
		rotPtr[1][2] = zero;
		rotPtr[1][3] = zero;
		rotPtr[2][0] = zero;
		rotPtr[2][1] = zero;
		rotPtr[2][2] = 1.0f;
		rotPtr[2][3] = zero;
		PSMTXConcat(rotPtr, src, src);
		PSMTXConcat(j3dSys.mCurrentMtx, rotPtr, j3dSys.mCurrentMtx);
	}
	return 1;
}
