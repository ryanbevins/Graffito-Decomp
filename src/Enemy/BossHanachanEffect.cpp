#include <Enemy/BossHanachan.hpp>
#include <Enemy/BossHanachanSaveParams.hpp>
#include <Enemy/BossHanachanSub.hpp>

#include <Camera/CameraShake.hpp>
#include <Camera/SunMgr.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <M3DUtil/MActor.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <math.h>
#include <Player/MarioAccess.hpp>
#include <Strategic/Spine.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>

static const f32 sSnortStepFrames[3]   = { 21.0f, 36.0f, 55.0f };
static const f32 sEmitSandFrameFoot[2] = { 14.0f, 34.0f };

void TBossHanachan::emitCamShake_()
{
	if (gpMarDirector->mState == 1)
		return;

	const TNerveBase<TLiveActor>* nerve = mSpine->getLatestNerve();
	J3DFrameCtrl* frameCtrl             = mBody[0]->getMActor()->getFrameCtrl(0);
	bool touching                       = SMS_IsMarioTouchGround4cm();

	if (nerve == &TNerveBossHanachanGraphWander::theNerve()) {
		f32 dist = MsSqrtf(mDistToMarioSquared);

		f32 ratio;
		if (dist <= mParams->mSLCamShakeMaxDist.value) {
			ratio = 1.0f;
		} else if (dist >= mParams->mSLCamShakeZeroDist.value) {
			ratio = 0.0f;
		} else {
			f32 r = CLBCalcRatio<f32>(mParams->mSLCamShakeZeroDist.value,
			                          mParams->mSLCamShakeMaxDist.value, dist);
			if (r > 1.0f)
				r = 1.0f;
			else if (r < 0.0f)
				r = 0.0f;
			ratio = r;
		}

		for (int i = 0; i < 2; i++) {
			if (frameCtrl->checkPass(sEmitSandFrameFoot[i])) {
				gpCameraShake->startShake((EnumCamShakeMode)0xB, ratio);
				if (touching) {
					for (int j = 0; j < 8; j++) {
						SMSRumbleMgr->start(
						    8, (Vec*)&((TBossHanachanPartsBody*)mBody[j])
						           ->unk154);
					}
				}
			}
		}
	} else if (nerve == &TNerveBossHanachanSnort::theNerve()) {
		for (int i = 0; i < 3; i++) {
			if (frameCtrl->checkPass(sSnortStepFrames[i])) {
				gpCameraShake->startShake((EnumCamShakeMode)0xA, 1.0f);
				if (touching) {
					SMSRumbleMgr->start(8, (f32*)NULL);
				}
			}
		}
	} else if (nerve == &TNerveBossHanachanGetUp::theNerve()) {
		switch (mBody[0]->mCurAnm) {
		case 0x9:
		case 0xC:
			if (frameCtrl->checkPass(9.0f)) {
				gpCameraShake->startShake((EnumCamShakeMode)0xC, 1.0f);
				if (touching) {
					SMSRumbleMgr->start(8, (f32*)NULL);
				}
				if (mLiveFlag & 0x20000) {
					MSBgm::stopTrackBGM(1, 30);
				}
			}
			break;
		}
	}
}

void TBossHanachan::emitOneTimeSandPillar_(TBossHanachanPartsBody* body)
{
	onLiveFlag(0x10000);

	MtxPtr m = body->mCenterJointMtx;
	mEffectPos.set<f32>(m[0][3], mPosition.y, m[2][3]);

	const TLiveActor* sand = body->getSandActor_();
	if (sand) {
		mEffectPos.x = 0.5f * (mEffectPos.x + sand->mPosition.x);
		mEffectPos.z = 0.5f * (mEffectPos.z + sand->mPosition.z);
		mEffectPos.y = sand->mPosition.y;
	}

	gpMarioParticleManager->emit(0x7E, &mEffectPos, 0, NULL);

	MtxPtr emtx = mEffectActor->getModel()->unk20;
	emtx[0][3]  = mEffectPos.x;
	emtx[1][3]  = mEffectPos.y;
	emtx[2][3]  = mEffectPos.z;

	mEffectActor->setBckFromIndex(0x25);
	mEffectActor->setBtkFromIndex(2);
	mEffectActor->setBrkFromIndex(2);

	gpCameraShake->startShake((EnumCamShakeMode)8, 1.0f);

	if (gpMSound->gateCheck(0x2884)) {
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x2884, (const Vec*)&mEffectPos, 0, (JAISound**)NULL, 0, 4);
	}
}

void TBossHanachan::emitParticle_()
{
	const TNerveBase<TLiveActor>* nerve = mSpine->getLatestNerve();

	if (nerve == &TNerveBossHanachanDead::theNerve())
		return;

	f32 waterY = gpSunMgr->unk20;

	if (nerve != &TNerveBossHanachanSnort::theNerve()) {
		if (((TBossHanachanPartsHead*)mHead)->mWaterHit->unk68 > 0) {
			gpMarioParticleManager->emitAndBindToMtxPtr(
			    0x169,
			    ((TBossHanachanPartsHead*)mHead)->mRightNoseHallJointMtx, 1,
			    (TBossHanachanPartsHead*)mHead);
			gpMarioParticleManager->emitAndBindToMtxPtr(
			    0x16B,
			    ((TBossHanachanPartsHead*)mHead)->mLeftNoseHallJointMtx, 1,
			    (TBossHanachanPartsHead*)mHead);
			gpMarioParticleManager->emitAndBindToMtxPtr(
			    0x16A,
			    ((TBossHanachanPartsHead*)mHead)->mRightNoseHallJointMtx, 1,
			    (TBossHanachanPartsHead*)mHead);
			gpMarioParticleManager->emitAndBindToMtxPtr(
			    0x16C,
			    ((TBossHanachanPartsHead*)mHead)->mLeftNoseHallJointMtx, 1,
			    (TBossHanachanPartsHead*)mHead);
		}
	}

	if (nerve == &TNerveBossHanachanTumble::theNerve()
	    || (nerve == &TNerveBossHanachanDamage::theNerve()
	        && mMarchSpeed > 0.001f)) {
		for (int i = 0; i < 8; i++) {
			JGeometry::TVec3<f32> pos;
			pos = mSphereLink->mPoints[i].mPos;
			if (pos.y > waterY) {
				TBossHanachanPartsBody* body
				    = (TBossHanachanPartsBody*)mBody[i];
				gpMarioParticleManager->emitAndBindToPosPtr(
				    0x16E,
				    (const JGeometry::TVec3<f32>*)&body->unk154, 1, body);
			}
		}
		for (int i = 0; i < 8; i++) {
			JGeometry::TVec3<f32> pos;
			pos = mSphereLink->mPoints[i].mPos;
			if (pos.y > waterY) {
				TBossHanachanPartsBody* body
				    = (TBossHanachanPartsBody*)mBody[i];
				gpMarioParticleManager->emitAndBindToPosPtr(
				    0x16D,
				    (const JGeometry::TVec3<f32>*)&body->unk154, 1, body);
			}
		}
	}

	if (nerve == &TNerveBossHanachanGraphWander::theNerve()) {
		for (int i = 0; i < 8; i++) {
			J3DFrameCtrl* fc = ((TBossHanachanPartsBody*)mBody[i])
			                       ->getMActor()
			                       ->getFrameCtrl(0);
			for (int j = 0; j < 2; j++) {
				if (!fc->checkPass(sEmitSandFrameFoot[j]))
					continue;
				TFootHitActor* foot
				    = ((TBossHanachanPartsBody*)mBody[i])->mFeet[j];
				MtxPtr footMtx = foot->unk6C;
				if (MsRandF() >= mChangeParams->mSLParticleProbability.value)
					continue;

				if (footMtx[1][3] < waterY) {
					MtxPtr legMtx
					    = (&((TBossHanachanPartsBody*)mBody[i])
					            ->mLeftLegJointMtx)[j];
					JGeometry::TVec3<f32> p;
					p.set<f32>(legMtx[0][3], waterY, legMtx[2][3]);
					gpMarioParticleManager->emit(0x7C, &p, 0, NULL);
					gpMarioParticleManager->emit(0x7D, &p, 0, NULL);
				} else {
					JGeometry::TVec3<f32> p;
					p.set<f32>(footMtx[0][3], footMtx[1][3], footMtx[2][3]);
					gpMarioParticleManager->emit(0x77, &p, 0, NULL);
					gpMarioParticleManager->emit(0x76, &p, 0, NULL);
				}
			}
		}
	} else if (nerve == &TNerveBossHanachanSnort::theNerve()) {
		J3DFrameCtrl* fc = mHead->getMActor()->getFrameCtrl(0);
		if (fc->checkPass(134.0f)) {
			gpMarioParticleManager->emitAndBindToMtxPtr(
			    0x78,
			    ((TBossHanachanPartsHead*)mHead)->mRightNoseHallJointMtx, 0,
			    NULL);
			gpMarioParticleManager->emitAndBindToMtxPtr(
			    0x7A,
			    ((TBossHanachanPartsHead*)mHead)->mLeftNoseHallJointMtx, 0,
			    NULL);
			gpMarioParticleManager->emitAndBindToMtxPtr(
			    0x79,
			    ((TBossHanachanPartsHead*)mHead)->mRightNoseHallJointMtx, 0,
			    NULL);
			gpMarioParticleManager->emitAndBindToMtxPtr(
			    0x7B,
			    ((TBossHanachanPartsHead*)mHead)->mLeftNoseHallJointMtx, 0,
			    NULL);
		}
	} else if (nerve == &TNerveBossHanachanDown::theNerve()) {
		TBossHanachanPartsBase* head = mHead;
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x16F, head->mCenterJointMtx, 1, head);
	}
}

void TBossHanachan::staticLoadParticle()
{
	SMS_LoadParticle("ms_boha_sandsmo.jpa", 0x76);
	SMS_LoadParticle("ms_boha_sand.jpa", 0x77);
	SMS_LoadParticle("ms_boha_jouki_r_a.jpa", 0x78);
	SMS_LoadParticle("ms_boha_jouki_r_b.jpa", 0x79);
	SMS_LoadParticle("ms_boha_jouki_l_a.jpa", 0x7A);
	SMS_LoadParticle("ms_boha_jouki_l_b.jpa", 0x7B);
	SMS_LoadParticle("ms_boha_hamon_a.jpa", 0x7C);
	SMS_LoadParticle("ms_boha_hamon_b.jpa", 0x7D);
	SMS_LoadParticle("ms_boha_crash_a.jpa", 0x7E);
	SMS_LoadParticle("ms_boha_jouki2_r_a.jpa", 0x169);
	SMS_LoadParticle("ms_boha_jouki2_r_b.jpa", 0x16A);
	SMS_LoadParticle("ms_boha_jouki2_l_a.jpa", 0x16B);
	SMS_LoadParticle("ms_boha_jouki2_l_b.jpa", 0x16C);
	SMS_LoadParticle("ms_boha_sandsmo_sl.jpa", 0x16D);
	SMS_LoadParticle("ms_boha_sand_sl.jpa", 0x16E);
	SMS_LoadParticle("ms_boha_kizetsu.jpa", 0x16F);
}
