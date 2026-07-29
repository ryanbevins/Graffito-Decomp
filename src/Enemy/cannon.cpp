#include <Enemy/Cannon.hpp>
#include <Enemy/Bombhei.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/EffectObj.hpp>
#include <Enemy/Graph.hpp>
#include <Enemy/Igaiga.hpp>
#include <Enemy/Killer.hpp>
#include <Enemy/Popo.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/SDLModel.hpp>
#include <Map/MapCollisionEntry.hpp>
#define MAP_COLLISION_ENTRY_DEFINE_SET_UP_TRANS
#include <Map/MapCollisionEntry.hpp>
#undef MAP_COLLISION_ENTRY_DEFINE_SET_UP_TRANS
#include <MoveBG/MapObjDolpic.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <System/Application.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Strategy.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <dolphin/mtx.h>
#include <stdlib.h>

static const char* cannon_bastable[] = {
	nullptr,
	nullptr,
	"/scene/cannon/bas/CannonDom_break.bas",
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	"/scene/cannon/bas/tyorobe_appear1.bas",
	"/scene/cannon/bas/tyorobe_damage1.bas",
	"/scene/cannon/bas/tyorobe_down1.bas",
	"/scene/cannon/bas/tyorobe_hyde1.bas",
	"/scene/cannon/bas/tyorobe_throw1.bas",
	nullptr,
	nullptr,
	nullptr,
};

u8 TCannon::mChorobeiJntIdx     = 4;
u8 TCannon::mChorobeiHandJntIdx = 4;
f32 TCannon::mVelocityRate      = 0.62f;
f32 TCannon::mSearchRate        = 0.02f;

DEFINE_NERVE(TNerveCannonObject, TLiveActor)
{
	TCannon* self = (TCannon*)spine->getBody();

	if (spine->getTime() == 0) {
		if (!self->isBckAnm(4)) {
			self->setBckAnm(4);
			J3DFrameCtrl* ctrl = self->getMActor()->getFrameCtrl(0);
			ctrl->setFrame((f32)ctrl->getEnd());
		}

		if (self->unk1A8 != nullptr)
			self->unk1A8->unk70 = 1.0f;
	}

	if (self->isBckAnm(4)) {
		if (self->getMActor()->getFrameCtrl(0)->checkPass(60.0f)) {
			if (gpMSound->gateCheck(0x38B4))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x38B4, &self->mPosition, 0, nullptr, 0, 4);
		}
	}

	if (spine->getTime() > 150 && self->unk254 != nullptr) {
		if (self->unk254->checkLiveFlag(LIVE_FLAG_DEAD)) {
			self->unk254->appear();
			self->unk254->mPosition = self->mPosition;
			self->unk254->mScaling.set(0.27f, 0.02f, 0.27f);
			self->unk254->getMActor()->setBtk("maregate");
		}

		f32 scaleY = 1.01f * self->unk254->mScaling.y;
		if (scaleY > 0.22f)
			scaleY = 0.22f;
		else if (scaleY < 0.0f)
			scaleY = 0.0f;
		self->unk254->mScaling.y = scaleY;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveCannonDamageDemo, TLiveActor)
{
	TCannon* self = (TCannon*)spine->getBody();

	if (spine->getTime() == 0) {
		TChorobei* chorobei = self->unk1A8;
		chorobei->unk6C->getMActor()->setBckFromIndex(14);
		const char** bas = chorobei->unk68->getBasNameTable();
		chorobei->unk78 = bas == nullptr ? nullptr : bas[14];
		if (chorobei->unk78 != nullptr) {
			chorobei->unk74->initAnmSound(
			    JKRFileLoader::getGlbResource(chorobei->unk78), 1, 0.0f);
		} else {
			chorobei->unk74->initAnmSound(nullptr, 1, 0.0f);
		}
	}

	if (spine->getTime() > 120
	    && self->unk1A8->unk6C->getMActor()->curAnmEndsNext(0, nullptr)
	    && !self->isBckAnm(4)) {
		self->setBckAnm(4);

		if (gpMSound->gateCheck(0x38B3))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x38B3, &self->mPosition, 0, nullptr, 0, 4);

		TEffectExplosion* effect
		    = (TEffectExplosion*)gpConductor->makeOneEnemyAppear(
		        self->mPosition, "エフェクト爆発マネージャー", 1);
		if (effect != nullptr) {
			JGeometry::TVec3<f32> scale(2.5f, 2.5f, 2.5f);
			effect->generate(self->mPosition, scale);
		}

		TFlagManager::smInstance->setBool(true, 0x5000C);
		spine->pushAfterCurrent(&TNerveCannonObject::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveCannonDamage, TLiveActor)
{
	TCannon* self = (TCannon*)spine->getBody();

	if (spine->getTime() == 0) {
		JPABaseEmitter* damageEmitter
		    = gpMarioParticleManager->emitAndBindToPosPtr(
		        0xC4, &self->mPosition, 0, nullptr);
		if (damageEmitter != nullptr) {
			JGeometry::TVec3<f32> scale(2.0f, 2.0f, 2.0f);
			damageEmitter->setScale(scale);
		}
		gpMarioParticleManager->emitAndBindToPosPtr(0xC5, &self->mPosition,
		                                            0, nullptr);
		if (damageEmitter != nullptr) {
			JGeometry::TVec3<f32> scale(2.0f, 2.0f, 2.0f);
			damageEmitter->setScale(scale);
		}
		gpMarioParticleManager->emitAndBindToPosPtr(0xC6, &self->mPosition,
		                                            0, nullptr);
		if (damageEmitter != nullptr) {
			JGeometry::TVec3<f32> scale(2.0f, 2.0f, 2.0f);
			damageEmitter->setScale(scale);
		}

		if (self->mHitPoints != 0)
			self->mHitPoints--;

		TChorobei* chorobei = self->unk1A8;
		if (self->mHitPoints == 0) {
			if (self->unk1A4 != nullptr)
				self->unk1A4->forceKill();

			chorobei->unk6C->getMActor()->setBckFromIndex(13);
			const char** bas = chorobei->unk68->getBasNameTable();
			chorobei->unk78 = bas == nullptr ? nullptr : bas[13];
			if (chorobei->unk78 != nullptr) {
				chorobei->unk74->initAnmSound(
				    JKRFileLoader::getGlbResource(chorobei->unk78), 1, 0.0f);
			} else {
				chorobei->unk74->initAnmSound(nullptr, 1, 0.0f);
			}

			if (gpApplication.mCurrArea.unk0 == 5)
				SMSRumbleMgr->start(0x18, (f32*)nullptr);
			else
				SMSRumbleMgr->start(0x17, (f32*)nullptr);

			JPABaseEmitter* emitter
			    = gpMarioParticleManager->emitAndBindToMtxPtr(
			        0xC8,
			        chorobei->unk6C->getMActor()->getModel()->getAnmMtx(12),
			        0, nullptr);
			if (emitter != nullptr)
				emitter->setScale(chorobei->mScaling);

			emitter = gpMarioParticleManager->emitAndBindToPosPtr(
			    0xC7, &self->unk294, 0, nullptr);
			if (emitter != nullptr)
				emitter->setScale(chorobei->mScaling);

			if (gpApplication.mCurrArea.unk0 == 5) {
				self->unk2A0 = self->mPosition;
				self->unk2A0.y = 0.0f;
				gpMarDirector->fireStartDemoCamera(
				    "tyorocam_pinna", &self->unk2A0, -1, self->mRotation.y,
				    true, nullptr, 0, nullptr, JDrama::TFlagT<u16>(0));
			} else {
				gpMarDirector->fireStartDemoCamera(
				    "tyorocam_mare", nullptr, -1, 0.0f, true, nullptr, 0,
				    nullptr, JDrama::TFlagT<u16>(0));
			}

			self->onHitFlag(HIT_FLAG_NO_COLLISION);
			chorobei->onHitFlag(HIT_FLAG_NO_COLLISION);
		} else {
			self->setDeadAnm();

			chorobei->unk6C->getMActor()->setBckFromIndex(13);
			const char** bas = chorobei->unk68->getBasNameTable();
			chorobei->unk78 = bas == nullptr ? nullptr : bas[13];
			if (chorobei->unk78 != nullptr) {
				chorobei->unk74->initAnmSound(
				    JKRFileLoader::getGlbResource(chorobei->unk78), 1, 0.0f);
			} else {
				chorobei->unk74->initAnmSound(nullptr, 1, 0.0f);
			}

			MtxPtr mtx = chorobei->unk6C->getMActor()->getModel()->getAnmMtx(0);
			self->unk294.set(mtx[0][3], mtx[1][3], mtx[2][3]);

			JPABaseEmitter* emitter
			    = gpMarioParticleManager->emitAndBindToPosPtr(
			        0xC7, &self->unk294, 0, nullptr);
			if (emitter != nullptr)
				emitter->setScale(chorobei->mScaling);
		}

		self->mVelocity.set(0.0f, 4.0f, 0.0f);
		self->onLiveFlag(LIVE_FLAG_AIRBORNE);
		self->mPosition.y += 10.0f;
	}

	MActor* chorobeiActor = self->unk1A8->unk6C->getMActor();
	if (chorobeiActor->curAnmEndsNext(0, nullptr)) {
		SMS_ResetDamageFogEffect(
		    chorobeiActor->getModel()->getModelData());
		if (self->mHitPoints == 0) {
			spine->pushAfterCurrent(&TNerveCannonDamageDemo::theNerve());
			return TRUE;
		}

		spine->pushAfterCurrent(&TNerveCannonSearch::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveCannonClose, TLiveActor)
{
	TCannon* self = (TCannon*)spine->getBody();

	if (spine->getTime() < 2) {
		self->onHitFlag(HIT_FLAG_NO_COLLISION);
		self->unk1A8->onHitFlag(HIT_FLAG_NO_COLLISION);

		TChorobei* chorobei = self->unk1A8;
		chorobei->unk6C->getMActor()->setBckFromIndex(15);
		const char** bas = chorobei->unk68->getBasNameTable();
		chorobei->unk78 = bas == nullptr ? nullptr : bas[15];
		if (chorobei->unk78 != nullptr) {
			chorobei->unk74->initAnmSound(
			    JKRFileLoader::getGlbResource(chorobei->unk78), 1, 0.0f);
		} else {
			chorobei->unk74->initAnmSound(nullptr, 1, 0.0f);
		}

		self->unk294 = self->mPosition;
		self->unk294.y += 300.0f;
		gpMarioParticleManager->emitAndBindToPosPtr(
		    0xC9, &self->unk294, 0, nullptr);
	}

	TChorobei* chorobei = self->unk1A8;
	bool hidden;
	if (chorobei->unk6C->getMActor()->curAnmEndsNext(0, nullptr)
	    && chorobei->unk6C->getMActor()->checkCurBckFromIndex(15)) {
		chorobei->unk70 = 1.0f;
		hidden          = true;
	} else {
		hidden = false;
	}

	if (hidden && !self->isBckAnm(0))
		self->setBckAnm(0);

	if (self->isBckAnm(0)
	    && self->getMActor()->getFrameCtrl(0)->checkPass(10.0f)) {
		self->unk294 = self->mPosition;
		self->unk294.y += 290.0f;
		JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToPosPtr(
		    0x11, &self->unk294, 0, nullptr);
		if (emitter != nullptr) {
			JGeometry::TVec3<f32> scale(1.5f, 1.5f, 1.5f);
			emitter->setScale(scale);
		}
	}

	self->updateSquareToMario();
	f32 hideDist = 3.0f * self->unk28C->mSLHideDist.get();
	if (self->mDistToMarioSquared > hideDist * hideDist) {
		self->offHitFlag(HIT_FLAG_NO_COLLISION);
		self->unk1A8->offHitFlag(HIT_FLAG_NO_COLLISION);
		spine->pushAfterCurrent(&TNerveCannonOpen::theNerve());
		return TRUE;
	}

	self->unk2B0->moveMtx(self->getMActor()->getModel()->getAnmMtx(4));
	return FALSE;
}

DEFINE_NERVE(TNerveCannonForceBombShoot, TLiveActor)
{
	TCannon* self = (TCannon*)spine->getBody();

	if (spine->getTime() == 0) {
		f32 bombDist = self->unk28C->mSLBombDist.get();
		f32 limit    = 2.0f * (bombDist * bombDist);
		if (self->mDistToMarioSquared < limit) {
			TChorobei* chorobei = self->unk1A8;
			chorobei->unk6C->getMActor()->setBckFromIndex(17);
			const char** bas = chorobei->unk68->getBasNameTable();
			chorobei->unk78 = bas == nullptr ? nullptr : bas[17];
			if (chorobei->unk78 != nullptr) {
				chorobei->unk74->initAnmSound(
				    JKRFileLoader::getGlbResource(chorobei->unk78), 1,
				    0.0f);
			} else {
				chorobei->unk74->initAnmSound(nullptr, 1, 0.0f);
			}
		} else {
			return TRUE;
		}
	}

	if (self->unk1A8->unk6C->getMActor()->checkCurBckFromIndex(17)) {
		if (self->unk1A8->unk6C->getMActor()->curAnmEndsNext(0, nullptr)) {
			TChorobei* chorobei = self->unk1A8;
			chorobei->unk6C->getMActor()->setBckFromIndex(16);
			const char** bas = chorobei->unk68->getBasNameTable();
			chorobei->unk78 = bas == nullptr ? nullptr : bas[16];
			if (chorobei->unk78 != nullptr) {
				chorobei->unk74->initAnmSound(
				    JKRFileLoader::getGlbResource(chorobei->unk78), 1, 0.0f);
			} else {
				chorobei->unk74->initAnmSound(nullptr, 1, 0.0f);
			}
			self->bombSet();
		}
		self->walkToCurPathNode(0.0f, self->mTurnSpeed, 0.0f);
	} else if (self->unk1A8->unk6C->getMActor()->checkCurBckFromIndex(16)) {
		if (self->unk1A8->unk6C->getMActor()->curAnmEndsNext(0, nullptr))
			return TRUE;

		if (self->unk1A8->unk6C->getMActor()->getFrameCtrl(0)->checkPass(38.0f))
			self->bombShoot();

		if (self->unk1A8->unk6C->getMActor()->getFrameCtrl(0)->getFrame() > 26.0f
		    && self->unk1A4 != nullptr) {
			f32 max        = self->unk220;
			f32 scaleDelta = 0.2f * max;
			f32 scale = MsClamp<f32>(self->unk1A4->mScaling.x + scaleDelta,
			                        0.0f, max);
			self->unk1A4->mScaling.x = scale;
			self->unk1A4->mScaling.set(self->mScaling.x, self->mScaling.x,
			                           self->mScaling.x);
		}
	}

	return FALSE;
}

DEFINE_NERVE(TNerveCannonShoot, TLiveActor)
{
	TCannon* self = (TCannon*)spine->getBody();

	if (spine->getTime() == 0) {
		if (!self->unk290) {
			self->setKillerGoalPoint();
		} else {
			TChorobei* chorobei = self->unk1A8;
			chorobei->unk6C->getMActor()->setBckFromIndex(17);
			const char** bas = chorobei->unk68->getBasNameTable();
			chorobei->unk78 = bas == nullptr ? nullptr : bas[17];
			if (chorobei->unk78 != nullptr) {
				chorobei->unk74->initAnmSound(
				    JKRFileLoader::getGlbResource(chorobei->unk78), 1, 0.0f);
			} else {
				chorobei->unk74->initAnmSound(nullptr, 1, 0.0f);
			}
		}
	}

	if (self->unk290) {
		if (self->unk1A8->unk6C->getMActor()->checkCurBckFromIndex(17)) {
			if (self->unk1A8->unk6C->getMActor()->curAnmEndsNext(0, nullptr)) {
				TChorobei* chorobei = self->unk1A8;
				chorobei->unk6C->getMActor()->setBckFromIndex(16);
				const char** bas = chorobei->unk68->getBasNameTable();
				chorobei->unk78 = bas == nullptr ? nullptr : bas[16];
				if (chorobei->unk78 != nullptr) {
					chorobei->unk74->initAnmSound(
					    JKRFileLoader::getGlbResource(chorobei->unk78), 1,
					    0.0f);
				} else {
					chorobei->unk74->initAnmSound(nullptr, 1, 0.0f);
				}
				self->bombSet();
			}
			self->walkToCurPathNode(0.0f, self->mTurnSpeed, 0.0f);
			return FALSE;
		} else if (self->unk1A8->unk6C->getMActor()->checkCurBckFromIndex(16)) {
			if (self->unk1A8->unk6C->getMActor()->curAnmEndsNext(0, nullptr)) {
				spine->pushAfterCurrent(&TNerveCannonSearch::theNerve());
				return TRUE;
			}

			if (self->unk1A8->unk6C->getMActor()->getFrameCtrl(0)->checkPass(38.0f))
				self->bombShoot();

			if (self->unk1A8->unk6C->getMActor()->getFrameCtrl(0)->getFrame() > 26.0f
			    && self->unk1A4 != nullptr) {
				f32 scaleDelta = 0.2f * self->unk220;
				f32 scale      = self->unk1A4->mScaling.x;
				scale += scaleDelta;
				if (scale > self->unk220)
					scale = self->unk220;
				if (scale < 0.0f)
					scale = 0.0f;
				self->unk1A4->mScaling.x = scale;
				self->unk1A4->mScaling.set(self->mScaling.x, self->mScaling.x,
				                           self->mScaling.x);
			}
		}
	} else {
		if (spine->getTime() < 40)
			self->walkToCurPathNode(0.0f, self->mTurnSpeed, 0.0f);

		if (spine->getTime() == 40)
			self->killerShoot();

		if (spine->getTime() > self->unk28C->mSLKillerInterval.get()) {
			self->unk214 += 1;
			if (self->unk214 >= 3)
				self->unk214 = 0;
			return TRUE;
		}
	}

	return FALSE;
}

DEFINE_NERVE(TNerveCannonSearch, TLiveActor)
{
	TCannon* self = (TCannon*)spine->getBody();
	self->updateSquareToMario();

	f32 distToMario = self->mDistToMarioSquared;
	if (spine->getTime() == 0) {
		f32 bombDist = self->unk28C->mSLBombDist.get();
		if (distToMario < bombDist * bombDist)
			self->setGoalPathMario();

		TChorobei* chorobei = self->unk1A8;
		chorobei->unk6C->getMActor()->setBckFromIndex(19);
		const char** bas = chorobei->unk68->getBasNameTable();
		chorobei->unk78 = bas == nullptr ? nullptr : bas[19];
		if (chorobei->unk78 != nullptr) {
			chorobei->unk74->initAnmSound(
			    JKRFileLoader::getGlbResource(chorobei->unk78), 1, 0.0f);
		} else {
			chorobei->unk74->initAnmSound(nullptr, 1, 0.0f);
		}
	}

	if (self->unk1A8->unk6C->getMActor()->curAnmEndsNext(0, nullptr)
	    && self->unk1A8->unk6C->getMActor()->checkCurBckFromIndex(19)) {
		TChorobei* chorobei = self->unk1A8;
		chorobei->unk6C->getMActor()->setBckFromIndex(18);
		const char** bas = chorobei->unk68->getBasNameTable();
		chorobei->unk78 = bas == nullptr ? nullptr : bas[18];
		if (chorobei->unk78 != nullptr) {
			chorobei->unk74->initAnmSound(
			    JKRFileLoader::getGlbResource(chorobei->unk78), 1, 0.0f);
		} else {
			chorobei->unk74->initAnmSound(nullptr, 1, 0.0f);
		}
	}

	f32 hideDist = self->unk28C->mSLHideDist.get();
	if (distToMario < hideDist * hideDist) {
		spine->pushAfterCurrent(&TNerveCannonClose::theNerve());
		return TRUE;
	}

	f32 bombDist = self->unk28C->mSLBombDist.get();
	if (distToMario < bombDist * bombDist) {
		if (spine->getTime() > self->unk28C->mSLBombInterval.get()) {
			self->unk290 = true;
			spine->pushAfterCurrent(&TNerveCannonShoot::theNerve());
			return TRUE;
		}
	} else if ((self->unk230 == 5 || self->unk230 == 9)
	           && distToMario
	                  < self->unk28C->mSLKillerDist.get()
	                        * self->unk28C->mSLKillerDist.get()) {
		if (spine->getTime() > self->unk28C->mSLShootInterval.get()) {
			self->unk290 = false;
			spine->pushAfterCurrent(&TNerveCannonSearch::theNerve());
			spine->pushAfterCurrent(
			    &TNerveCannonForceBombShoot::theNerve());
			spine->pushAfterCurrent(&TNerveCannonShoot::theNerve());
			spine->pushAfterCurrent(&TNerveCannonShoot::theNerve());
			spine->pushAfterCurrent(&TNerveCannonShoot::theNerve());
			return TRUE;
		}
	}

	if (self->unk2AC != self->mRotation.y) {
		self->unk2AC = self->mRotation.y;
		if (gpMSound->gateCheck(0x20C8))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x20C8, &self->mPosition, 0, nullptr, 0, 4);
	}

	if (gpApplication.mCurrArea.unk0 == 5 && gpMarDirector->mState == 1) {
		JGeometry::TVec3<f32> diff = *gpMarioPos;
		diff.sub(self->mPosition);
		JGeometry::TVec3<f32> rot = MsGetRotFromZaxis(diff);
		self->mRotation.y         = rot.y;
	} else {
		self->walkToCurPathNode(0.0f, self->mTurnSpeed, 0.0f);
	}

	return FALSE;
}

DEFINE_NERVE(TNerveCannonOpen, TLiveActor)
{
	TCannon* self = (TCannon*)spine->getBody();

	if (spine->getTime() == 0) {
		TChorobei* chorobei = self->unk1A8;
		chorobei->unk6C->getMActor()->setBckFromIndex(12);
		const char** bas = chorobei->unk68->getBasNameTable();
		chorobei->unk78 = bas == nullptr ? nullptr : bas[12];
		if (chorobei->unk78 != nullptr) {
			chorobei->unk74->initAnmSound(
			    JKRFileLoader::getGlbResource(chorobei->unk78), 1, 0.0f);
		} else {
			chorobei->unk74->initAnmSound(nullptr, 1, 0.0f);
		}

		self->setBckAnm(3);
	}

	TChorobei* chorobei = self->unk1A8;
	bool chorobeiOpened;
	if (chorobei->unk6C->getMActor()->curAnmEndsNext(0, nullptr)
	    && chorobei->unk6C->getMActor()->checkCurBckFromIndex(12)) {
		chorobeiOpened = true;
	} else {
		chorobei->unk70 = 0.0f;
		chorobeiOpened  = false;
	}

	if (chorobeiOpened && self->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveCannonSearch::theNerve());
		return TRUE;
	}

	return FALSE;
}

void TCannon::startChorobeiShout() { }

bool TCannon::isObject()
{
	bool isObject = mCurrentBckAnm == 4 ? true : false;
	if (isObject && checkCurAnmEnd(0))
		return true;

	return false;
}

void TCannon::setKillerGoalPoint()
{
	JGeometry::TVec3<f32> target;
	if (unk239) {
		target = *gpMarioPos;
		f32 angle = 0.0f
		            + (360000.0f - 0.0f)
		                * ((f32)rand() * (1.0f / 32768.0f));
		u16 angleShort = (u16)(s32)angle;
		u32 index      = angleShort >> jmaSinShift;
		target.x += 500.0f * jmaCosTable[index];
		target.z += 500.0f * jmaSinTable[index];
	} else {
		target = unk248;
	}

	TPathNode node(target);
	unkF4  = node;
	unk104 = node;
	unk114.clear();

	TCannonDom* dom = unk1AC[unk214];
	dom->getMActor()->setBckFromIndex(1);
	const char** bas = dom->unk10->getBasNameTable();
	dom->unk20       = bas == nullptr ? nullptr : bas[1];
	if (dom->unk20 != nullptr) {
		dom->unk1C->initAnmSound(JKRFileLoader::getGlbResource(dom->unk20),
		                         1, 0.0f);
	} else {
		dom->unk1C->initAnmSound(nullptr, 1, 0.0f);
	}
}

void TCannon::killerShoot()
{
	if (unk239) {
		TKiller* killer = (TKiller*)gpConductor->makeOneEnemyAppear(
		    mPosition, "キラーマネージャー", 0);
		if (killer == nullptr)
			return;

		killer->reset();
		unk1E0 = unk1AC[unk214]->getMActor()->getModel()->getAnmMtx(1);

		TPosition3f localMtx;
		localMtx.identity33();
		localMtx.mMtx[0][3] = 0.0f;
		localMtx.mMtx[1][3] = -60.0f;
		localMtx.mMtx[2][3] = 150.0f;
		PSMTXConcat(unk1E0, localMtx.mMtx, localMtx.mMtx);
		killer->mPosition.x = localMtx.mMtx[0][3];
		killer->mPosition.y = localMtx.mMtx[1][3];
		killer->mPosition.z = localMtx.mMtx[2][3];

		JGeometry::TVec3<f32> target = *gpMarioPos;
		target.x += -300.0f
		            + (300.0f - -300.0f)
		                * ((f32)rand() * (1.0f / 32768.0f));

		switch (unk214) {
		case 0: {
			f32 offset = -300.0f
			             + (300.0f - -300.0f)
			                 * ((f32)rand() * (1.0f / 32768.0f));
			target.z -= 2.0f * __fabsf(offset);
			break;
		}
		case 2: {
			f32 offset = -300.0f
			             + (300.0f - -300.0f)
			                 * ((f32)rand() * (1.0f / 32768.0f));
			target.z += 2.0f * __fabsf(offset);
			break;
		}
		}

		JGeometry::TVec3<f32> velocity
		    = killer->calcVelocityToJumpToY(target, 5.0f,
		                                    killer->getGravityY());
		JGeometry::TVec3<f32> diff = target;
		diff.sub(mPosition);
		f32 flightTime
		    = __fabsf(MsVECMag2((Vec*)&diff) / (velocity.x * mVelocityRate));

		f32 velocityRate = mVelocityRate;
		killer->mIsChaseMode = 0;
		int roll             = (s32)(100.0f
		                 * ((f32)rand() * (1.0f / 32768.0f)));
		if (roll % 5 == 0) {
			killer->mIsChaseMode = 1;
		} else {
			if (*gpMarioSpeedX > 2.0f)
				velocityRate = 0.55f;
			if (*gpMarioSpeedX < -2.0f)
				velocityRate = 0.68f;
		}

		JGeometry::TVec3<f32> predicted;
		predicted.x = target.x + mSearchRate * (*gpMarioSpeedX * flightTime);
		predicted.y = target.y;
		predicted.z = target.z + mSearchRate * (*gpMarioSpeedZ * flightTime);
		velocity = killer->calcVelocityToJumpToY(predicted, 5.0f,
		                                         killer->getGravityY());
		velocity.scale(velocityRate);

		killer->mRotation.x = 0.0f;
		killer->mRotation.y = MsAngleWrap(MsGetRotFromZaxisY(velocity));
		killer->mRotation.z = 0.0f;
		killer->mScaling.set(0.1f, 0.1f, 0.1f);

		if (gpMarDirector->mState == 1) {
			velocity.x *= 0.2f;
			velocity.y *= 0.4f;
			velocity.z *= 0.2f;
			killer->mScaling.set(0.6f, 0.6f, 0.6f);
		}

		killer->setColorType();
		killer->mChaseTarget = velocity;
		killer->mVelocity    = velocity;
		killer->onLiveFlag(LIVE_FLAG_AIRBORNE);

		JGeometry::TVec3<f32> goal = *gpMarioPos;
		goal.sub(mPosition);
		target.x += goal.x;
		target.z += goal.z;
		killer->setGoalPath(TPathNode(target));

		if (gpMSound->gateCheck(0x285D))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x285D, &killer->mPosition, 0, nullptr, 0, 4);
		if (gpMSound->gateCheck(0x20A9))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x20A9, &killer->mPosition, 0, nullptr, 0, 4);
	} else {
		TIgaiga* igaiga = (TIgaiga*)gpConductor->makeOneEnemyAppear(
		    mPosition, "イガイガマネージャー", 1);
		if (igaiga == nullptr)
			return;

		igaiga->reset();
		unk1E0 = unk1AC[unk214]->getMActor()->getModel()->getAnmMtx(1);

		TPosition3f localMtx;
		localMtx.identity33();
		localMtx.mMtx[0][3] = 0.0f;
		localMtx.mMtx[1][3] = -60.0f;
		localMtx.mMtx[2][3] = 150.0f;
		PSMTXConcat(unk1E0, localMtx.mMtx, localMtx.mMtx);
		igaiga->mPosition.x = localMtx.mMtx[0][3];
		igaiga->mPosition.y = localMtx.mMtx[1][3];
		igaiga->mPosition.z = localMtx.mMtx[2][3];

		JPABaseEmitter* emitter = gpMarioParticleManager->emitWithRotate(
		    0xCB, &igaiga->mPosition, 0,
		    (s16)(igaiga->mRotation.y * 182.04445f), 0, 0, nullptr);
		if (emitter != nullptr) {
			JGeometry::TVec3<f32> scale;
			scale.x = 1.5f * mScaling.x;
			scale.y = 1.5f * mScaling.y;
			scale.z = 1.5f * mScaling.z;
			emitter->setScale(scale);
		}

		JGeometry::TVec3<f32> target;
		igaiga->getTracer()->getGraph()->getFirstGraphNode().getPoint(
		    (Vec*)&target);
		unk248 = target;
		JGeometry::TVec3<f32> velocity
		    = igaiga->calcVelocityToJumpToY(target, 10.0f,
		                                    igaiga->getGravityY());
		igaiga->mRotation = mRotation;
		igaiga->shoot(velocity);
	}
}

void TCannon::bombShoot()
{
	if (unk1A4 == nullptr)
		return;

	JGeometry::TVec3<f32> velocity;
	velocity.x = gpMarioPos->x - mPosition.x;
	velocity.y = 0.0f;
	velocity.z = gpMarioPos->z - mPosition.z;
	if (velocity.x == 0.0f && velocity.z == 0.0f)
		velocity.x = 1.0f;
	MsVECNormalize(&velocity, &velocity);

	Mtx rot;
	f32 angle = -30.0f
	            + (30.0f - -30.0f) * ((f32)rand() * (1.0f / 32768.0f));
	MsMtxSetRotRPH(rot, 0.0f, mRotation.y + angle, 0.0f);

	f32 speed = unk28C->mSLThrowXZSpeed.get();
	velocity.y = speed;
	velocity.x *= speed;
	velocity.z *= speed;

	unk1A4->mVelocity = velocity;
	if (unk21C) {
		unk1A4->offLiveFlag(LIVE_FLAG_UNK10);
	} else {
		unk1A4->onLiveFlag(LIVE_FLAG_AIRBORNE);
		unk1A4->getMActor()->setFrameRate(SMSGetAnmFrameRate(), 0);
	}

	unk1A4->mPosition.y += 2.0f;
	unk1A4->receiveMessage(this, HIT_MESSAGE_UNK6);
}

void TCannon::bombSet()
{
	unk21C = false;
	unk1A4 = nullptr;

	TSmallEnemy* spawned = nullptr;
	f32 rate = (f32)rand() * (1.0f / 32768.0f);
	if (rate < unk28C->mSLBombHeiGenerateRate.get()) {
		spawned = (TSmallEnemy*)gpConductor->makeOneEnemyAppear(
		    mPosition, "ボム兵マネージャー", 1);
	} else {
		int choice = (s32)(100.0f * ((f32)rand() * (1.0f / 32768.0f)));
		if (choice % 2 == 1) {
			spawned = (TSmallEnemy*)gpConductor->makeOneEnemyAppear(
			    mPosition, "ポポマネージャ", 1);
			if (spawned != nullptr)
				((TPopo*)spawned)->thrownByChorobei();
		} else {
			spawned = (TSmallEnemy*)gpConductor->makeOneEnemyAppear(
			    mPosition, "ハムクリマネージャ", 1);
		}
	}

	if (unk1A4 == nullptr) {
		unk1A4 = spawned;
		if (spawned != nullptr) {
			spawned->reset();
			spawned->getMActor()->setFrameRate(0.0f, 0);
		}
	}

	if (unk1A4 != nullptr) {
		unk1A4->mPosition = unk1A8->mPosition;
		unk220 = unk1A4->mScaling.x;
		unk1A4->mScaling.zero();
		unk1A4->mRotation = mRotation;
		if (unk1A4->receiveMessage(this, HIT_MESSAGE_TAKE))
			mHeldObject = unk1A4;
	}
}

bool TCannon::isHitVallid(u32)
{
	return false;
}

MtxPtr TCannon::getTakingMtx()
{
	if (checkLiveFlag(LIVE_FLAG_CLIPPED_OUT))
		return unk1A8->unk6C->getMActor()->getModel()->getBaseTRMtx();

	MtxPtr takingMtx = unk1A8->unk6C->getMActor()->getModel()->getAnmMtx(
	    mChorobeiHandJntIdx);
	unk1E4.identity33();
	unk1E4.mMtx[0][3] = unk1A8->mPosition.x;
	unk1E4.mMtx[1][3] = takingMtx[1][3];
	unk1E4.mMtx[2][3] = unk1A8->mPosition.z;
	return unk1E4.mMtx;
}

void TCannon::calcRootMatrix()
{
	static const f32 xzTable[] = {
		1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
	};

	if (mSpine->getCurrentNerve() != &TNerveCannonObject::theNerve()) {
		mHeadHeight = 50.0f;
		JGeometry::TVec3<f32> base = mPosition;
		MtxPtr chorobeiMtx
		    = getMActor()->getModel()->getAnmMtx(mChorobeiJntIdx);
		base.y = chorobeiMtx[1][3];

		for (int i = 0; i < 4; ++i) {
			unk25C[i] = base;
			unk25C[i].x += 200.0f * xzTable[i * 2];
			unk25C[i].z += 200.0f * xzTable[i * 2 + 1];
		}

		unk258->setVertexData(0, unk25C[2], unk25C[1], unk25C[0]);
		unk258->setVertexData(1, unk25C[0], unk25C[3], unk25C[2]);
	}

	if (mHolder != nullptr) {
		MtxPtr takingMtx = mHolder->getTakingMtx();
		if (mSpine->getCurrentNerve() == &TNerveCannonObject::theNerve()) {
			PSMTXCopy(takingMtx, getModel()->getBaseTRMtx());
			mPosition.x = takingMtx[0][3];
			mPosition.y = takingMtx[1][3];
			mPosition.z = takingMtx[2][3];
		} else {
			if (gpMarDirector->unk124 == 3 || gpMarDirector->unk124 == 4)
				mRotation.y = -80.0f;

			mPosition.x = takingMtx[0][3];
			mPosition.y = takingMtx[1][3];
			mPosition.z = takingMtx[2][3];
			MsMtxSetXYZRPH(getMActor()->getModel()->getBaseTRMtx(),
			                mPosition.x, mPosition.y, mPosition.z,
			                (s16)(mRotation.x * 182.04445f),
			                (s16)(mRotation.y * 182.04445f),
			                (s16)(mRotation.z * 182.04445f));
		}
	} else {
		TSpineEnemy::calcRootMatrix();
	}

	if (unk1A8 != nullptr
	    && unk1A8->unk6C->getMActor()->checkCurBckFromIndex(14)
	    && unk1A8->unk6C->getMActor()->getFrameCtrl(0)->checkPass(2.0f)) {
		for (int i = 0; i < 3; ++i) {
			TCannonDom* dom = unk1AC[i];
			MtxPtr mtx      = dom->getConnectedMtx();
			unk294.x        = mtx[0][3];
			unk294.y        = mtx[1][3];
			unk294.z        = mtx[2][3];

			dom->getMActor()->setBckFromIndex(2);
			const char** bas = dom->unk10->getBasNameTable();
			dom->unk20       = bas == nullptr ? nullptr : bas[2];
			if (dom->unk20 != nullptr) {
				dom->unk1C->initAnmSound(
				    JKRFileLoader::getGlbResource(dom->unk20), 1, 0.0f);
			} else {
				dom->unk1C->initAnmSound(nullptr, 1, 0.0f);
			}

			gpMarioParticleManager->emit(0x14, &unk294, 0, nullptr);
			gpMarioParticleManager->emit(0x13, &unk294, 0, nullptr);
			gpMarioParticleManager->emit(0x12, &unk294, 0, nullptr);
		}
	}
}

void TCannon::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TSmallEnemy::perform(flags, graphics);

	if (unk238) {
		MActor* marioActor = unk1BC->getMActor();
		marioActor->perform(flags, graphics);

		if ((flags & 1) && marioActor->curAnmEndsNext(0, nullptr))
			unk238 = false;

		if ((flags & 2) && unk230 == 1) {
			J3DFrameCtrl* ctrl = marioActor->getFrameCtrl(0);
			MtxPtr mtx
			    = unk1B8->getMActor()->getModel()->getBaseTRMtx();
			if (ctrl->checkPass(174.0f)) {
				gpMarioParticleManager->emitAndBindToMtxPtr(
				    0xE8, mtx, 0, nullptr);
				gpMarioParticleManager->emitAndBindToMtxPtr(
				    0xE9, mtx, 0, nullptr);
				gpMarioParticleManager->emitAndBindToMtxPtr(
				    0xEA, mtx, 0, nullptr);
				gpMarioParticleManager->emitAndBindToMtxPtr(
				    0xEB, mtx, 0, nullptr);
				gpMarioParticleManager->emitAndBindToMtxPtr(
				    0xEC, mtx, 0, nullptr);
			}

			if (ctrl->getFrame() > 175.0f)
				gpMarioParticleManager->emitAndBindToMtxPtr(
				    0x166, mtx, 1, this);
		}
	}

	if (unk230 == 5 || unk230 == 9) {
		unk1A8->perform(flags, graphics);

		if ((flags & 0x200)
		    && mSpine->getCurrentNerve()
		           == &TNerveCannonDamage::theNerve()) {
			MActor* chorobeiActor = unk1A8->unk6C->getMActor();
			chorobeiActor->offMakeDL();
			SMS_AddDamageFogEffect(
			    chorobeiActor->getModel()->getModelData(), mPosition,
			    graphics);
		}

		for (int i = 0; i < 3; ++i) {
			TCannonDom* dom = unk1AC[i];
			if (dom->unk24)
				continue;

			if (flags == 1) {
				f32 bombDist = unk28C->mSLBombDist.get();
				if (bombDist * bombDist < mDistToMarioSquared
				    && unk230 == 5) {
					dom->unk2C = (&unk224)[i];
					if (i != unk214
					    && mSpine->getCurrentNerve()
					           != &TNerveCannonObject::theNerve()) {
						dom->unk30 += 1.0f;
						if (dom->unk30 > 360.0f)
							dom->unk30 -= 360.0f;

						f32 angle = 0.5f * dom->unk30;
						u16 angleShort = (u16)(s32)(182.04445f * angle);
						u32 index = angleShort >> jmaSinShift;
						if (i == 2) {
							dom->unk28 = -10.0f * jmaSinTable[index];
						} else {
							dom->unk28 = -20.0f * jmaSinTable[index];
						}
					}
				} else {
					dom->unk30 = 0.0f;
					dom->unk28 *= 0.99f;
					unk1C0[i]->moveMtx(dom->getConnectedMtx());
				}
			}

			dom->perform(flags, graphics);
		}
	} else {
		if (!unk1B8->unk24)
			unk1B8->perform(flags, graphics);
	}
}

const char** TCannon::getBasNameTable() const
{
	return cannon_bastable;
}

BOOL TCannon::receiveMessage(THitActor* sender, u32 message)
{
	if (sender->mActorType == 0x40000235 && message == HIT_MESSAGE_TAKE
	    && mHolder == nullptr) {
		mHolder = (TTakeActor*)sender;
		return TRUE;
	}

	if (message == HIT_MESSAGE_SPRAYED_BY_WATER)
		return TRUE;

	return FALSE;
}

void TCannon::moveObject()
{
	TSmallEnemy::moveObject();

	if (unk230 != 5 && unk230 != 9)
		return;

	if (checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)) {
		unk1A8->mPosition = mPosition;
	} else {
		MtxPtr mtx = getModel()->getAnmMtx(mChorobeiJntIdx);
		unk1A8->mPosition.x = mtx[0][3];
		unk1A8->mPosition.y = mtx[1][3] - 100.0f;
		unk1A8->mPosition.z = mtx[2][3];
	}

	unk1A8->checkHit();

	JGeometry::TVec3<f32> velocity = mVelocity;
	mPosition.y += velocity.y;
	mVelocity.y -= getGravityY();
	if (mPosition.y < unk23C.y) {
		mVelocity.zero();
		mPosition.y = unk23C.y;
	}

	if (unk1A0 == nullptr)
		return;

	JGeometry::TVec3<f32> carriedPos = unk194;
	carriedPos.add(unk1A8->mPosition);
	unk1A0->mPosition = carriedPos;
	unk1A0->offLiveFlag(LIVE_FLAG_AIRBORNE);

	if (mSpine->getCurrentNerve() == &TNerveCannonClose::theNerve()) {
		unk1A0->kill();
		unk1A0 = nullptr;
		return;
	}

	if (!unk1A0->doKeepDistance()) {
		unk1A0 = nullptr;
		mSpine->pushNerve(&TNerveCannonDamage::theNerve());
	}
}

void TCannon::reset()
{
	TSmallEnemy::reset();
	mHitPoints = getSaveParam() ? getSaveParam()->mSLHitPointMax.get() : 1;
	unk64 |= HIT_FLAG_NO_COLLISION;
	unk214 = 1;
	mHeadHeight = 40.0f;
	mLiveFlag |= LIVE_FLAG_UNK10;
	unk2AC = mRotation.y;

	if (unk230 == 9) {
		unk239 = false;
		JGeometry::TVec3<f32> target(-565.0f, 8500.0f, 7675.0f);
		TPathNode node(target);
		unkF4  = node;
		unk104 = node;
		unk114.clear();
	}
}

void TCannon::init(TLiveManager* manager)
{
	TSmallEnemy::init(manager);
	mActorType = 0x1000001c;
	unk150     = 0x11;
	unk28C     = getCannonParams();
	setBckAnm(3);

	void* res = JKRFileLoader::getGlbResource("/scene/cannon/cannon_Dom.bmd");
	SDLModelData* domData
	    = new SDLModelData(J3DModelLoaderDataBase::load(res, 0x10050000));
	unk234 = mRotation.y;
	unk230 = gpApplication.mCurrArea.unk0;

	if (unk230 == 5 || unk230 == 9) {
		mSpine->initWith(&TNerveCannonSearch::theNerve());

		if (unk230 == 9) {
			unk239 = false;
			JGeometry::TVec3<f32> target(-565.0f, 8500.0f, 7675.0f);
			unkF4  = TPathNode(target);
			unk104 = TPathNode(target);
			unk114.clear();
		} else {
			setGoalPath(TPathNode((THitActor*)gpMarioAddress));
		}

		unk1A8 = new TChorobei(this, 0, "チョロベー");
		unk1A8->initHitActor(
		    0x1000001d, 3, 0x90000000,
		    unk28C->mSLChorobeiAttackRadius.get(),
		    unk28C->mSLChorobeiAttackHeight.get(),
		    unk28C->mSLChorobeiDamageRadius.get(),
		    unk28C->mSLChorobeiDamageHeight.get());
		TIdxGroupObj* group
		    = JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ");
		group->getChildren().push_back(unk1A8);

		static const char* sCannonDomPartsJointTable[] = {
			"nullC",
			"nullB",
			"nullA",
		};

		JUTNameTab* joints
		    = getMActor()->getModel()->getModelData()->getJointName();
		for (int i = 0; i < 3; ++i) {
			u16 index = joints->getIndex(sCannonDomPartsJointTable[i]);
			unk1AC[i] = new TCannonDom(this, index, domData, 3, "砲身");
			unk1C0[i] = new TMapCollisionMove();
			unk1C0[i]->init("/cannon/CannonDom", 2, this);
			unk1C0[i]->setUpTrans(mPosition);
		}

		unk2B0 = new TMapCollisionMove();
		unk2B0->init("/cannon/CannonFuta", 2, this);
		unk2B0->setUpTrans(mPosition);
	} else {
		mSpine->initWith(&TNerveCannonObject::theNerve());
		JUTNameTab* joints
		    = getMActor()->getModel()->getModelData()->getJointName();
		u16 index = joints->getIndex("nullA");
		unk1B8    = new TCannonDom(this, index, domData, 3, "砲身");
		unk1B8->unk24 = true;
	}

	void* marioRes
	    = JKRFileLoader::getGlbResource("/scene/cannon/hodai_mario.bmd");
	SDLModelData* marioData
	    = new SDLModelData(J3DModelLoaderDataBase::load(marioRes, 0x10010000));
	unk1BC = new TSharedParts(this, 0, marioData, 3, "<TSharedParts>");
	unk258 = new TMapCollisionMove();
	unk258->init(2, 0, 0, nullptr);
}

void TCannon::loadAfter()
{
	SMS_LoadParticle("/scene/cannon/jpa/ms_cannon_a.jpa", 0xe8);
	SMS_LoadParticle("/scene/cannon/jpa/ms_cannon_b.jpa", 0xe9);
	SMS_LoadParticle("/scene/cannon/jpa/ms_cannon_c.jpa", 0xea);
	SMS_LoadParticle("/scene/cannon/jpa/ms_cannon_d.jpa", 0xeb);
	SMS_LoadParticle("/scene/cannon/jpa/ms_cannon_e.jpa", 0xec);
	SMS_LoadParticle("/scene/cannon/jpa/ms_cannon_smoke.jpa", 0x166);

	if (unk230 == 5) {
		unk254 = JDrama::TNameRefGen::search<TMareGate>("efMareGate");
		unk254->kill();
	}
}

void TCannon::load(JSUMemoryInputStream& stream)
{
	TSmallEnemy::load(stream);
	reset();
	mHitPoints = getSaveParam() ? getSaveParam()->mSLHitPointMax.get() : 1;
	unk230 = gpApplication.mCurrArea.unk0;
	unk23C = mPosition;
}

TCannon::TCannon(const char* name)
    : TSmallEnemy(name)
    , unk1A0(nullptr)
    , unk1A8(nullptr)
    , unk1B8(nullptr)
    , unk1E0(nullptr)
    , unk214(0)
    , unk218(nullptr)
    , unk21C(false)
    , unk220(0.0f)
    , unk230(true)
    , unk238(false)
    , unk239(true)
    , unk254(nullptr)
    , unk258(nullptr)
    , unk290(false)
    , unk2AC(0.0f)
{
	unk224 = 0.0f;
	unk228 = 0.0f;
	unk22C = 0.0f;
}

void TCannonDom::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if ((unk10->mLiveFlag & 0x7) != 0)
		return;

	if (flags == 2) {
		if (unk1C != nullptr && unk20 != nullptr) {
			J3DFrameCtrl* ctrl = unk18->getFrameCtrl(0);
			unk1C->animeLoop((Vec*)&unk10->mPosition, ctrl->getFrame(),
			                 ctrl->getRate(), 0, 4);
		}

		Mtx rot;
		MtxPtr connectedMtx = getConnectedMtx();
		MsMtxSetRotRPH(rot, unk28, unk2C, 0.0f);
		PSMTXConcat(connectedMtx, rot, connectedMtx);
		PSMTXCopy(connectedMtx, unk18->getModel()->getBaseTRMtx());
	}

	unk18->perform(flags, graphics);
}

TCannonDom::TCannonDom(TLiveActor* owner, int jointIndex, SDLModelData* data,
                       u32 flags, const char* name)
    : TSharedParts(owner, jointIndex, data, flags, name)
    , unk1C(nullptr)
    , unk20(nullptr)
    , unk24(0)
    , unk28(0.0f)
    , unk2C(0.0f)
    , unk30(0.0f)
{
	f32 min = 0.0f;
	f32 max = 360.0f;
	unk30  = min + (max - min) * ((f32)rand() * (1.0f / 32768.0f));

	if (unk1C != nullptr)
		return;

	unk1C = new MAnmSound(gpMSound);
	unk1C->initAnmSound(nullptr, 1, 0.0f);
}

BOOL TChorobei::receiveMessage(THitActor*, u32)
{
	return FALSE;
}

BOOL TChorobei::checkHit()
{
	for (int i = 0; i < mColCount; ++i) {
		THitActor* actor = mCollisions[i];
		if (actor->mActorType == 0x80000001)
			SMS_SendMessageToMario(this, HIT_MESSAGE_ATTACK);

		if (actor->mActorType == 0x1000001e) {
			TBombHei* bomb = (TBombHei*)actor;
			if (unk68->mSpine->getCurrentNerve()
			        != &TNerveCannonDamage::theNerve()
			    && bomb->isDamageToCannon()) {
				unk68->mSpine->pushNerve(&TNerveCannonDamage::theNerve());
				bomb->kill();
			}
		}

		if (actor->mActorType == 0x1000001f) {
			TKiller* killer = (TKiller*)actor;
			if (killer->isRollFly()) {
				unk68->mSpine->pushNerve(&TNerveCannonDamage::theNerve());
				killer->kill();
			}
		}
	}

	return FALSE;
}

void TChorobei::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if ((unk68->mLiveFlag & 0x7) != 0)
		return;

	if (unk70 != 0.0f)
		return;

	if (flags & 2) {
		if (unk74 != nullptr && unk78 != nullptr) {
			J3DFrameCtrl* ctrl = unk6C->getMActor()->getFrameCtrl(0);
			unk74->animeLoop(&mPosition, ctrl->getFrame(), ctrl->getRate(), 0,
			                 4);
		}

		Mtx mtx;
		PSMTXCopy(unk6C->getConnectedMtx(), mtx);
		mtx[1][3] += unk7C;
		PSMTXCopy(mtx, unk6C->getMActor()->getModel()->getBaseTRMtx());
		mPosition.x = mtx[0][3];
		mPosition.y = mtx[1][3] - 150.0f;
		mPosition.z = mtx[2][3];
	}

	THitActor::perform(flags, graphics);
	unk6C->getMActor()->perform(flags, graphics);
}

TChorobei::TChorobei(TCannon* cannon, int jointIndex, const char* name)
    : THitActor(name)
    , unk68(cannon)
    , unk6C(nullptr)
    , unk70(0.0f)
    , unk74(nullptr)
    , unk78(nullptr)
    , unk7C(300.0f)
{
	unk6C = new TSharedParts(unk68, jointIndex,
	                         "/scene/cannon/tyorobe_model1.bmd", 0x10020000,
	                         3, "<TSharedParts>");
	if (unk74 != nullptr)
		return;

	MAnmSound* anmSound = new MAnmSound(SMSGetMSound());
	unk74 = anmSound;
	unk74->initAnmSound(nullptr, 1, 0.0f);
}

TSmallEnemy* TCannonManager::createEnemyInstance()
{
	return new TCannon("砲台");
}

void TCannonManager::load(JSUMemoryInputStream& stream)
{
	TSmallEnemyManager::load(stream);
	unk38 = new TCannonSaveLoadParams("/enemy/cannon.prm");
}

TCannonManager::TCannonManager(const char* name)
    : TSmallEnemyManager(name)
{
}

TCannonSaveLoadParams::TCannonSaveLoadParams(const char* path)
    : TSmallEnemyParams(path)
    , PARAM_INIT(mSLHideDist, 300.0f)
    , PARAM_INIT(mSLBombDist, 2000.0f)
    , PARAM_INIT(mSLKillerDist, 10000.0f)
    , PARAM_INIT(mSLBombInterval, 100)
    , PARAM_INIT(mSLKillerInterval, 50)
    , PARAM_INIT(mSLShootInterval, 100)
    , PARAM_INIT(mSLChorobeiAttackRadius, 100.0f)
    , PARAM_INIT(mSLChorobeiAttackHeight, 100.0f)
    , PARAM_INIT(mSLChorobeiDamageRadius, 100.0f)
    , PARAM_INIT(mSLChorobeiDamageHeight, 100.0f)
    , PARAM_INIT(mSLKillerTransYOffset, -50.0f)
    , PARAM_INIT(mSLBombHeiGenerateRate, 0.7f)
    , PARAM_INIT(mSLThrowXZSpeed, 12.0f)
{
	TParams::load(mPrmPath);
}
