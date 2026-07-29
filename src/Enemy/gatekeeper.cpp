#include <Enemy/GateKeeper.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/Igaiga.hpp>
#include <Enemy/NameKuri.hpp>
#include <GC2D/GCConsole2.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DCluster.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <M3DUtil/MActor.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <Map/PollutionManager.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <MarioUtil/TexUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Strategy.hpp>
#include <System/FlagManager.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>
#include <dolphin/mtx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline f32 callMsWrap(f32 t, f32 l, f32 r)
{
	return MsWrap<f32>(t, l, r);
}

DEFINE_NERVE(TNerveBGKSleep, TLiveActor)
{
	TBiancoGateKeeper* gatekeeper
	    = (TBiancoGateKeeper*)spine->getBody();

	if (spine->getTime() == 0) {
		gatekeeper->changeBck(0x0A);
		gatekeeper->offHitFlag(HIT_FLAG_NO_COLLISION);
		gatekeeper->mMActor->setBpkFromIndex(0);

		J3DFrameCtrl* ctrl = gatekeeper->mMActor->getFrameCtrl(2);
		if (ctrl) {
			ctrl->setFrame(0.0f);
			ctrl->setRate(0.0f);
		}
	}

	if (gpMarDirector->mMap == 2) {
		if (gatekeeper->unk298 > 0)
			--gatekeeper->unk298;

		if (gatekeeper->unk298 == 0) {
			TBiancoGateKeeperParams* params
			    = (TBiancoGateKeeperParams*)gatekeeper->getSaveParam();
			s32 timer = params->mSLLaunchTimerNormal.get();
			timer += (s32)(240.0f * (0.000030517578f * rand())) - 0x78;
			gatekeeper->unk298 = timer;

			spine->pushAfterCurrent(&TNerveBGKLaunchGoro::theNerve());
			return true;
		}
	}

	if (gatekeeper->mMActor->checkBckPass(18.0f)) {
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x1E0, (MtxPtr)gatekeeper->getModel()->mNodeMatrices, 2,
		    nullptr);
	}

	if (gatekeeper->unk17C >= 0xFF) {
		spine->pushAfterCurrent(&TNerveBGKAppear::theNerve());
		return true;
	}

	if (gatekeeper->unk154 > 0) {
		spine->pushAfterCurrent(&TNerveBGKSleepDamage::theNerve());
		return true;
	}

	return false;
}

DEFINE_NERVE(TNerveBGKAppear, TLiveActor)
{
	TBiancoGateKeeper* gatekeeper
	    = (TBiancoGateKeeper*)spine->getBody();

	if (spine->getTime() == 0) {
		gatekeeper->changeBck(0);

		if (gatekeeper->unk28A == 0) {
			snprintf((char*)gatekeeper->unk188, 0x100, "%s出現カメラ",
			         gatekeeper->getName());
			gpMarDirector->fireStartDemoCamera(
			    (char*)gatekeeper->unk188, &gatekeeper->mPosition, -1, 0.0f,
			    true, nullptr, 0, nullptr, JDrama::TFlagT<u16>(0));
			MSBgm::setTrackVolume(0, 0.0f, 10, 0);
			MSBgm::startBGM(0x8001000B);
		}

		if (gatekeeper->unk28A < 0xFF)
			++gatekeeper->unk28A;

		if (gatekeeper->unk28A == 2)
			gpMarDirector->mConsole->startAppearBalloon(0xE0047, true);
	}

	if (spine->getTime() == 8) {
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x1DF,
		    (MtxPtr)((u8*)gatekeeper->getModel()->mNodeMatrices + 0x240), 2,
		    nullptr);

		if (SMS_IsMarioTouchGround4cm()) {
			gatekeeper->unk29C = gatekeeper->getRumblePow();
			SMSRumbleMgr->start(8, &gatekeeper->unk29C);
		}
	}

	if (gatekeeper->mMActor->curAnmEndsNext()) {
		if (gpMarDirector->mMap == 0)
			spine->pushAfterCurrent(&TNerveBGKWait2::theNerve());
		else
			spine->pushAfterCurrent(&TNerveBGKWait::theNerve());

		return true;
	}

	return false;
}

DEFINE_NERVE(TNerveBGKAwakeDamage, TLiveActor)
{
	TBiancoGateKeeper* gatekeeper
	    = (TBiancoGateKeeper*)spine->getBody();

	if (spine->getTime() == 0)
		gatekeeper->changeBck(3);

	BOOL animEnd = true;
	J3DFrameCtrl* ctrl = gatekeeper->mMActor->getFrameCtrl(0);
	if (ctrl) {
		if (!ctrl->checkState(J3DFrameCtrl::STATE_COMPLETED_ONCE)
		    && !ctrl->checkState(J3DFrameCtrl::STATE_LOOPED_ONCE)
		    && 0.1f + ctrl->getFrame() < ctrl->getEnd())
			animEnd = false;
	}

	if (animEnd) {
		if (gpMarDirector->mMap == 0)
			spine->pushAfterCurrent(&TNerveBGKWait2::theNerve());
		else
			spine->pushAfterCurrent(&TNerveBGKWait::theNerve());

		return true;
	}

	return false;
}

DEFINE_NERVE(TNerveBGKLaunchGoro, TLiveActor)
{
	TBiancoGateKeeper* gatekeeper
	    = (TBiancoGateKeeper*)spine->getBody();

	if (spine->getTime() == 0)
		gatekeeper->changeBck(8);

	if (spine->getTime() == 0x34) {
		TGorogoro* goro
		    = (TGorogoro*)gpConductor->makeOneEnemyAppear(
		        gatekeeper->mPosition, "ゴロゴロマネージャー", 0);
		if (goro)
			goro->generateByGateKeeper(gatekeeper->mPosition,
			                           gatekeeper->mRotation);

		if (SMS_IsMarioTouchGround4cm()) {
			gatekeeper->unk29C = gatekeeper->getRumblePow();
			SMSRumbleMgr->start(8, &gatekeeper->unk29C);
		}
	}

	if (gatekeeper->unk17C >= 0xFF) {
		spine->pushAfterCurrent(&TNerveBGKAppear::theNerve());
		return true;
	}

	if (gatekeeper->mMActor->curAnmEndsNext()) {
		spine->pushAfterCurrent(&TNerveBGKSleep::theNerve());
		return true;
	}

	return false;
}

DEFINE_NERVE(TNerveBGKLaunchName, TLiveActor)
{
	TBiancoGateKeeper* gatekeeper
	    = (TBiancoGateKeeper*)spine->getBody();

	if (spine->getTime() == 0)
		gatekeeper->changeBck(8);

	if (spine->getTime() == 0x34) {
		gatekeeper->launchNamekuri();

		if (SMS_IsMarioTouchGround4cm()) {
			gatekeeper->unk29C = gatekeeper->getRumblePow();
			SMSRumbleMgr->start(8, &gatekeeper->unk29C);
		}
	}

	if (gatekeeper->mMActor->curAnmEndsNext()) {
		spine->pushAfterCurrent(&TNerveBGKAppear::theNerve());
		return true;
	}

	return false;
}

DEFINE_NERVE(TNerveBGKDive, TLiveActor)
{
	TBiancoGateKeeper* gatekeeper
	    = (TBiancoGateKeeper*)spine->getBody();

	if (spine->getTime() == 0)
		gatekeeper->changeBck(6);

	if (spine->getTime() == 0xF0) {
		J3DModel* model = gatekeeper->getModel();
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x1DF, (MtxPtr)model->mNodeMatrices, 2, nullptr);

		if (SMS_IsMarioTouchGround4cm()) {
			gatekeeper->unk29C = gatekeeper->getRumblePow();
			SMSRumbleMgr->start(8, &gatekeeper->unk29C);
		}
	}

	if (gatekeeper->mMActor->curAnmEndsNext()) {
		if (gatekeeper->unk292 == 1) {
			TBiancoGateKeeperParams* params
			    = (TBiancoGateKeeperParams*)gatekeeper->getSaveParam();
			s32 timer = params->mSLLaunchTimerNormal.get();
			timer += (s32)(240.0f * (0.000030517578f * rand())) - 0x78;
			gatekeeper->unk298 = timer;

			TGorogoro* goro
			    = (TGorogoro*)gpConductor->makeOneEnemyAppear(
			        gatekeeper->mPosition, "ゴロゴロマネージャー", 0);
			if (goro)
				goro->generateByGateKeeper(gatekeeper->mPosition,
				                           gatekeeper->mRotation);

			if (SMS_IsMarioTouchGround4cm()) {
				gatekeeper->unk29C = gatekeeper->getRumblePow();
				SMSRumbleMgr->start(8, &gatekeeper->unk29C);
			}
		}

		spine->pushAfterCurrent(&TNerveBGKSleep::theNerve());
		return true;
	}

	return false;
}

DEFINE_NERVE(TNerveBGKDie, TLiveActor)
{
	TBiancoGateKeeper* gatekeeper
	    = (TBiancoGateKeeper*)spine->getBody();

	if (spine->getTime() == 0) {
		gatekeeper->changeBck(5);

		if (!((gatekeeper->unk292 == 3 || gatekeeper->unk292 == 4)
		      && gatekeeper->unk296 == 0)) {
			snprintf((char*)gatekeeper->unk188, 0x100, "%s撃退カメラ",
			         gatekeeper->getName());
			gpMarDirector->fireStartDemoCamera(
			    (char*)gatekeeper->unk188, &gatekeeper->mPosition, -1, 0.0f,
			    true, nullptr, 0, nullptr, JDrama::TFlagT<u16>(0));
			MSBgm::stopTrackBGM(1, 10);

			gatekeeper->mMActor->setBpkFromIndex(0);
			J3DFrameCtrl* bpkCtrl = gatekeeper->mMActor->getFrameCtrl(2);
			if (bpkCtrl) {
				bpkCtrl->setFrame(0.0f);
				bpkCtrl->setRate(SMSGetAnmFrameRate());
			}

			J3DModel* model      = gatekeeper->getModel();
			JPABaseEmitter* emit = gpMarioParticleManager->emitAndBindToMtxPtr(
			    0xA7, (MtxPtr)((u8*)model->mNodeMatrices + 0x120), 0,
			    nullptr);
			if (emit)
				SMSSetEmitterPolColor(emit, 6);

			gpMarioParticleManager->emitAndBindToMtxPtr(
			    0xA8, (MtxPtr)gatekeeper->getModel()->mNodeMatrices, 0,
			    nullptr);
		}
	}

	if (spine->getTime() == 0x154) {
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x1DF, (MtxPtr)gatekeeper->getModel()->mNodeMatrices, 2, nullptr);

		if (gpMSound->gateCheck(0x38B0))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x38B0, &gatekeeper->mPosition, 0, nullptr, 0, 4);

		if (SMS_IsMarioTouchGround4cm()) {
			gatekeeper->unk29C = gatekeeper->getRumblePow();
			SMSRumbleMgr->start(8, &gatekeeper->unk29C);
		}
	}

	if (SMS_IsMarioTouchGround4cm()) {
		J3DFrameCtrl* ctrl = gatekeeper->mMActor->getFrameCtrl(0);
		if (ctrl) {
			f32 ratio = 1.0f - ctrl->getFrame() / ctrl->getEnd();
			gatekeeper->unk29C = ratio * gatekeeper->getRumblePow();
			SMSRumbleMgr->start(8, &gatekeeper->unk29C);
		}
	}

	if (gatekeeper->mMActor->curAnmEndsNext()) {
		if ((gatekeeper->unk292 == 3 || gatekeeper->unk292 == 4)
		    && gatekeeper->unk296 == 0) {
			++gatekeeper->unk296;
			gatekeeper->mHitPoints = 3;

			if (gatekeeper->unk292 == 4)
				spine->pushAfterCurrent(&TNerveBGKLaunchName::theNerve());
			else
				spine->pushAfterCurrent(&TNerveBGKAppear::theNerve());

			return true;
		}

		gatekeeper->unk160 = 0;
		gatekeeper->kill();
		return true;
	}

	return false;
}

DEFINE_NERVE(TNerveBGKSleepDamage, TLiveActor)
{
	TBiancoGateKeeper* gatekeeper
	    = (TBiancoGateKeeper*)spine->getBody();

	if (spine->getTime() == 0)
		gatekeeper->changeBck(4);

	if (gatekeeper->unk292 == 1) {
		if (gatekeeper->unk298 > 0)
			--gatekeeper->unk298;

		if (gatekeeper->unk298 == 0) {
			TBiancoGateKeeperParams* params
			    = (TBiancoGateKeeperParams*)gatekeeper->getSaveParam();
			s32 timer = params->mSLLaunchTimerDamage.get();
			timer += (s32)(240.0f * (0.000030517578f * rand())) - 0x78;
			gatekeeper->unk298 = timer;

			TGorogoro* goro
			    = (TGorogoro*)gpConductor->makeOneEnemyAppear(
			        gatekeeper->mPosition, "ゴロゴロマネージャー", 0);
			if (goro)
				goro->generateByGateKeeper(gatekeeper->mPosition,
				                           gatekeeper->mRotation);

			if (SMS_IsMarioTouchGround4cm()) {
				gatekeeper->unk29C = gatekeeper->getRumblePow();
				SMSRumbleMgr->start(8, &gatekeeper->unk29C);
			}
		}
	}

	if (gatekeeper->mMActor->curAnmEndsNext()) {
		spine->pushAfterCurrent(&TNerveBGKSleep::theNerve());
		return true;
	}

	return false;
}

DEFINE_NERVE(TNerveBGKWait, TLiveActor)
{
	TBiancoGateKeeper* gatekeeper
	    = (TBiancoGateKeeper*)spine->getBody();
	MActor* actor = gatekeeper->mMActor;

	if (spine->getTime() == 0)
		gatekeeper->changeBck(0x11);

	if (gatekeeper->unk154 > 0 && actor->checkCurBckFromIndex(0x12)
	    && !gatekeeper->isHeadHitActive()) {
		gatekeeper->changeBck(7);
		return false;
	}

	if (gatekeeper->unk154 > 0 && !actor->checkCurBckFromIndex(0x0B)
	    && !actor->checkCurBckFromIndex(7)
	    && !actor->checkCurBckFromIndex(0x11)
	    && !actor->checkCurBckFromIndex(0x0D)) {
		if (gatekeeper->mHitPoints != 0)
			--gatekeeper->mHitPoints;

		if (gatekeeper->mHitPoints == 0)
			spine->pushAfterCurrent(&TNerveBGKDie::theNerve());
		else
			spine->pushAfterCurrent(&TNerveBGKAwakeDamage::theNerve());

		return true;
	}

	if (gatekeeper->unk154 > 0 && actor->checkCurBckFromIndex(0x0B)) {
		gatekeeper->changeBck(7);
		return false;
	}

	if (gatekeeper->unk154 > 0 && actor->checkCurBckFromIndex(7))
		gatekeeper->unk290 = 0;

	TBiancoGateKeeperParams* params
	    = (TBiancoGateKeeperParams*)gatekeeper->getSaveParam();
	if (spine->getTime() > params->mSLDiveTimer.get()
	    && !actor->checkCurBckFromIndex(0x0D)) {
		J3DFrameCtrl* ctrl = actor->getFrameCtrl(0);
		BOOL diveReady;
		if (ctrl == nullptr)
			diveReady = true;
		else if (ctrl->checkState(J3DFrameCtrl::STATE_COMPLETED_ONCE))
			diveReady = true;
		else if (ctrl->checkState(J3DFrameCtrl::STATE_LOOPED_ONCE))
			diveReady = true;
		else if (0.1f + ctrl->getFrame() >= ctrl->getEnd())
			diveReady = true;
		else
			diveReady = false;

		if (diveReady)
			gatekeeper->changeBck(0x0D);
	}

	J3DFrameCtrl* ctrl = actor->getFrameCtrl(0);
	BOOL animEnd;
	if (ctrl == nullptr)
		animEnd = true;
	else if (ctrl->checkState(J3DFrameCtrl::STATE_COMPLETED_ONCE))
		animEnd = true;
	else if (ctrl->checkState(J3DFrameCtrl::STATE_LOOPED_ONCE))
		animEnd = true;
	else if (0.1f + ctrl->getFrame() >= ctrl->getEnd())
		animEnd = true;
	else
		animEnd = false;

	if (animEnd) {
		if (actor->checkCurBckFromIndex(0x11) && gatekeeper->unk28A == 1
		    && gatekeeper->mHitPoints == 3) {
			gatekeeper->changeBck(0x0B);
		} else if (actor->checkCurBckFromIndex(0x11)) {
			gatekeeper->changeBck(0x12);
		} else if (actor->checkCurBckFromIndex(0x0B)) {
			gatekeeper->changeBck(0x12);
		} else if (actor->checkCurBckFromIndex(0x12)) {
			gatekeeper->changeBck(0x12);
		} else if (actor->checkCurBckFromIndex(7)) {
			if (gatekeeper->unk154 > 0) {
				gatekeeper->unk290 = 0;
				gatekeeper->changeBck(7);
			} else {
				gatekeeper->unk17C = 0;
				if (gatekeeper->unk290 >= 0) {
					gatekeeper->unk290 = 0;
					gatekeeper->changeBck(0x12);
				} else {
					++gatekeeper->unk290;
					gatekeeper->changeBck(7);
				}
			}
		} else if (actor->checkCurBckFromIndex(0x0D)) {
			spine->pushAfterCurrent(&TNerveBGKDive::theNerve());
			return true;
		}
	}

	return false;
}

DEFINE_NERVE(TNerveBGKWait2, TLiveActor)
{
	TBiancoGateKeeper* gatekeeper
	    = (TBiancoGateKeeper*)spine->getBody();

	if (spine->getTime() == 0)
		gatekeeper->changeBck(0x11);

	if (gatekeeper->unk154 > 0) {
		if (gatekeeper->mHitPoints != 0)
			--gatekeeper->mHitPoints;

		if (gatekeeper->mHitPoints == 0)
			spine->pushAfterCurrent(&TNerveBGKDie::theNerve());
		else
			spine->pushAfterCurrent(&TNerveBGKAwakeDamage::theNerve());

		return true;
	}

	BOOL animEnd = true;
	J3DFrameCtrl* ctrl = gatekeeper->mMActor->getFrameCtrl(0);
	if (ctrl) {
		if (!ctrl->checkState(J3DFrameCtrl::STATE_COMPLETED_ONCE)
		    && !ctrl->checkState(J3DFrameCtrl::STATE_LOOPED_ONCE)
		    && 0.1f + ctrl->getFrame() < ctrl->getEnd())
			animEnd = false;
	}

	if (animEnd) {
		MActor* actor = gatekeeper->mMActor;
		if (actor->checkCurBckFromIndex(0x11)) {
			gatekeeper->changeBck(0x0F);
		} else if (actor->checkCurBckFromIndex(0x0F)) {
			gatekeeper->changeBck(0x10);
		} else if (actor->checkCurBckFromIndex(0x10)) {
			++gatekeeper->unk288;

			if (gatekeeper->unk288 == 2 && gatekeeper->unk28A == 1
			    && gpMarDirector->mMap == 0)
				gpMarDirector->mConsole->startAppearBalloon(0xE0000,
				                                            true);

			TBiancoGateKeeperParams* params
			    = (TBiancoGateKeeperParams*)gatekeeper->getSaveParam();
			if (gatekeeper->unk288 > params->mSLLoop2Dive.get()) {
				gatekeeper->changeBck(0x0C);
				gatekeeper->unk288 = 0;
			}
		} else if (actor->checkCurBckFromIndex(0x0C)) {
			gatekeeper->changeBck(0x0D);
		} else if (actor->checkCurBckFromIndex(0x0D)) {
			spine->pushAfterCurrent(&TNerveBGKDive::theNerve());
			return true;
		}
	}

	return false;
}

const char* gatekeeper_bastable[] = {
	"/scene/gatekeeper/bas/gene_pakkun_appear1.bas",
	nullptr,
	nullptr,
	"/scene/gatekeeper/bas/gene_pakkun_damage1.bas",
	nullptr,
	"/scene/gatekeeper/bas/gene_pakkun_dead1.bas",
	"/scene/gatekeeper/bas/gene_pakkun_dive1.bas",
	"/scene/gatekeeper/bas/gene_pakkun_iyaiya1.bas",
	nullptr,
	nullptr,
	nullptr,
	"/scene/gatekeeper/bas/gene_pakkun_wait2.bas",
	nullptr,
	nullptr,
	"/scene/gatekeeper/bas/gene_pakkun_wait2_loop.bas",
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

TBiancoGateKeeper::TBiancoGateKeeper(const char* name)
    : TGateKeeperBase(name)
    , unk170(0)
    , unk174(0)
    , unk178(0)
    , unk17C(0)
    , unk17E(0)
    , unk180(0.0f)
    , unk184(0)
    , unk288(0)
    , unk28A(0)
    , unk28C(0)
    , unk290(0)
    , unk292(2)
    , unk293(0)
    , unk294(0)
    , unk296(0)
    , unk298(0)
    , unk29C(0.0f)
{
}

void TBiancoGateKeeper::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (checkLiveFlag(LIVE_FLAG_DEAD))
		return;

	u8 gameState = gpMarDirector->unk124;
	if (!(gameState == 3 || gameState == 4)) {
		if (gameState == 1 || gameState == 2)
			flags &= ~1;
	}

	BOOL doMove = flags & 1;
	if (doMove) {
		for (int i = 0; i < getColNum(); ++i) {
			THitActor* actor = getCollision(i);
			if (actor->getActorType() == 0x80000001)
				SMS_SendMessageToMario(this, HIT_MESSAGE_ATTACK);
		}

		if (unk154 > 0)
			unk17E = 0x10;

		if (unk17E > 0) {
			if (unk17C < 0xFF)
				++unk17C;
			else
				unk17C = 0xFF;
		} else {
			if (unk17C > 0)
				--unk17C;
			else
				unk17C = 0;
		}

		if (unk17E > 0)
			--unk17E;
	}

	if (doMove) {
		unk178->unk50 -= unk158;
		if (unk178->unk50 < 0.0f)
			unk178->unk50 = 0.0f;
		else if (unk178->unk50 > 1.0f)
			unk178->unk50 = 1.0f;

		controlCollision();
	}

	BOOL doAnim = flags & 2;
	if (doAnim) {
		if (gpMSound->gateCheck(0x210B))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x210B, &mPosition, 0, nullptr, 0, 4);

		J3DMtxCalc* calc = nullptr;
		if (unk178)
			calc = unk178;
		mMActor->getModel()->getModelData()->getJointNodePointer(0)->mMtxCalc
		    = calc;
	}

	if (flags & 0x200) {
		if (isDamageFogSituation()) {
			mMActor->offMakeDL();
			SMS_AddDamageFogEffect(mMActor->getModel()->getModelData(),
			                       mPosition, graphics);
		} else {
			SMS_ResetDamageFogEffect(mMActor->getModel()->getModelData());
		}
	}

	unk174->perform(flags, graphics);
	unk28C->perform(flags, graphics);

	if (doAnim)
		unk15C->update();

	if (doMove && unk293 == 0 && mMActor->checkCurBckFromIndex(7)
	    && unk154 > 0) {
		++unk294;
		if (unk294 > 160) {
			unk294 = 0;
			unk293 = 1;
			gpMarDirector->mConsole->startAppearBalloon(0xE002E, true);
		}
	}

	TGateKeeperBase::perform(flags, graphics);

	if (doAnim && !checkLiveFlag(LIVE_FLAG_CLIPPED_OUT))
		emitParticles();
}

void TBiancoGateKeeper::controlCollision()
{
	if (mMActor->checkCurBckFromIndex(0x0B)
	    || mMActor->checkCurBckFromIndex(0x07)) {
		unk174->unk70 = 1;
		unk161        = 0;
		return;
	}

	if (mMActor->checkCurBckFromIndex(0x12)) {
		unk174->unk70 = 1;
		unk161        = 0;
		return;
	}

	if (mMActor->checkCurBckFromIndex(0x0A)
	    || mMActor->checkCurBckFromIndex(0x04)) {
		unk174->unk70 = 0;
		unk161        = 1;
		return;
	}

	if (mMActor->checkCurBckFromIndex(0x0F)) {
		J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(MActor::ANM_TYPE_BCK);
		if (ctrl->getFrame() > 40.0f)
			unk174->unk70 = 1;
		else
			unk174->unk70 = 0;
		unk161 = 0;
		return;
	}

	if (mMActor->checkCurBckFromIndex(0x10)
	    || mMActor->checkCurBckFromIndex(0x0C)) {
		unk174->unk70 = 1;
		unk161        = 0;
		return;
	}

	unk174->unk70 = 0;
	unk161        = 0;
}

void TBiancoGateKeeper::emitParticles()
{
	const TNerveBase<TLiveActor>* sleep = &TNerveBGKSleep::theNerve();
	if (mSpine->getLatestNerve() == sleep)
		return;

	const TNerveBase<TLiveActor>* launchGoro
	    = &TNerveBGKLaunchGoro::theNerve();
	if (mSpine->getLatestNerve() == launchGoro)
		return;

	J3DModel* model = getModel();

	JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x140, (MtxPtr)((u8*)model->mNodeMatrices + 0x120), 1, this);
	if (emitter)
		SMSSetEmitterPolColor(emitter, 6);

	emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x142, (MtxPtr)((u8*)model->mNodeMatrices + 0x1B0), 1,
	    (u8*)this + 1);
	if (emitter)
		SMSSetEmitterPolColor(emitter, 6);

	emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x142, (MtxPtr)((u8*)model->mNodeMatrices + 0x0F0), 1,
	    (u8*)this + 2);
	if (emitter)
		SMSSetEmitterPolColor(emitter, 6);

	emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x141, (MtxPtr)((u8*)model->mNodeMatrices + 0x180), 1,
	    (u8*)this + 3);
	if (emitter)
		SMSSetEmitterPolColor(emitter, 6);
}

const char** TBiancoGateKeeper::getBasNameTable() const
{
	return gatekeeper_bastable;
}

BOOL TBiancoGateKeeper::isDamageFogSituation() const
{
	const TNerveBase<TLiveActor>* awakeDamage
	    = &TNerveBGKAwakeDamage::theNerve();

	if (mSpine->getLatestNerve() == awakeDamage && mSpine->getTime() <= 110)
		return true;

	if (mMActor->checkCurBckFromIndex(0x12)) {
		J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(MActor::ANM_TYPE_BCK);
		if (!(50.0f < ctrl->getFrame() && ctrl->getFrame() < 160.0f))
			return false;
	}

	if (mMActor->checkCurBckFromIndex(0x0B)
	    || mMActor->checkCurBckFromIndex(0x11)
	    || mMActor->checkCurBckFromIndex(0x0D)
	    || mMActor->checkCurBckFromIndex(0x07)
	    || mMActor->checkCurBckFromIndex(0x06))
		return false;

	if (unk17C > 0)
		return true;

	return false;
}

#pragma dont_inline on
BOOL TBiancoGateKeeper::isHeadHitActive() const
{
	J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(MActor::ANM_TYPE_BCK);
	MActor* actor      = mMActor;

	if (actor->checkCurBckFromIndex(0x12)) {
		if (50.0f < ctrl->getFrame() && ctrl->getFrame() < 160.0f)
			return true;
		return false;
	}

	if (mMActor->checkCurBckFromIndex(0x0F)) {
		if (ctrl->getFrame() > 12.0f)
			return true;
		return false;
	}

	if (mMActor->checkCurBckFromIndex(0x10)
	    || mMActor->checkCurBckFromIndex(0x0C))
		return true;

	return false;
}
#pragma dont_inline off

#pragma dont_inline on
f32 TBiancoGateKeeper::getRumblePow()
{
	JGeometry::TVec3<f32> delta = mPosition;
	delta.sub(*gpMarioPos);

	f32 distance = delta.length();
	if (distance == 0.0f)
		return 1.0f;

	f32 power = 2000.0f / distance;
	if (power > 1.0f)
		power = 1.0f;

	return power;
}
#pragma dont_inline off

void TBiancoGateKeeper::launchNamekuri()
{
	TNameKuriManager* manager = JDrama::TNameRefGen::search<TNameKuriManager>(
	    "拡散ナメクリマネージャー");
	if (manager == nullptr) {
		manager = JDrama::TNameRefGen::search<TNameKuriManager>(
		    "ナメクリマネージャー");
	}

	if (manager == nullptr)
		return;

	for (int i = 0; i < 10; ++i) {
		TNameKuri* nameKuri = (TNameKuri*)manager->getFarOutEnemy();
		if (nameKuri == nullptr)
			return;

		JGeometry::TVec3<f32> position = mPosition;
		position.y += 100.0f;
		JGeometry::TVec3<f32> rotation = mRotation;
		JGeometry::TVec3<f32> scaling(1.0f, 1.0f, 1.0f);

		f32 angle = 36.0f * i;
		JGeometry::TVec3<f32> velocity(4.0f * JMASin(angle), 12.0f,
		                                4.0f * JMACos(angle));

		nameKuri->reset();
		nameKuri->mPosition = position;
		nameKuri->mRotation = rotation;
		nameKuri->mScaling  = scaling;

		nameKuri->mHitPoints = nameKuri->getMaxHitPoints();
		nameKuri->mVelocity  = velocity;
		nameKuri->mLiveFlag |= LIVE_FLAG_AIRBORNE;
		nameKuri->mLiveFlag &= ~(LIVE_FLAG_DEAD | LIVE_FLAG_UNK8);
		nameKuri->mLiveFlag |= LIVE_FLAG_UNK8000;
		nameKuri->offHitFlag(HIT_FLAG_NO_COLLISION);

		nameKuri->mSpine->pushNerve(&TNerveNameKuriDiffuse::theNerve());
	}
}

void TBiancoGateKeeper::changeBck(int index)
{
	int curBck = mMActor->getCurAnmIdx(MActor::ANM_TYPE_BCK);

	if ((curBck == 0x11 && index == 0x0B)
	    || (curBck == 0x11 && index == 0x12)
	    || (curBck == 0x0B && index == 0x12)
	    || (curBck == 0x12 && index == 0x0B)
	    || (curBck == 0x07 && index == 0x07)
	    || (curBck == 0x07 && index == 0x12)
	    || (curBck == 0x0F && index == 0x10)
	    || (curBck == 0x10 && index == 0x0C)) {
		TBGKMtxCalc* calc = unk178;
		MActorAnmDataEach<J3DAnmTransformKey>* data
		    = calc->unk64->mMActorKeeper->getMActorAnmData()->getUnk2C();
		J3DAnmTransform* nextAnm = data->getAnmPtr(index);

		calc->unk54 = nextAnm;
		calc->unk58 = nullptr;
		calc->unk50 = 0.0f;

		MActorAnmBck* bck = mMActor->unkC;
		bck->unk0         = index;
		if (index >= 0) {
			bck->unk24 = bck->getData()->getAnmPtr(index);
			bck->unk4.init(bck->unk24->getFrameMax());
			bck->unk4.setAttribute(bck->unk24->getAttribute());
			bck->unk4.setRate(SMSGetAnmFrameRate());
		}
		unk158 = 0.0f;
	} else {
		TBGKMtxCalc* calc = unk178;
		MActorAnmDataEach<J3DAnmTransformKey>* data
		    = calc->unk64->mMActorKeeper->getMActorAnmData()->getUnk2C();
		J3DAnmTransform* nextAnm = data->getAnmPtr(index);

		if (calc->unk54 != nextAnm) {
			calc->unk58 = calc->unk54;
			calc->unk54 = nextAnm;
			calc->unk50 = 1.0f;
		}

		MActorAnmBck* bck = mMActor->unkC;
		bck->unk0         = index;
		if (index >= 0) {
			bck->unk24 = bck->getData()->getAnmPtr(index);
			bck->unk4.init(bck->unk24->getFrameMax());
			bck->unk4.setAttribute(bck->unk24->getAttribute());
			bck->unk4.setRate(SMSGetAnmFrameRate());
		}

		J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(MActor::ANM_TYPE_BCK);
		if (ctrl) {
			f32 blendFrames = 0.2f * ctrl->getEnd();
			if (blendFrames < 1.0f)
				unk158 = 1.0f;
			else
				unk158 = 1.0f / blendFrames;
		} else {
			unk158 = 1.0f;
		}
	}

	const char** basTable = getBasNameTable();
	const char* basName;
	if (basTable == nullptr)
		basName = nullptr;
	else
		basName = basTable[index];
	setAnmSound(basName);
}

void TBiancoGateKeeper::kill()
{
	TGateKeeperBase::kill();
	onHitFlag(HIT_FLAG_NO_COLLISION);
	unk174->onHitFlag(HIT_FLAG_NO_COLLISION);
	unk28C->onHitFlag(HIT_FLAG_NO_COLLISION);
}

void TBiancoGateKeeper::init(TLiveManager* manager)
{
	static const char enemyGroupName[]
	    = "\x93\x47\x83\x4F\x83\x8B\x81\x5B\x83\x76";
	static const char riccoGateKeeperName[]
	    = "\x83\x51\x81\x5B\x83\x67\x83\x4C\x81\x5B\x83\x70\x81\x5B\x81\x69"
	      "\x83\x8A\x83\x52\x81\x6A";
	static const char mammaGateKeeperName[]
	    = "\x83\x51\x81\x5B\x83\x67\x83\x4C\x81\x5B\x83\x70\x81\x5B\x81\x69"
	      "\x83\x7D\x83\x93\x83\x7D\x81\x6A";
	static const char obstacleName[] = "TBGKObstacle";
	static const char headHitName[]
	    = "\x93\xAA\x83\x71\x83\x62\x83\x67";

	mManager = manager;
	mManager->manageActor(this);

	mMActorKeeper = new TMActorKeeper(mManager, 3);
	mMActor       = mMActorKeeper->createMActor("gene_pakkun_model1.bmd", 3);

	if (strcmp(mName, riccoGateKeeperName) == 0) {
		unk292 = 3;
		if (!TFlagManager::smInstance->getBool(0x50001))
			onLiveFlag(LIVE_FLAG_DEAD);
	} else if (strcmp(mName, mammaGateKeeperName) == 0) {
		unk292 = 4;
		if (!TFlagManager::smInstance->getBool(0x50002))
			onLiveFlag(LIVE_FLAG_DEAD);
	} else if (gpMarDirector->mMap == 0) {
		unk292 = 0;
	} else if (gpMarDirector->mMap == 1) {
		unk292 = 2;
	} else {
		unk292 = 1;
	}

	if (gpMarDirector->mMap == 1) {
		unk150
		    = mMActorKeeper->createMActor("stamp_keeper_model1.bmd", 3);
		unk150->setBckFromIndex(19);

		if (unk292 == 2)
			unk164.scale(0.8f);
		else if (unk292 == 3)
			unk164.scale(1.2f);
		else if (unk292 == 4)
			unk164.scale(1.4f);
	}

	initHitActor(0x10000022, 5, 0x81000000, 400.0f, 150.0f, 400.0f,
	             150.0f);
	TIdxGroupObj* enemyGroup
	    = JDrama::TNameRefGen::search<TIdxGroupObj>(enemyGroupName);
	enemyGroup->getChildren().push_back(this);
	offHitFlag(HIT_FLAG_NO_COLLISION);

	mSpine->initWith(&TNerveBGKSleep::theNerve());

	J3DModel* model = mMActor->getModel();
	if (!model->getSkinDeform())
		model->setSkinDeform(new J3DSkinDeform, J3D_DEFORM_ATTACH_FLAG_UNK_1);

	unk178 = new TBGKMtxCalc(this);
	mMActor->setCalcForBck((J3DMtxCalc*)unk178);

	mHitPoints = 3;
	mMActor->offMakeDL();

	unk15C = new TMultiBtk(2, getModel()->getModelData());
	MActorAnmDataEach<J3DAnmTextureSRTKey>* btkData
	    = mMActorKeeper->getMActorAnmData()->getUnk38();
	for (int i = 0; i < 2; ++i)
		unk15C->setNthData(i, btkData->getAnmPtr(i));

	TBGKObstacle* obstacle = new TBGKObstacle(obstacleName);
	obstacle->mPosition = mPosition;
	obstacle->mPosition.y -= 1000.0f;
	obstacle->initHitActor(0x10000022, 1, 0x80000000, 800.0f, 800.0f,
	                       800.0f, 800.0f);
	obstacle->offHitFlag(HIT_FLAG_NO_COLLISION);
	JDrama::TNameRefGen::search<TIdxGroupObj>(enemyGroupName)->add(obstacle);
	unk28C = obstacle;

	ResTIMG* tex = (ResTIMG*)JKRFileLoader::getGlbResource(
	    "/scene/map/pollution/H_ma_rak.bti");
	if (tex)
		SMS_ChangeTextureAll(mMActor->getModel()->getModelData(),
		                     "Q_kepper_dummy_128IA4", *tex);

	initAnmSound();
	(void)getBasNameTable();

	unk174        = new TGKHitObj(headHitName);
	unk174->unk68 = this;
	unk174->unk6C = 10;
	unk174->unk70 = 0;
	unk174->initHitActor(0x10000022, 1, 0x80000000, 0.0f, 0.0f, 150.0f,
	                      200.0f);
	unk174->offHitFlag(HIT_FLAG_NO_COLLISION);
	JDrama::TNameRefGen::search<TIdxGroupObj>(enemyGroupName)->add(unk174);

	mMActor->calc();
	if (unk150)
		unk150->calc();

	if (checkLiveFlag(LIVE_FLAG_DEAD)) {
		onHitFlag(HIT_FLAG_NO_COLLISION);
		unk174->onHitFlag(HIT_FLAG_NO_COLLISION);
		unk28C->onHitFlag(HIT_FLAG_NO_COLLISION);
	}
}

void TBGKMtxCalc::calc(u16 joint_no)
{
	M3UMtxCalcSIAnmBlendQuat::calc(joint_no);

	if (joint_no == 0x0B) {
		TBiancoGateKeeper* gatekeeper = unk64;
		J3DModel* model              = gatekeeper->mMActor->getModel();
		MtxPtr jointMtx              = model->mNodeMatrices[joint_no];
		MsMtxSetXYZRPH(jointMtx, gatekeeper->mPosition.x,
		               gatekeeper->mPosition.y, gatekeeper->mPosition.z,
		               gatekeeper->mRotation.x, gatekeeper->mRotation.y,
		               gatekeeper->mRotation.z);
		PSMTXCopy(jointMtx, J3DSys::mCurrentMtx);
		return;
	}

	if (joint_no != 0)
		return;

	J3DModel* model = unk64->mMActor->getModel();
	MtxPtr jointMtx              = model->mNodeMatrices[joint_no];

	if (!gpMarDirector->checkUnk124Thing2()
	    && !gpMarDirector->isTalkModeNow()) {
		if (unk64->mMActor->checkCurBckFromIndex(0x0B)
		    || unk64->mMActor->checkCurBckFromIndex(0x12)
		    || unk64->mMActor->checkCurBckFromIndex(0x0F)
		    || unk64->mMActor->checkCurBckFromIndex(0x10)
		    || unk64->mMActor->checkCurBckFromIndex(0x0C)) {
			JGeometry::TVec3<f32> delta = *gpMarioPos;
			delta.sub(unk64->mPosition);

			f32 targetYaw = MsGetRotFromZaxisY(delta);
			f32 currentYaw = unk64->mRotation.y + unk64->unk180;
			while (targetYaw >= 180.0f)
				targetYaw -= 360.0f;
			while (targetYaw < -180.0f)
				targetYaw += 360.0f;

			while (currentYaw >= 180.0f)
				currentYaw -= 360.0f;
			while (currentYaw < -180.0f)
				currentYaw += 360.0f;

			f32 diff = targetYaw
			           - callMsWrap(currentYaw, targetYaw - 180.0f,
			                        targetYaw + 180.0f);
			if (diff > 0.0f) {
				if (diff > 3.0f)
					diff = 3.0f;
			} else {
				if (diff < -3.0f)
					diff = -3.0f;
			}

			unk64->unk180
			    = MsWrap<f32>(currentYaw + diff - unk64->mRotation.y, 0.0f,
			                  360.0f);
		}
	}

	s16 angle = (s16)(unk64->unk180 * (65536.0f / 360.0f));
	f32 s     = JMASSin(angle);
	f32 c     = JMASCos(angle);

	Mtx rot;
	rot[0][0] = c;
	rot[0][1] = 0.0f;
	rot[0][2] = s;
	rot[0][3] = 0.0f;
	rot[1][0] = 0.0f;
	rot[1][1] = 1.0f;
	rot[1][2] = 0.0f;
	rot[1][3] = 0.0f;
	rot[2][0] = -s;
	rot[2][1] = 0.0f;
	rot[2][2] = c;
	rot[2][3] = 0.0f;

	PSMTXConcat(jointMtx, rot, jointMtx);
	PSMTXCopy(jointMtx, J3DSys::mCurrentMtx);
}

void TBiancoGateKeeperManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "gene_pakkun_model1.bmd", 0x11210000, 0 },
		{ "stamp_keeper_model1.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TBiancoGateKeeperManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TBiancoGateKeeperParams("/enemy/gatekeeper.prm");
	TEnemyManager::load(stream);
	initJParticle();
}

void TBiancoGateKeeperManager::initJParticle()
{
	SMS_LoadParticle("/scene/gatekeeper/jpa/ms_gkpa_dead.jpa", 0xA7);
	SMS_LoadParticle("/scene/gatekeeper/jpa/ms_gkpa_deadsmoke.jpa", 0xA8);
	SMS_LoadParticle("/scene/gatekeeper/jpa/ms_gkpa_bota.jpa", 0x140);
	SMS_LoadParticle("/scene/gatekeeper/jpa/ms_gkpa_yodare_s.jpa", 0x141);
	SMS_LoadParticle("/scene/gatekeeper/jpa/ms_gkpa_yodare_l.jpa", 0x142);
	SMS_LoadParticle("/scene/gatekeeper/jpa/ms_gkpa_kemuri.jpa", 0x1DF);
	SMS_LoadParticle("/scene/gatekeeper/jpa/ms_gkpa_kemuri_w.jpa", 0x1E0);
}

TBiancoGateKeeperManager::TBiancoGateKeeperManager(const char* name)
    : TEnemyManager(name)
{
}

void TGateKeeperBase::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (checkLiveFlag(LIVE_FLAG_DEAD))
		return;

	if (flags & 2)
		unk15C->update();

	if (flags & 1) {
		f32 blend;
		if (!mMActor->unkC)
			blend = 0.0f;
		else
			blend = mMActor->unkC->getMotionBlendRatio();

		blend -= unk158;
		if (blend < 0.0f)
			blend = 0.0f;

		if (mMActor->unkC)
			mMActor->unkC->setMotionBlendRatio(blend);
	}

	TSpineEnemy::perform(flags, graphics);

	if (unk160 && unk150) {
		if (flags & 2) {
			J3DModel* model = getModel();
			unk150->getModel()->setBaseScale(unk164);
			PSMTXCopy(model->getBaseTRMtx(),
			          unk150->getModel()->getBaseTRMtx());
			unk150->calcAnm();
		}

		if (flags & 0x200)
			gpPollution->stampModel(unk150->getModel());
	}

	if (flags & 1)
		unk154 = 0;
}

BOOL TGateKeeperBase::receiveMessage(THitActor* sender, u32 message)
{
	if ((sender->mActorType - 0x01000000) == 1) {
		if (unk161 && message == HIT_MESSAGE_SPRAYED_BY_WATER)
			unk154 += 1;

		gpMarioParticleManager->emit(0xE7, &sender->mPosition, 0, nullptr);
		gpMSound->startSoundSet(0x6802, &sender->mPosition, 0, 0.0f, 0, 0, 4);
		return true;
	}

	return false;
}

void TGateKeeperBase::kill() { onLiveFlag(LIVE_FLAG_DEAD); }

void TGKHitObj::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (unk68->checkLiveFlag(LIVE_FLAG_DEAD))
		return;

	if (flags & 2) {
		if (!unk68->checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)) {
			MtxPtr mtx = unk68->mMActor->unk4->getAnmMtx(unk6C);
			mPosition.set(mtx[0][3], mtx[1][3], mtx[2][3]);
		} else {
			mPosition = unk68->mPosition;
		}
	}

	THitActor::perform(flags, graphics);
}

BOOL TGKHitObj::receiveMessage(THitActor* sender, u32 message)
{
	if ((sender->mActorType - 0x01000000) == 1
	    && message == HIT_MESSAGE_SPRAYED_BY_WATER) {
		if (unk70)
			unk68->unk154 += 1;

		gpMarioParticleManager->emit(0xE7, &sender->mPosition, 0, nullptr);
		gpMSound->startSoundSet(0x6802, &sender->mPosition, 0, 0.0f, 0, 0, 4);
		return true;
	}

	return unk68->receiveMessage(sender, message);
}
