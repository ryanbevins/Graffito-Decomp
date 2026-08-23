#define J3DMTXCALC_BASIC_INIT_OUT_OF_LINE
#define J3DMTXCALC_MAYA_INIT_OUT_OF_LINE
#include <Enemy/BathtubKiller.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/DirectionCalc.hpp>
#include <Enemy/EffectObj.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MoveBG/MapObjCorona.hpp>
#include <Map/Map.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <System/FlagManager.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/Particles.hpp>
#include <Player/Watergun.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <new>
#undef J3DMTXCALC_MAYA_INIT_OUT_OF_LINE
#undef J3DMTXCALC_BASIC_INIT_OUT_OF_LINE

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

static const char* bathtubkiller_bastable[] = {
	"/scene/bathtubkiller/bas/bathtubdownkiller_down1.bas",
	nullptr,
	nullptr,
};

TBathtubKillerParams::TBathtubKillerParams(const char* prm)
    : TSmallEnemyParams(prm)
    , PARAM_INIT(fastAccelerationQuatRate, 0.05f)
    , PARAM_INIT(fastChaseAcceleration, 0.2f)
    , PARAM_INIT(fastChaseSpeed, 15.0f)
    , PARAM_INIT(fastInitialSpeed, 50.0f)
    , PARAM_INIT(fastDeadPeriod, 1800)
    , PARAM_INIT(shineAccelerationQuatRate, 0.05f)
    , PARAM_INIT(shineChaseAcceleration, 0.2f)
    , PARAM_INIT(shineChaseSpeed, 15.0f)
    , PARAM_INIT(shineInitialSpeed, 50.0f)
    , PARAM_INIT(shineDeadPeriod, 1800)
    , PARAM_INIT(mushroomProbability, 0.05f)
    , PARAM_INIT(mSLColorChangeRateDelta, 0.05f)
    , PARAM_INIT(mSLAccelerationQuatRate, 0.05f)
    , PARAM_INIT(mSLChaseAcceleration, 0.2f)
    , PARAM_INIT(mSLChaseSpeed, 15.0f)
    , PARAM_INIT(mSLInitialSpeed, 20.0f)
    , PARAM_INIT(mSLDeadPeriod, 1800)
    , PARAM_INIT(mSLStraightDistance, 50.0f)
    , PARAM_INIT(mSLChaseMinY, 50.0f)
    , PARAM_INIT(mSLChaseMaxY, 100.0f)
    , PARAM_INIT(mSLAboidDistanceY, 0.05f)
    , PARAM_INIT(mSLAboidDistance, 500.0f)
    , PARAM_INIT(mSLChaseDistanceY, 200.0f)
    , PARAM_INIT(mSLChaseDistance, 1000.0f)
    , PARAM_INIT(mSLTrampleVelocity, 100.0f)
    , PARAM_INIT(mSLFlyingSpeedMax, 200.0f)
    , PARAM_INIT(mSLFlyingGravityY, 0.1f)
    , PARAM_INIT(mSLBombRange, 300.0f)
    , PARAM_INIT(aboidAngle, 10.0f)
    , PARAM_INIT(mSLChaseStraightPeriod, 360)
    , PARAM_INIT(mSLSmokeInterval, 3)
    , PARAM_INIT(mSLLaunchingPeriod, 360)
    , PARAM_INIT(noCollisionAmongKillers, 360)
{
	TParams::load(mPrmPath);

	fastAccelerationQuatRate.set(0.03f);
	fastChaseAcceleration.set(0.1f);
	fastChaseSpeed.set(10.0f);
	fastInitialSpeed.set(14.0f);
	fastDeadPeriod.set(720);
	shineAccelerationQuatRate.set(0.03f);
	shineChaseAcceleration.set(0.03f);
	shineChaseSpeed.set(7.0f);
	shineInitialSpeed.set(12.0f);
	shineDeadPeriod.set(2400);
	mSLAccelerationQuatRate.set(0.03f);
	mSLChaseAcceleration.set(0.1f);
	mSLChaseSpeed.set(8.0f);
	mSLInitialSpeed.set(12.0f);
	mSLDeadPeriod.set(1440);
	mSLColorChangeRateDelta.set(0.16f);
	mSLStraightDistance.set(400.0f);
	mSLChaseMinY.set(50.0f);
	mSLChaseMaxY.set(100.0f);
	mSLAboidDistanceY.set(100.0f);
	mSLAboidDistance.set(100.0f);
	mSLChaseDistanceY.set(400.0f);
	mSLDamageRadius.set(120);
	mSLDamageHeight.set(100);
	mSLAttackRadius.set(100);
	mSLAttackHeight.set(90);
	mSLTrampleVelocity.set(500.0f);
	mSLFlyingSpeedMax.set(200.0f);
	mSLFlyingGravityY.set(0.06f);
	mSLBombRange.set(500.0f);
	aboidAngle.set(5.0f);
	mSLChaseStraightPeriod.set(420);
	mSLSmokeInterval.set(4);
	mSLLaunchingPeriod.set(240);
	noCollisionAmongKillers.set(480);
	mushroomProbability.set(0.3f);
}

TBathtubKiller::TBathtubKiller(const char* name)
    : TSmallEnemy(name)
{
	unk1CC = (TBathtub*)nullptr;
}

void TBathtubKiller::init(TLiveManager* manager)
{
	TSmallEnemy::init(manager);
	mActorType = 0x08000024;
	unk150     = 0x11;
	onLiveFlag(LIVE_FLAG_UNK10);
	onLiveFlag(LIVE_FLAG_DEAD);
	onLiveFlag(LIVE_FLAG_UNK8);
	onHitFlag(HIT_FLAG_NO_COLLISION);
	unk194 = 0;
	resetBathtubKiller();
}

void TBathtubKiller::setMActorAndKeeper()
{
	mMActorKeeper = new TMActorKeeper(mManager, 2);
	mMActor       = mMActorKeeper->createMActor("bathtubkiller_model1.bmd", 0);
	mMActorKeeper->createMActor("bathtubdownkiller_model1.bmd", 3);

	s32 noseMatIdx = getActorKeeper()
	                     ->getMActor("bathtubkiller_model1.bmd")
	                     ->getModel()
	                     ->getModelData()
	                     ->getMaterialName()
	                     ->getIndex("_nosemat1");
	s32 eyesMatIdx = getActorKeeper()
	                     ->getMActor("bathtubkiller_model1.bmd")
	                     ->getModel()
	                     ->getModelData()
	                     ->getMaterialName()
	                     ->getIndex("_eyesmat1");
	s32 bodyMatIdx = getActorKeeper()
	                     ->getMActor("bathtubkiller_model1.bmd")
	                     ->getModel()
	                     ->getModelData()
	                     ->getMaterialName()
	                     ->getIndex("_body1");

	SMS_InitPacket_OneTevColor(
	    getActorKeeper()->getMActor("bathtubkiller_model1.bmd")->getModel(),
	    noseMatIdx, GX_TEVREG0, &unk1E0);
	SMS_InitPacket_OneTevColor(
	    getActorKeeper()->getMActor("bathtubkiller_model1.bmd")->getModel(),
	    eyesMatIdx, GX_TEVREG0, &unk1E8);
	SMS_InitPacket_OneTevColor(
	    getActorKeeper()->getMActor("bathtubkiller_model1.bmd")->getModel(),
	    bodyMatIdx, GX_TEVREG0, &unk1D8);
	SMS_InitPacket_OneTevColor(
	    getActorKeeper()->getMActor("bathtubdownkiller_model1.bmd")->getModel(),
	    0, GX_TEVREG0, &unk1F0);
}

void TBathtubKiller::reset()
{
	TSmallEnemy::reset();
	offLiveFlag(LIVE_FLAG_DEAD);
	offLiveFlag(LIVE_FLAG_UNK8);
	offHitFlag(HIT_FLAG_NO_COLLISION);
	offHitFlag(HIT_FLAG_UNK2);
	offHitFlag(HIT_FLAG_UNK4);
	resetBathtubKiller();
}

#pragma dont_inline on
void TBathtubKiller::resetBathtubKiller()
{
	mSpine->initWith(&TNerveBathtubKillerWander::theNerve());
	onLiveFlag(LIVE_FLAG_AIRBORNE);

	unk208 = 0;
	unk20C = 0;
	unk210 = 0;
	unk214 = 0;
	unk218 = 0;

	mQuat.x = 0.0f;
	mQuat.y = 0.0f;
	mQuat.z = 0.0f;
	mQuat.w = 1.0f;
	mVelocity.set(0.0f, 0.0f, 0.0f);
	unk1BC.set(0.0f, 0.0f, 0.0f);
	unk21C = 0;
	unk1D4 = 0;

	if (unk194 == 1) {
		unk1D8.r = 50;
		unk1D8.g = 70;
		unk1D8.b = 160;
		unk1D8.a = 0;
		unk1E0   = unk1D8;
		unk1E8   = unk1D8;
		unk1F0   = unk1D8;

		TBathtubKillerParams* p = getSaveParam2();
		unk198                  = p->shineAccelerationQuatRate.value;
		unk19C                  = p->shineChaseAcceleration.value;
		unk1A0                  = p->shineChaseSpeed.value;
		unk1A4                  = p->shineInitialSpeed.value;
		unk1A8                  = p->shineDeadPeriod.value;
	} else {
		unk1D8.r = 0;
		unk1D8.g = 0;
		unk1D8.b = 0;
		unk1D8.a = 0;
		unk1E0   = unk1D8;
		unk1E8   = unk1D8;
		unk1F0   = unk1D8;

		if (unk194 == 2) {
			TBathtubKillerParams* p = getSaveParam2();
			unk198                  = p->fastAccelerationQuatRate.value;
			unk19C                  = p->fastChaseAcceleration.value;
			unk1A0                  = p->fastChaseSpeed.value;
			unk1A4                  = p->fastInitialSpeed.value;
			unk1A8                  = p->fastDeadPeriod.value;
		} else {
			TBathtubKillerParams* p = getSaveParam2();
			unk198                  = p->mSLAccelerationQuatRate.value;
			unk19C                  = p->mSLChaseAcceleration.value;
			unk1A0                  = p->mSLChaseSpeed.value;
			unk1A4                  = p->mSLInitialSpeed.value;
			unk1A8                  = p->mSLDeadPeriod.value;
		}
	}

	unk1FC = 0.0f;
	unk1F8 = getSaveParam2()->mSLColorChangeRateDelta.value;
	unk208 = unk1A8;
	unk20C = getSaveParam2()->mSLLaunchingPeriod.value;
	unk214 = getSaveParam2()->noCollisionAmongKillers.value;
	unk200 = getSaveParam2()->mSLChaseMinY.value;
	unk204 = getSaveParam2()->mSLChaseMaxY.value;

	if (unk194 == 2) {
		int sel = (int)(MsRandF() * 4.0f);
		f32 off = 0.0f;
		if (sel == 0)
			off = 120.0f;
		else if (sel == 1)
			off = 240.0f;
		unk200 += off;
		unk204 += off;
	}
}
#pragma dont_inline off

void TBathtubKiller::generateItemBathtubKiller()
{
	if (unk194 != 1)
		return;

	TMapObjBase* item = nullptr;
	TFlagManager* flagManager = TFlagManager::smInstance;
	TBathtubKillerManager* manager = (TBathtubKillerManager*)mManager;
	s32 flag = flagManager->getFlag(0x20001);

	if (((TWaterGun*)SMS_GetMarioWaterGun())->mCurrentWater == 0) {
		item = gpItemManager->makeObjAppear(mPosition.x, mPosition.y,
		                                    mPosition.z, 0x20000002, true);
	} else if (manager->unk60 == flag && manager->unk69 < 7) {
		JGeometry::TVec3<f32> pos = mPosition;
		if (manager->unk64 == nullptr
		    || (manager->unk64->mLiveFlag & LIVE_FLAG_DEAD)) {
			manager->unk64 = gpItemManager->makeObjAppear(
			    pos.x, pos.y, pos.z, 0x20000005, true);
		}
		manager->unk69++;
	} else if (flag <= manager->unk60 + 1
	           && unk1CC->getNumGripsDead() == 3 && manager->unk68 == 0) {
		JGeometry::TVec3<f32> pos = mPosition;
		if (manager->unk64 == nullptr
		    || (manager->unk64->mLiveFlag & LIVE_FLAG_DEAD)) {
			manager->unk64 = gpItemManager->makeObjAppear(
			    pos.x, pos.y, pos.z, 0x20000005, true);
		}
		manager->unk68 = 1;
	}

	if (item == nullptr) {
		item = gpItemManager->makeObjAppear(mPosition.x, mPosition.y,
		                                    mPosition.z, 0x20000002, true);
	}

	if (item != nullptr && item->mActorType == 0x20000002) {
		JPABaseEmitter* emitter =
		    gpMarioParticleManager->emit(0xe5, &item->mPosition, 0, nullptr);
		if (emitter != nullptr) {
			emitter->unk154.x = item->mScaling.x;
			emitter->unk154.y = item->mScaling.y;
			emitter->unk154.z = item->mScaling.z;
			emitter->unk174.x = item->mScaling.x;
			emitter->unk174.y = item->mScaling.y;
			emitter->unk174.z = item->mScaling.z;
		}

		emitter =
		    gpMarioParticleManager->emit(0xe6, &item->mPosition, 0, nullptr);
		if (emitter != nullptr) {
			emitter->unk154.x = item->mScaling.x;
			emitter->unk154.y = item->mScaling.y;
			emitter->unk154.z = item->mScaling.z;
			emitter->unk174.x = item->mScaling.x;
			emitter->unk174.y = item->mScaling.y;
			emitter->unk174.z = item->mScaling.z;
		}
	}
}

inline void TBathtubKiller::killBathtubKiller()
{
	mMActor = mMActorKeeper->getMActor("bathtubdownkiller_model1.bmd");
	setBckAnm(0);

	mQuat.x = 0.0f;
	mQuat.y = 0.0f;
	mQuat.z = 0.0f;
	mQuat.w = 1.0f;
	unk1BC.x = 0.0f;
	unk1BC.y = 0.0f;
	unk1BC.z = 0.0f;

	JGeometry::TVec3<f32> vel(0, 0, 0);
	setVelocity(vel);

	onLiveFlag(LIVE_FLAG_UNK8);
	unk1E0 = unk1D8;
}

static inline JGeometry::TVec3<f32> makeZeroVelocity()
{
	return JGeometry::TVec3<f32>(0, 0, 0);
}

inline void TBathtubKiller::breakBathtubKiller()
{
	mMActor = mMActorKeeper->getMActor("bathtubdownkiller_model1.bmd");
	setBckAnm(0);

	mQuat.x = 0.0f;
	mQuat.y = 0.0f;
	mQuat.z = 0.0f;
	mQuat.w = 1.0f;
	unk1BC.x = 0.0f;
	unk1BC.y = 0.0f;
	unk1BC.z = 0.0f;

	setVelocity(makeZeroVelocity());

	onLiveFlag(LIVE_FLAG_UNK8);
	unk1E0 = unk1D8;

	generateItemBathtubKiller();
	onHitFlag(HIT_FLAG_NO_COLLISION);
}

inline void TBathtubKiller::explodeBathtubKiller()
{
	mMActor = mMActorKeeper->getMActor("bathtubdownkiller_model1.bmd");
	setBckAnm(0);

	mQuat.x = 0.0f;
	mQuat.y = 0.0f;
	mQuat.z = 0.0f;
	mQuat.w = 1.0f;
	unk1BC.x = 0.0f;
	unk1BC.y = 0.0f;
	unk1BC.z = 0.0f;

	setVelocity(makeZeroVelocity());

	onLiveFlag(LIVE_FLAG_UNK8);
	unk1E0 = unk1D8;

	TEffectExplosion* effect = (TEffectExplosion*)gpConductor->makeOneEnemyAppear(
	    mPosition, "エフェクト爆発マネージャー", 1);
	if (effect)
		effect->generate(mPosition, mScaling);

	onHitFlag(HIT_FLAG_NO_COLLISION);
}

void TBathtubKiller::bind()
{
	JGeometry::TVec3<f32> target(mPosition);
	target.add(mLinearVelocity);
	target.add(mVelocity);
	mVelocity.add(unk1BC);

	if (mSpine->getCurrentNerve()
	        != &TNerveBathtubKillerExplosion::theNerve()
	    && mSpine->getCurrentNerve()
	           != &TNerveBathtubKillerBreak::theNerve()) {
		mGroundHeight = gpMap->checkGround(target.x, target.y + mHeadHeight,
		                                   target.z, &mGroundPlane);
		mGroundHeight += 1.0f;
		if (target.y <= 0.05f + mGroundHeight) {
			if (mSpine->getCurrentNerve()
			        != &TNerveBathtubKillerExplosion::theNerve()
			    && mSpine->getCurrentNerve()
			           != &TNerveBathtubKillerBreak::theNerve())
				mSpine->pushNerve(
				    &TNerveBathtubKillerExplosion::theNerve());
			unk1BC.x    = 0.0f;
			unk1BC.y    = 0.0f;
			unk1BC.z    = 0.0f;
			mVelocity.x = unk1BC.x;
			mVelocity.y = unk1BC.y;
			mVelocity.z = unk1BC.z;
			target.y = mGroundHeight;
		}

		if (gpMap->isTouchedOneWallAndMoveXZ(&target.x,
		                                     target.y + mHeadHeight,
		                                     &target.z, mBodyRadius)) {
			if (mSpine->getCurrentNerve()
			        != &TNerveBathtubKillerExplosion::theNerve()
			    && mSpine->getCurrentNerve()
			           != &TNerveBathtubKillerBreak::theNerve())
				mSpine->pushNerve(
				    &TNerveBathtubKillerExplosion::theNerve());
		}
	}

	JGeometry::TVec3<f32> delta(target);
	delta.sub(mPosition);
	mLinearVelocity = delta;
}

void TBathtubKiller::perform(u32 param1, JDrama::TGraphics* graphics)
{
	TSmallEnemy::perform(param1, graphics);

	if (unk1CC == nullptr) {
		JDrama::TNameRef* root = JDrama::TNameRefGen::instance->mRootNameRef;
		unk1CC               = (TBathtub*)root->searchF(
            JDrama::TNameRef::calcKeyCode("バスタブ"), "バスタブ");
	}

	if (param1 & 1) {
		if (!checkLiveFlag(LIVE_FLAG_DEAD)) {
			if (unk208 > 0)
				unk208--;
			if (unk20C > 0)
				unk20C--;
			if (unk210 > 0)
				unk210--;
			if (unk214 > 0)
				unk214--;
			if (unk218 > 0)
				unk218--;

			if (unk208 <= 0) {
				if (mSpine->getCurrentNerve()
				        != &TNerveBathtubKillerExplosion::theNerve()
				    && mSpine->getCurrentNerve()
				           != &TNerveBathtubKillerBreak::theNerve())
					mSpine->pushNerve(
					    &TNerveBathtubKillerExplosion::theNerve());
			}

			if (!gpMap->isInArea(mPosition.x, mPosition.z)) {
				unk21C = 0;
				onLiveFlag(LIVE_FLAG_DEAD);
				stopAnmSound();
			}

			if (unk1CC->getUnk29A() != 0) {
				unk21C = 0;
				onLiveFlag(LIVE_FLAG_DEAD);
				stopAnmSound();
			}
		}
	}

	if (param1 & 2) {
		if (!checkLiveFlag(LIVE_FLAG_DEAD)) {
			if (mSpine->getCurrentNerve()
			        != &TNerveBathtubKillerExplosion::theNerve()
			    && mSpine->getCurrentNerve()
			           != &TNerveBathtubKillerBreak::theNerve()) {
				if (unk194 == 2) {
					unk1FC += unk1F8;
					if (unk1FC > 1.0f) {
						unk1FC = 1.0f;
						unk1F8 = -getSaveParam2()
						              ->mSLColorChangeRateDelta.value;
					}
					if (unk1FC < 0.0f) {
						unk1FC = 0.0f;
						unk1F8 = getSaveParam2()
						             ->mSLColorChangeRateDelta.value;
					}
					unk1E0.r = (u8)(255.0f * unk1FC);
				}

				unk1D4++;
				if (unk1D4 >= getSaveParam2()->mSLSmokeInterval.value) {
					unk1D4 = 0;
					((TPosition3f*)&unk220)->setQT(mQuat, mPosition);
					gpMarioParticleManager->emitAndBindToMtxPtr(
					    0x1bd, (MtxPtr)unk220.mMtx, 1, this);
				}

				f32 dist = JGeometry::TUtil<f32>::sqrt(
				    (mPosition.x - gpMarioPos->x)
				        * (mPosition.x - gpMarioPos->x)
				    + (mPosition.y - gpMarioPos->y)
				          * (mPosition.y - gpMarioPos->y)
				    + (mPosition.z - gpMarioPos->z)
				          * (mPosition.z - gpMarioPos->z));
				if (gpMSound->gateCheck(0x20a9))
					MSoundSESystem::MSoundSE::startSoundActorWithInfo(
					    0x20a9, (Vec*)&mPosition, nullptr, dist, 0, 0,
					    nullptr, 0, 4);
			}
		}
	}
}

void TBathtubKiller::makeInitialVelocity(JGeometry::TVec3<f32> vel)
{
	f32 speed    = vel.length();
	f32 maxSpeed = getSaveParam2()->mSLFlyingSpeedMax.value;
	if (speed > maxSpeed) {
		vel.normalize();
		vel.scale(maxSpeed);
	}
	mVelocity.set(vel);

	vel.normalize();

	JGeometry::TVec3<f32> forward;
	mQuat.getZDir(forward);
	JGeometry::TQuat4<f32> rot;
	rot.setRotate(forward, vel);
	mQuat.mul(rot);
}

void TBathtubKiller::moveChasing()
{
	JGeometry::TVec3<f32> chasePoint(*gpMarioPos);
	f32 minY = unk200 + (*unk1CC->getRootJointMtx())[1][3];
	f32 maxY = unk204 + (*unk1CC->getRootJointMtx())[1][3];
	chasePoint.y = 0.5f * (minY + maxY);

	JGeometry::TVec3<f32> dir;
	dir.sub(chasePoint, mPosition);
	dir.normalize();
	unk1BC.scale(unk19C, dir);
	makeQuat(unk1BC, unk198, 0.1f);

	JGeometry::TVec3<f32> forward;
	mQuat.getZDir(forward);
	forward.normalize();
	if (mPosition.y > maxY) {
		if (!(0.0f >= forward.y))
			forward.y = 0.0f;
	}
	if (mPosition.y < minY)
		forward.y = (0.0f >= forward.y) ? 0.0f : forward.y;
	mVelocity.scale(unk1A0, forward);
}

void TBathtubKiller::moveStraight()
{
	JGeometry::TVec3<f32> dir;
	mQuat.getZDir(dir);
	dir.y = 0.0f;
	dir.normalize();
	dir.scale(unk1A0);
	mVelocity.set(dir);
	makeQuat(mVelocity, unk198, 0.1f);
}

void TBathtubKiller::makeQuat(JGeometry::TVec3<f32> axis, f32 moveAmountY,
                              f32 moveAmountX)
{
	JGeometry::TVec3<f32> normAxis = axis;
	normAxis.normalize();

	JGeometry::TVec3<f32> forward;
	mQuat.getZDir(forward);

	JGeometry::TVec3<f32> up;
	mQuat.getYDir(up);

	JGeometry::TQuat4<f32> steer;
	steer.setRotate(forward, normAxis, moveAmountY);
	mQuat.mul(steer);

	// Y-axis rotation
	JGeometry::TVec3<f32> right;
	right.cross(forward, JGeometry::TVec3<f32>(0.0f, 1.0f, 0.0f));
	if (right.length() > 0.0f) {
		right.normalize();

		JGeometry::TQuat4<f32> tiltQuat;
		tiltQuat.setRotate(right, M_PI / 2.0f);

		JGeometry::TVec3<f32> curUp;
		tiltQuat.rotate(forward, curUp);

		steer.setRotate(up, curUp, moveAmountX);
		mQuat.mul(steer);
	}

	mQuat.normalize();
}

f32 TBathtubKiller::getGravityY() const
{
	return getSaveParam2()->mSLFlyingGravityY.value;
}

void TBathtubKiller::calcRootMatrix()
{
	TPosition3f mtx;

	mtx.setQT(mQuat, mPosition);
	getModel()->setBaseScale(mScaling);
	getModel()->setBaseTRMtx(mtx);
}

BOOL TBathtubKiller::receiveMessage(THitActor* sender, u32 msg)
{
	if (msg == 3 || msg <= 1) {
		const TNerveBase<TLiveActor>* nerve = mSpine->getCurrentNerve();
		bool isBroken
		    = nerve == &TNerveBathtubKillerExplosion::theNerve()
		      || nerve == &TNerveBathtubKillerBreak::theNerve();
		if (!isBroken)
			mSpine->pushNerve(&TNerveBathtubKillerBreak::theNerve());
		return TRUE;
	} else if (msg == 0xa) {
		const TNerveBase<TLiveActor>* nerve = mSpine->getCurrentNerve();
		bool isBroken
		    = nerve == &TNerveBathtubKillerExplosion::theNerve()
		      || nerve == &TNerveBathtubKillerBreak::theNerve();
		if (!isBroken)
			mSpine->pushNerve(&TNerveBathtubKillerExplosion::theNerve());
		return TRUE;
	} else if (msg == 0xd) {
		kill();
		return TRUE;
	} else if (msg == 0xf) {
		behaveToWater(sender);
		return TRUE;
	}
	return FALSE;
}

void TBathtubKiller::attackToMario()
{
	const TNerveBase<TLiveActor>* nerve = mSpine->getCurrentNerve();
	bool isBroken = nerve == &TNerveBathtubKillerExplosion::theNerve()
	                || nerve == &TNerveBathtubKillerBreak::theNerve();
	if (!isBroken && gpMarioPos->y < mPosition.y) {
		mSpine->pushNerve(&TNerveBathtubKillerExplosion::theNerve());
		SMS_SendMessageToMario(this, 0xe);
		SMS_ThrowMario(JGeometry::TVec3<f32>(0.0f, 1.0f, 0.0f), 60.0f);
		unk21C = 1;
	}
}

bool TBathtubKiller::isCollidMove(THitActor* sender)
{
	const TNerveBase<TLiveActor>* nerve = mSpine->getCurrentNerve();
	bool isBroken = nerve == &TNerveBathtubKillerExplosion::theNerve()
	                || nerve == &TNerveBathtubKillerBreak::theNerve();
	if (isBroken)
		return false;

	if (sender->isActorType(0x8000029)) {
		bool isBroken
		    = mSpine->getCurrentNerve()
		          == &TNerveBathtubKillerExplosion::theNerve()
		      || mSpine->getCurrentNerve()
		             == &TNerveBathtubKillerBreak::theNerve();
		if (!isBroken)
			mSpine->pushNerve(&TNerveBathtubKillerExplosion::theNerve());
		return true;
	}

	if (sender->isActorType(0x8000021) || sender->isActorType(0x800002a)
	    || sender->isActorType(0x800002c)) {
		bool isBroken
		    = mSpine->getCurrentNerve()
		          == &TNerveBathtubKillerExplosion::theNerve()
		      || mSpine->getCurrentNerve()
		             == &TNerveBathtubKillerBreak::theNerve();
		if (!isBroken)
			mSpine->pushNerve(&TNerveBathtubKillerExplosion::theNerve());
		sender->receiveMessage(this, 0xe);
		return true;
	}

	if (sender->isActorType(0x8000024)) {
		if (unk214 <= 0) {
			bool isBroken
			    = mSpine->getCurrentNerve()
			          == &TNerveBathtubKillerExplosion::theNerve()
			      || mSpine->getCurrentNerve()
			             == &TNerveBathtubKillerBreak::theNerve();
			if (!isBroken)
				mSpine->pushNerve(
				    &TNerveBathtubKillerExplosion::theNerve());
		}
	}

	return true;
}

void TBathtubKiller::behaveToWater(THitActor*)
{
	bool isBroken
	    = mSpine->getCurrentNerve()
	          == &TNerveBathtubKillerExplosion::theNerve()
	      || mSpine->getCurrentNerve()
	             == &TNerveBathtubKillerBreak::theNerve();
	if (!isBroken)
		mSpine->pushNerve(&TNerveBathtubKillerBreak::theNerve());
}

const char** TBathtubKiller::getBasNameTable() const
{
	return bathtubkiller_bastable;
}

inline void TBathtubKiller::setNormalBathtubKillerAnm()
{
	mMActor = mMActorKeeper->getMActor("bathtubkiller_model1.bmd");
	setBckAnm(1);
}

inline void TBathtubKiller::setStraightBathtubKillerAnm()
{
	mMActor = mMActorKeeper->getMActor("bathtubkiller_model1.bmd");
	setBckAnm(2);
}

bool TBathtubKiller::isAboided()
{
	f32 ceiling = unk204 + (*unk1CC->getRootJointMtx())[1][3];
	if (mPosition.y > 5.0f + ceiling)
		return false;

	JGeometry::TVec3<f32> toMario(*gpMarioPos);
	JGeometry::TVec3<f32> selfPos(mPosition);
	f32 vertDist = __fabsf(toMario.y - selfPos.y);
	toMario.y = 0.0f;
	selfPos.y = 0.0f;
	f32 diffX = toMario.x - selfPos.x;
	f32 diffY = toMario.y - selfPos.y;
	f32 diffZ = toMario.z - selfPos.z;
	f32 horizDist
	    = JGeometry::TUtil<f32>::sqrt(diffX * diffX + diffY * diffY
	                                  + diffZ * diffZ);

	if (vertDist > getSaveParam2()->mSLAboidDistanceY.value
	    && horizDist <= getSaveParam2()->mSLAboidDistance.value)
		return true;

	if (horizDist > getSaveParam2()->mSLStraightDistance.value)
		return false;

	if (SMS_GetMarioStatus() == 0x3800034B) {
		unk218 = 240;
		unk64 |= 1;
		return true;
	}

	JGeometry::TVec3<f32> dir(diffX, diffY, diffZ);
	dir.normalize();
	TDirectionCalc dc1(dir);

	JGeometry::TVec3<f32> forward;
	mQuat.getZDir(forward);
	TDirectionCalc dc2(forward);

	f32 ang = dc2.absDirection(dc1.mDirection);
	if (ang > TDirectionCalc::d2r(getSaveParam2()->aboidAngle.value))
		return false;
	return true;
}

DEFINE_NERVE(TNerveBathtubKillerWander, TLiveActor)
{
	TBathtubKiller* self = (TBathtubKiller*)spine->getBody();
	if (spine->getTime() == 0)
		self->setNormalBathtubKillerAnm();

	if (!self->unk1CC->isKillerAttackable())
		return FALSE;

	bool wantStraight;
	if (self->unk194 == 2) {
		JGeometry::TVec3<f32> mp(*gpMarioPos);
		mp.y = 0.0f;
		JGeometry::TVec3<f32> sp(self->mPosition);
		sp.y = 0.0f;
		JGeometry::TVec3<f32> bp(self->unk1CC->mPosition);
		bp.y = 0.0f;
		f32 mdx = mp.x - bp.x;
		f32 mdy = mp.y - bp.y;
		f32 mdz = mp.z - bp.z;
		f32 dM = JGeometry::TUtil<f32>::sqrt(
		    mdx * mdx + mdy * mdy + mdz * mdz);
		f32 sdx = sp.x - bp.x;
		f32 sdy = sp.y - bp.y;
		f32 sdz = sp.z - bp.z;
		f32 dS = JGeometry::TUtil<f32>::sqrt(
		    sdx * sdx + sdy * sdy + sdz * sdz);
		wantStraight = dS <= 100.0f + dM;
	} else {
		wantStraight = true;
	}

	if (!wantStraight) {
		spine->pushNerve(&TNerveBathtubKillerStraight::theNerve());
		return TRUE;
	}

	bool wantChase;
	if (self->unk20C > 0) {
		wantChase = false;
	} else {
		f32 height = self->getSaveParam2()->mSLChaseDistanceY.value;
		f32 bathtubY = (*self->unk1CC->getRootJointMtx())[1][3];
		wantChase = self->mPosition.y <= self->unk200 + bathtubY + height;
	}

	if (wantChase) {
		spine->pushNerve(&TNerveBathtubKillerChase::theNerve());
		return TRUE;
	}

	f32 gravity = self->getGravityY();
	self->unk1BC.set(0.0f, -gravity, 0.0f);
	self->makeQuat(self->mVelocity, 1.0f, 0.1f);
	return FALSE;
}

DEFINE_NERVE(TNerveBathtubKillerChase, TLiveActor)
{
	TBathtubKiller* self = (TBathtubKiller*)spine->getBody();
	if (spine->getTime() == 0)
		self->setNormalBathtubKillerAnm();

	if (!self->unk1CC->isKillerAttackable())
		return FALSE;

	bool wantStraight;
	if (self->unk194 == 2) {
		JGeometry::TVec3<f32> mp(*gpMarioPos);
		mp.y = 0.0f;
		JGeometry::TVec3<f32> sp(self->mPosition);
		sp.y = 0.0f;
		JGeometry::TVec3<f32> bp(self->unk1CC->mPosition);
		bp.y = 0.0f;
		f32 dM = mp.distance(bp);
		f32 dS = sp.distance(bp);
		wantStraight = dS <= 100.0f + dM;
	} else {
		wantStraight = true;
	}

	if (!wantStraight) {
		spine->pushNerve(&TNerveBathtubKillerStraight::theNerve());
		return TRUE;
	}

	if (self->isAboided()) {
		if (self->unk194 == 1)
			spine->pushNerve(&TNerveBathtubKillerChaseStraight::theNerve());
		else
			spine->pushNerve(&TNerveBathtubKillerStraight::theNerve());
		return TRUE;
	}

	self->moveChasing();
	return FALSE;
}

DEFINE_NERVE(TNerveBathtubKillerChaseStraight, TLiveActor)
{
	TBathtubKiller* self = (TBathtubKiller*)spine->getBody();
	if (spine->getTime() == 0) {
		self->setStraightBathtubKillerAnm();
		self->unk210 = self->getSaveParam2()->mSLChaseStraightPeriod.value;
	}

	if (!self->unk1CC->isKillerAttackable())
		return FALSE;

	bool wantStraight;
	if (self->unk194 == 2) {
		JGeometry::TVec3<f32> mp(*gpMarioPos);
		mp.y = 0.0f;
		JGeometry::TVec3<f32> sp(self->mPosition);
		sp.y = 0.0f;
		JGeometry::TVec3<f32> bp(self->unk1CC->mPosition);
		bp.y = 0.0f;
		f32 mdx = mp.x - bp.x;
		f32 mdy = mp.y - bp.y;
		f32 mdz = mp.z - bp.z;
		f32 dM = JGeometry::TUtil<f32>::sqrt(
		    mdx * mdx + mdy * mdy + mdz * mdz);
		f32 sdx = sp.x - bp.x;
		f32 sdy = sp.y - bp.y;
		f32 sdz = sp.z - bp.z;
		f32 dS = JGeometry::TUtil<f32>::sqrt(
		    sdx * sdx + sdy * sdy + sdz * sdz);
		wantStraight = dS <= 100.0f + dM;
	} else {
		wantStraight = true;
	}

	if (!wantStraight) {
		spine->pushNerve(&TNerveBathtubKillerStraight::theNerve());
		return TRUE;
	}

	if (self->unk218 <= 0)
		self->unk64 &= ~1;

	if (self->unk210 <= 0) {
		spine->pushNerve(&TNerveBathtubKillerChase::theNerve());
		return TRUE;
	}

	self->moveStraight();
	return FALSE;
}

DEFINE_NERVE(TNerveBathtubKillerStraight, TLiveActor)
{
	TBathtubKiller* self = (TBathtubKiller*)spine->getBody();
	if (spine->getTime() == 0)
		self->setStraightBathtubKillerAnm();

	if (self->unk218 <= 0)
		self->unk64 &= ~1;

	self->moveStraight();
	return FALSE;
}

DEFINE_NERVE(TNerveBathtubKillerBreak, TLiveActor)
{
	TBathtubKiller* self = (TBathtubKiller*)spine->getBody();
	if (spine->getTime() == 0)
		self->breakBathtubKiller();

	if (self->checkCurAnmEnd(0)) {
		self->unk21C = 0;
		self->onLiveFlag(LIVE_FLAG_DEAD);
		self->stopAnmSound();
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveBathtubKillerExplosion, TLiveActor)
{
	TBathtubKiller* self = (TBathtubKiller*)spine->getBody();
	if (spine->getTime() == 0)
		self->explodeBathtubKiller();

	if (self->checkCurAnmEnd(0)) {
		self->unk21C = 0;
		self->onLiveFlag(LIVE_FLAG_DEAD);
		self->stopAnmSound();
		return TRUE;
	}
	return FALSE;
}

TBathtubKillerManager::TBathtubKillerManager(const char* name)
    : TSmallEnemyManager(name)
{
}

#define ASSERT_MSG(msg, line) (void)((msg), (line))
#define ASSERT_TEST(expr)                                                      \
	(void)((expr) ? true : (ASSERT_MSG(__FILE__, __LINE__), false));

void TBathtubKillerManager::load(JSUMemoryInputStream& stream)
{
	ASSERT_TEST(unk38);
	TSmallEnemyManager::load(stream);
	unk38 = new TBathtubKillerParams("/enemy/bathtubkiller.prm");
	ASSERT_TEST(unk38);
}

void TBathtubKillerManager::loadAfter()
{
	TSmallEnemyManager::loadAfter();
	TMapObjBaseManager::newAndRegisterObj("mushroom1up");
	TMapObjBaseManager::newAndRegisterObj("mushroom1up");
	unk60 = TFlagManager::smInstance->getFlag(0x20001);
	unk64 = nullptr;
	unk68 = 0;
	unk69 = 0;
	ASSERT_TEST(unk38);

	static const char* loopFilenames[] = {
		"/scene/map/map/ms_kp_kill_smoke.jpa",
	};
	bool* particleFlag = &gParticleFlagLoaded[0x1bd];
	if (!*particleFlag) {
		gpResourceManager->load(loopFilenames[0], 0x1bd);
		*particleFlag = true;
	}
}

int TBathtubKillerManager::countActiveKillers()
{
	int count = 0;
	for (int i = 0;
	     i < (unk38 == nullptr ? mObjNum
	                        : (unk38->mSLActiveEnemyNum.value > mObjNum
	                               ? mObjNum
	                               : unk38->mSLActiveEnemyNum.value));
	     i++) {
		if (!(((TLiveActor*)unk18[i])->mLiveFlag & 1))
			count++;
	}
	return count;
}

void TBathtubKillerManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "bathtubkiller_model1.bmd", 0x50230000, 0 },
		{ "bathtubdownkiller_model1.bmd", 0x50210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TSpineEnemy* TBathtubKillerManager::createEnemyInstance()
{
	TBathtubKiller* k = new TBathtubKiller;
	return k;
}
